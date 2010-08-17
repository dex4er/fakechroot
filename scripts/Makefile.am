bin_SCRIPTS = fakechroot ldd.fakechroot
EXTRA_DIST = fakechroot.sh ldd.pl restoremode.sh savemode.sh
CLEANFILES = fakechroot ldd.fakechroot

do_subst = sed -e 's,[@]prefix[@],$(prefix),g' \
	       -e 's,[@]bindir[@],$(bindir),g' \
	       -e 's,[@]libdir[@],$(libdir),g' \
	       -e 's,[@]pkglibdir[@],$(pkglibdir),g' \
	       -e 's,[@]libpath[@],$(libpath),g' \
	       -e 's,[@]fakechroot_transformed[@],'`echo fakechroot | sed -e '$(transform)'`',g' \
	       -e 's,[@]VERSION[@],$(VERSION),g'

fakechroot: $(srcdir)/fakechroot.sh
	$(do_subst) < $< > $@
	chmod +x $@

ldd.fakechroot: ldd.pl
	cp $< $@
	chmod +x $@
