AUTOMAKE_OPTIONS=foreign
bin_SCRIPTS=fakeroot
EXTRA_DIST = fakeroot.in
CLEANFILES=fakeroot

do_subst = sed -e 's,[@]prefix[@],$(prefix),g' \
	   -e 's,[@]bindir[@],$(bindir),g' \
	   -e 's,[@]libdir[@],$(libdir),g' \
	   -e 's,[@]fakeroot_transformed[@],'`echo fakechroot | sed -e '$(transform)'`',g' \
	   -e 's,[@]faked_transformed[@],'`echo fakechrootd | sed -e '$(transform)'`',g' \
	   -e 's,[@]VERSION[@],$(VERSION),g' 

fakeroot: $(srcdir)/fakeroot.in Makefile
	$(do_subst) < $(srcdir)/fakeroot.in > fakeroot
	chmod +x fakeroot
