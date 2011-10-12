sysconfdir = @sysconfdir@/@PACKAGE@

bin_SCRIPTS = fakechroot ldd.fakechroot
sbin_SCRIPTS = chroot.fakechroot
EXTRA_DIST = chroot.sh fakechroot.sh ldd.pl restoremode.sh savemode.sh
CLEANFILES = $(bin_SCRIPTS) $(sbin_SCRIPTS)

sysconf_DATA = chroot.env debootstrap.env

do_subst = sed -e 's,[@]libpath[@],$(libpath),g' \
               -e 's,[@]sbindir[@],$(sbindir),g' \
               -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
               -e 's,[@]VERSION[@],$(VERSION),g'

fakechroot: $(srcdir)/fakechroot.sh
	$(do_subst) < $(srcdir)/fakechroot.sh > $@
	chmod +x $@

chroot.fakechroot: $(srcdir)/chroot.sh
	$(do_subst) < $(srcdir)/chroot.sh > $@
	chmod +x $@

ldd.fakechroot: $(srcdir)/ldd.pl
	$(do_subst) < $(srcdir)/ldd.pl > $@
	chmod +x $@
