sysconfdir = @sysconfdir@/@PACKAGE@

src_wrappers = chroot.fakechroot.sh env.fakechroot.sh fakechroot.sh ldd.fakechroot.pl unshare.fakechroot
src_envs = chroot.env.sh debootstrap.env.sh arch-chroot.env.sh pacstrap.env.sh rinse.env.sh
example_scripts = relocatesymlinks.sh restoremode.sh savemode.sh

bin_SCRIPTS = env.fakechroot fakechroot ldd.fakechroot unshare.fakechroot
sbin_SCRIPTS = chroot.fakechroot
sysconf_DATA = chroot.env debootstrap.env arch-chroot.env pacstrap.env rinse.env

EXTRA_DIST = $(src_wrappers) $(src_envs) $(example_scripts)
CLEANFILES = $(bin_SCRIPTS) $(sbin_SCRIPTS) $(sysconf_DATA)

do_subst = $(SED) -e 's,[@]bindir[@],$(bindir),g' \
               -e 's,[@]libpath[@],$(libpath),g' \
               -e 's,[@]sbindir[@],$(sbindir),g' \
               -e 's,[@]sysconfdir[@],$(sysconfdir),g' \
               -e 's,[@]CHROOT[@],$(CHROOT),g' \
               -e 's,[@]DEVFS[@],$(DEVFS),g' \
               -e 's,[@]ECHO[@],$(ECHO),g' \
               -e 's,[@]ENV[@],$(ENV),g' \
               -e 's,[@]EGREP[@],$(EGREP),g' \
               -e 's,[@]INSSERV[@],$(INSSERV),g' \
               -e 's,[@]ISCHROOT[@],$(ISCHROOT),g' \
               -e 's,[@]LDCONFIG[@],$(LDCONFIG),g' \
               -e 's,[@]LDD[@],$(LDD),g' \
               -e 's,[@]MKFIFO[@],$(MKFIFO),g' \
               -e 's,[@]MOUNT[@],$(MOUNT),g' \
               -e 's,[@]PERL[@],$(PERL),g' \
               -e 's,[@]SEQ[@],$(SEQ),g' \
               -e 's,[@]SHELL[@],$(SHELL),g' \
               -e 's,[@]SYSTEMCTL[@],$(SYSTEMCTL),g' \
               -e 's,[@]UMOUNT[@],$(UMOUNT),g' \
               -e 's,[@]UNSHARE[@],$(UNSHARE),g' \
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

arch-chroot.env: $(srcdir)/arch-chroot.env.sh
	$(do_subst) < $(srcdir)/arch-chroot.env.sh > $@
	chmod +x $@

pacstrap.env: $(srcdir)/pacstrap.env.sh
	$(do_subst) < $(srcdir)/pacstrap.env.sh > $@
	chmod +x $@

env.fakechroot: $(srcdir)/env.fakechroot.sh
	$(do_subst) < $(srcdir)/env.fakechroot.sh > $@
	chmod +x $@

fakechroot: $(srcdir)/fakechroot.sh
	$(do_subst) < $(srcdir)/fakechroot.sh > $@
	chmod +x $@

ldd.fakechroot: $(srcdir)/ldd.fakechroot.pl
	$(do_subst) < $(srcdir)/ldd.fakechroot.pl > $@
	chmod +x $@

unshare.fakechroot: $(srcdir)/unshare.fakechroot.sh
	$(do_subst) < $(srcdir)/unshare.fakechroot.sh > $@
	chmod +x $@

rinse.env: $(srcdir)/rinse.env.sh
	$(do_subst) < $(srcdir)/rinse.env.sh > $@
	chmod +x $@
