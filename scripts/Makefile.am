bin_SCRIPTS = fakechroot ldd.fakechroot
EXTRA_DIST = fakechroot.sh ldd.pl restoremode.sh savemode.sh
CLEANFILES = $(bin_SCRIPTS)

do_subst = sed -e 's,[@]libpath[@],$(libpath),g' \
	       -e 's,[@]VERSION[@],$(VERSION),g'

fakechroot: $(srcdir)/fakechroot.sh
	$(do_subst) < $(srcdir)/fakechroot.sh > $@
	chmod +x $@

ldd.fakechroot: $(srcdir)/ldd.pl
	$(do_subst) < $(srcdir)/ldd.pl > $@
	chmod +x $@
