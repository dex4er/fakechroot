AUTOMAKE_OPTIONS=foreign

bin_SCRIPTS=fakechroot
EXTRA_DIST = fakechroot.in ldd.fake restoremode.sh savemode.sh
CLEANFILES=fakechroot

do_subst = sed -e 's,[@]prefix[@],$(prefix),g' \
	   -e 's,[@]bindir[@],$(bindir),g' \
	   -e 's,[@]libdir[@],$(libdir),g' \
	   -e 's,[@]pkglibdir[@],$(pkglibdir),g' \
	   -e 's,[@]libpath[@],$(libpath),g' \
	   -e 's,[@]fakechroot_transformed[@],'`echo fakechroot | sed -e '$(transform)'`',g' \
	   -e 's,[@]VERSION[@],$(VERSION),g' 

fakechroot: $(srcdir)/fakechroot.in Makefile
	$(do_subst) < $(srcdir)/fakechroot.in > fakechroot
	chmod +x fakechroot
