/**
 * A utility to read the last n bytes or lines from a file on a remote Gluster
 * volume and stream it to stdout.
 *
 * Copyright (C) 2017 Redhat Inc.
 *
 *      This program is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include "glfs-head.h"
#include "glfs-util.h"

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <glusterfs/api/glfs.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define AUTHORS "Written by Jayadeep KM."

static volatile int keep_running = 1;

static void
int_handler (int value)
{
        keep_running = 0;
}

enum head_mode
{
        BYTES,
        LINES
};

/**
 * Used to store the state of the program, including user supplied options.
 *
 * gluster_url: Struct of the parsed url supplied by the user.
 * url: Full url used to find the remote file.
 * bytes: Number of bytes to print from the end of the file.
 * debug: Whether to log additional debug information.
 * lines: Number of lines to print from the end of the file.
 * mode: The mode the application is in (bytes vs lines).
 */
struct state {
        struct gluster_url *gluster_url;
        struct xlator_option *xlator_options;
        char *url;
        unsigned int bytes;
        bool debug;
        unsigned int lines;
        enum head_mode mode;
};

static struct state *state;

static struct option const long_options[] =
{
        {"bytes", required_argument, NULL, 'c'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'x'},
        {"lines", required_argument, NULL, 'n'},
        {"xlator-option", required_argument, NULL, 'o'},
        {"port", required_argument, NULL, 'p'},
        {"version", no_argument, NULL, 'v'},
        {NULL, no_argument, NULL, 0}
};

/**
 * Prints usage information.
 */
static void
usage ()
{
        printf ("Usage: %s [OPTION]... URL\n"
                "Print the first 10 lines (default) of the file to standard output.\n\n"
                "  -c, --bytes=K                output the first K bytes\n"
                "  -n, --lines=K                output the first K lines, instead of the last 10 bytes\n"
                "  -o, --xlator-option=OPTION   specify a translator option for the\n"
                "                               connection. Multiple options are supported\n"
                "                               and take the form xlator.key=value.\n"
                "  -p, --port=PORT              specify the port on which to connect\n"
                "      --help     display this help and exit\n"
                "      --version  output version information and exit\n\n"
                "Examples:\n"
                "  gfhead glfs://localhost/groot/file\n"
                "         Head the first 10 lines of the file /file on the Gluster\n"
                "         volume groot on host localhost.\n"
                "  gfhead -c 100 glfs://localhost/groot/file\n"
                "         Head the first 100 bytes of the file /file on the Gluster\n"
                "         volume groot on host localhost.\n"
                "  gfcli (localhost/groot)> head /example\n"
                "        In the context of a shell with a connection established,\n"
                "        head the file example on the root of the Gluster volume\n"
                "        groot on localhost.\n",
                program_invocation_name);
}

/**
 * Helper function that converts a char * string into an int. Returns -1 on failure.
 */
static int
strtoint (const char *str)
{
        long int raw_value;
        int value = -1;
        char *end;

        raw_value = strtol (str, &end, 10);

        if (str == end) {
                goto out;
        }

        if (raw_value > INT_MAX || raw_value < 0) {
                goto out;
        }

        value = (int) raw_value;

out:
        return value;
}

/**
 * Parses command line flags into a global application state.
 */
static int
parse_options (int argc, char *argv[], bool has_connection)
{
        uint16_t port = GLUSTER_DEFAULT_PORT;
        int ret = -1;
        int opt = 0;
        int option_index = 0;
        struct xlator_option *option;

        // Reset getopt since other utilities may have called it already.
        optind = 0;
        while (true) {
                opt = getopt_long (argc, argv, "c:dfl:n:o:p:s:", long_options, &option_index);

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                        case 'c':
                                state->bytes = strtoint (optarg);
                                if (state->bytes == -1) {
                                        error (0, 0, "invalid number of bytes: \"%s\"", optarg);
                                        goto out;
                                }

                                state->mode = BYTES;
                                break;
                        case 'd':
                                state->debug = true;
                                break;
                        case 'n':
                                state->lines = strtoint (optarg);
                                if (state->lines == -1) {
                                        error (0, 0, "invalid number of lines: \"%s\"", optarg);
                                        goto out;
                                }

                                state->mode = LINES;
                                break;
                        case 'o':
                                option = parse_xlator_option (optarg);
                                if (option == NULL) {
                                        error (0, errno, "%s", optarg);
                                        goto err;
                                }

                                if (append_xlator_option (&state->xlator_options, option) == -1) {
                                        error (0, errno, "append_xlator_option: %s", optarg);
                                        goto err;
                                }

                                break;
                        case 'p':
                                port = strtoport (optarg);
                                if (port == 0) {
                                        goto err;
                                }

                                break;
                        case 'v':
                                printf ("%s (%s) %s\n%s\n%s\n%s\n",
                                        program_invocation_name,
                                        PACKAGE_NAME,
                                        PACKAGE_VERSION,
                                        COPYRIGHT,
                                        LICENSE,
                                        AUTHORS);
                                ret = -2;
                                goto out;
                        case 'x':
                                usage ();
                                ret = -2;
                                goto out;
                        default:
                                goto err;
                }
        }

        if ((argc - option_index) < 2) {
                error (0, 0, "missing operand");
                goto err;
        } else {
                state->url = strdup (argv[argc - 1]);
                if (state->url == NULL) {
                        error (0, errno, "strdup");
                        goto out;
                }

                if (has_connection) {
                        state->gluster_url = gluster_url_init ();
                        state->gluster_url->path = strdup (argv[argc -1]);
                        ret = 0;
                        goto out;
                }

                ret = gluster_parse_url (argv[argc - 1], &(state->gluster_url));
                if (ret == -1) {
                        error (0, EINVAL, "%s", state->url);
                        goto err;
                }

                state->gluster_url->port = port;
        }

        goto out;

err:
        error (0, 0, "Try --help for more information.");
out:
        return ret;
}

/**
 * Initializes the global application state.
 */
static struct state*
init_state ()
{
        struct state *state = malloc (sizeof (*state));

        if (state == NULL) {
                goto out;
        }

        state->bytes = 0;
        state->debug = false;
        state->gluster_url = NULL;
        state->lines = 10;
        state->mode = LINES;
        state->url = NULL;
        state->xlator_options = NULL;

out:
        return state;
}

/**
 * Sets the offset of the fd object based on the number of bytes.
 */
static int
head_lines (glfs_fd_t *fd)
{
        char buffer[BUFSIZE];
        size_t num_read = 0;
        size_t num_written = 0;
        size_t ret = 0;
        size_t total_written = 0;
        int dst = STDOUT_FILENO;
        unsigned long lines = state->lines;
        size_t lines_remaining = lines;
        size_t bytes_to_write = 0;
        size_t loop_chars = 0;
        size_t lines_written = 0;
        size_t lines_writing = 0;

        while (true) {
                num_read = glfs_read (fd, buffer, BUFSIZE, 0);
                if (num_read == -1) {
                        goto out;
                }

                if (num_read == 0) {
                        goto out;
                }

                for (num_written = 0; num_written < num_read;) {
                        bytes_to_write = num_read - num_written;
                        lines_writing = 0;
                        for(loop_chars=0;loop_chars<bytes_to_write;loop_chars++){
                                if(buffer[num_written+loop_chars]=='\n'){
                                        lines_writing++;
                                        if(lines_written+lines_writing==lines){
                                                bytes_to_write = loop_chars+1;
                                                break;
                                        }
                                }
                        }

                        ret = write (dst,
                                     &buffer[num_written],
                                     bytes_to_write );
                        if (ret != bytes_to_write) {
                                goto err;
                        }

                        num_written += ret;
                        total_written += ret;
                        lines_written +=lines_writing;
                        if(lines_written==lines)
                                goto out;
                }
                if(lines_written==lines)
                        goto out;
        }

err:
        ret = -1;
out:
        return ret;
}


static int
head_bytes (glfs_fd_t *fd)
{
        char buffer[BUFSIZE];
        size_t num_read = 0;
        size_t num_written = 0;
        size_t ret = 0;
        size_t total_written = 0;
        int dst = STDOUT_FILENO;
        unsigned long bytes = state->bytes;
        size_t bytes_remaining = bytes;
        size_t bytes_to_write = 0;

        while (true) {
                num_read = glfs_read (fd, buffer, BUFSIZE, 0);
                if (num_read == -1) {
                        goto out;
                }

                if (num_read == 0) {
                        goto out;
                }

                for (num_written = 0; num_written < num_read && bytes_remaining>0;) {
                        bytes_remaining = bytes - total_written;
                        bytes_to_write = bytes_remaining;
                        if(num_read - num_written < bytes_to_write)
                                bytes_to_write = num_read - num_written;
                        ret = write (dst,
                                     &buffer[num_written],
                                     bytes_to_write);
                        if (ret != bytes_to_write) {
                                goto err;
                        }

                        num_written += ret;
                        total_written += ret;
                }
                if(bytes_remaining<=0)
                        goto out;
        }

err:
        ret = -1;
out:
        return ret;
}

static int
head (glfs_t *fs)
{
        glfs_fd_t *fd = NULL;
        int ret;
        long long size;

        fd = glfs_open (fs, state->gluster_url->path, O_RDONLY);
        if (fd == NULL) {
                error (0, errno, "error reading `%s'", state->url);
                goto err;
        }

        switch (state->mode) {
                case BYTES:
                        ret = head_bytes (fd);
                        break;
                case LINES:
                        ret = head_lines (fd);
                        break;
                default:
                        error (0, 0, "unknown error");
                        goto err;
        }

        if (ret == -1) {
                goto err;
        }

        ret = gluster_read (fd, STDOUT_FILENO);
        if (ret == -1) {
                error (0, errno, "write error");
                goto err;
        }

        goto out;
err:
        ret = -1;
out:
        // Disable our signal handler
        // FIXME: This clobbers gfcli's signal handler.
        signal (SIGINT, SIG_DFL);

        if (fd) {
                if (glfs_close (fd) == -1) {
                        error (0, errno, "failed to close file");
                        ret = -1;
                }
        }

        return ret;
}

static int
do_head_without_context ()
{
        glfs_t *fs = NULL;
        int ret;

        ret = gluster_getfs (&fs, state->gluster_url);
        if (ret == -1) {
                error (0, errno, "%s", state->url);
                goto out;
        }

        ret = apply_xlator_options (fs, &state->xlator_options);
        if (ret == -1) {
                error (0, errno, "failed to apply xlator options");
                goto out;
        }

        if (state->debug) {
                ret = glfs_set_logging (fs, "/dev/stderr", GF_LOG_DEBUG);

                if (ret == -1) {
                        error (0, errno, "failed to set logging level");
                        goto out;
                }
        }

        ret = head (fs);

out:
        if (fs) {
                glfs_fini (fs);
        }

        return ret;
}

/**
 * Main entry point into application (called from glfs-cli.c)
 */
int
do_head (struct cli_context *ctx)
{
        int argc = ctx->argc;
        char **argv = ctx->argv;
        int ret;

        state = init_state ();
        if (state == NULL) {
                error (0, errno, "failed to initialize state");
                goto out;
        }

        if (ctx->fs) {
                ret = parse_options (argc, argv, true);
                if (ret != 0) {
                        goto out;
                }

                ret = head (ctx->fs);
        } else {
                ret = parse_options (argc, argv, false);
                if (ret == -1) {
                        goto out;
                }

                // Valid options were passed and executed, but we don't want to
                // continue with execution.
                if (ret == -2) {
                        ret = 0;
                        goto out;
                }

                ret = do_head_without_context ();
        }

out:
        if (state) {
                gluster_url_free (state->gluster_url);
                free (state->url);
        }

        free (state);

        return ret;
}
