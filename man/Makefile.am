if HAVE_HELP2MAN

HELP2MAN=help2man -N
SUFFIXES = .1

gfcat.1: $(top_builddir)/build/bin/gfcat
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfcli.1: $(top_builddir)/build/bin/gfcli
	$(HELP2MAN) --output=$@-t $(top_builddir)/build/bin/$* \
		&& mv $@-t $@

gfcp.1: $(top_builddir)/build/bin/gfcp
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfls.1: $(top_builddir)/build/bin/gfls
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfmkdir.1: $(top_builddir)/build/bin/gfmkdir
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfchmod.1: $(top_builddir)/build/bin/gfchmod
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfwc.1: $(top_builddir)/build/bin/gfwc
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfput.1: $(top_builddir)/build/bin/gfput
	$(HELP2MAN) --output=$@-t $(top_builddir)/build/bin/$* \
		&& mv $@-t $@

gfrm.1: $(top_builddir)/build/bin/gfrm
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfstat.1: $(top_builddir)/build/bin/gfstat
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfhead.1: $(top_builddir)/build/bin/gfhead
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gftail.1: $(top_builddir)/build/bin/gftail
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

man1_MANS = gfcat.1 \
	    gfcli.1 \
	    gfchmod.1 \
	    gfcp.1 \
	    gfls.1 \
	    gfmkdir.1 \
	    gfput.1 \
	    gfrm.1 \
	    gfstat.1 \
	    gfhead.1 \
	    gftail.1 \
	    gfwc.1

EXTRA_DIST = common_seealso.h2m

$(man1_MANS): $(srcdir)/common_seealso.h2m

CLEANFILES = $(man1_MANS)

endif
