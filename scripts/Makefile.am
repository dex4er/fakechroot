sysconfdir = @sysconfdir@/@PACKAGE@

src_wrappers = chroot.fakechroot.sh fakechroot.sh ldd.fakechroot.pl
src_envs = chroot.env.sh debootstrap.env.sh rinse.env.sh
example_scripts = relocatesymlinks.sh restoremode.sh savemode.sh

bin_SCRIPTS = fakechroot ldd.fakechroot
sbin_SCRIPTS = chroot.fakechroot
sysconf_DATA = chroot.env debootstrap.env rinse.env

EXTRA_DIST = $(src_wrappers) $(src_envs) $(example_scripts)
CLEANFILES = $(bin_SCRIPTS) $(sbin_SCRIPTS) $(sysconf_DATA)

do_subst = sed -e 's,[@]bindir[@],$(bindir),g' \
               -e 's,[@]libpath[@],$(libpath),g' \
               -e 's,[@]sbindir[@],$(sbindir),g' \
               -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
               -e 's,[@]CHROOT[@],$(CHROOT),g' \
               -e 's,[@]VERSION[@],$(VERSION),g'

chroot.env: $(srcdir)/chroot.env.sh
	$(do_subst) < $(srcdir)/chroot.env.sh > $@
	chmod +x $@

chroot.fakechroot: $(srcdir)/chroot.fakechroot.sh
	$(do_subst) < $(srcdir)/chroot.fakechroot.sh > $@
	chmod +x $@

debootstrap.env: $(srcdir)/debootstrap.env.sh
	$(do_subst) < $(srcdir)/debootstrap.env.sh > $@
	chmod +x $@

fakechroot: $(srcdir)/fakechroot.sh
	$(do_subst) < $(srcdir)/fakechroot.sh > $@
	chmod +x $@

ldd.fakechroot: $(srcdir)/ldd.fakechroot.pl
	$(do_subst) < $(srcdir)/ldd.fakechroot.pl > $@
	chmod +x $@

rinse.env: $(srcdir)/rinse.env.sh
	$(do_subst) < $(srcdir)/rinse.env.sh > $@
	chmod +x $@
