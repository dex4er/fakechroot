bin_SCRIPTS = fakechroot ldconfig.fakechroot ldd.fakechroot
EXTRA_DIST = fakechroot.sh ldconfig.sh ldd.pl restoremode.sh savemode.sh
CLEANFILES = $(bin_SCRIPTS)

do_subst = sed -e 's,[@]prefix[@],$(prefix),g' \
	       -e 's,[@]bindir[@],$(bindir),g' \
	       -e 's,[@]libdir[@],$(libdir),g' \
	       -e 's,[@]pkglibdir[@],$(pkglibdir),g' \
	       -e 's,[@]libpath[@],$(libpath),g' \
	       -e 's,[@]fakechroot_transformed[@],'`echo fakechroot | sed -e '$(transform)'`',g' \
	       -e 's,[@]VERSION[@],$(VERSION),g'

fakechroot: $(srcdir)/fakechroot.sh
	$(do_subst) < $(srcdir)/fakechroot.sh > fakechroot
	chmod +x $@

ldd.fakechroot: $(srcdir)/ldd.pl
	cp $(srcdir)/ldd.pl ldd.fakechroot
	chmod +x ldd.fakechroot

ldconfig.fakechroot: $(srcdir)/ldconfig.sh
	cp $(srcdir)/ldconfig.sh ldconfig.fakechroot
	chmod +x ldconfig.fakechroot
