SUBDIRS = src

TESTS = \
    t/00echo.t \
    t/canonicalize_file_name.t \
    t/chdir.t \
    t/chroot.t \
    t/clearenv.t \
    t/cmd-subst.t \
    t/cp.t \
    t/dedotdot.t \
    t/execlp.t \
    t/execve-elfloader.t \
    t/execve-null-envp.t \
    t/escape-nested-chroot.t \
    t/fts.t \
    t/ftw.t \
    t/host.t \
    t/mkstemps.t \
    t/mktemp.t \
    t/opendir.t \
    t/popen.t \
    t/pwd.t \
    t/readlink.t \
    t/realpath.t \
    t/socket-af_unix.t \
    t/statfs.t \
    t/statvfs.t \
    t/symlink.t \
    t/system.t \
    t/test-r.t \
    t/touch.t \
    t/zzarchlinux.t \
    t/zzdebootstrap.t \
    #

suffix =

CLEANFILES = .proverc

EXTRA_DIST = $(TESTS) \
    proverc \
    archlinux.sh \
    chroot.sh \
    common.inc.sh \
    debootstrap.sh \
    fakechroot.sh \
    seq.inc.sh \
    tap.inc.sh \
    testtree.sh \
    touch.inc.sh \
    bin/fakechroot \
    t/cmd-subst-pwd.sh \
    #

check-src:
	cd src && $(MAKE) $(AM_MAKEFLAGS) check

prove: check-src
	cp -f $(srcdir)/proverc .proverc
	srcdir=$(srcdir) SEQ=$(seq) $(PROVE) $(srcdir)/t

test: check-src
	if [ -n "$(PROVE)" ]; then \
	    $(MAKE) $(AM_MAKEFLAGS) prove; \
	else \
	    $(MAKE) $(AM_MAKEFLAGS) check-TESTS; \
	fi

.PHONY: check-src prove test
