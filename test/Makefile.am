SUBDIRS = src

TESTS = \
    t/00echo.t \
    t/canonicalize_file_name.t \
    t/chdir.t \
    t/chroot.t \
    t/cmd-subst.t \
    t/cp.t \
    t/execlp.t \
    t/escape-nested-chroot.t \
    t/fts.t \
    t/ftw.t \
    t/host.t \
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
    #

suffix =

CLEANFILES = .proverc

EXTRA_DIST = $(TESTS) \
    proverc \
    chroot.sh \
    common.inc \
    debootstrap.sh \
    fakechroot.sh \
    readlink.inc \
    rinse.sh \
    seq.inc \
    tap.inc \
    testtree.sh \
    touch.inc \
    bin/fakechroot \
    t/cmd-subst-pwd.sh \
    #

test:
	( cd src && $(MAKE) $(AM_MAKEFLAGS) check )
	if [ -n "$(PROVE)" ]; then \
	    cp -f $(srcdir)/proverc .proverc; \
	    srcdir=$(srcdir) SEQ=$(seq) $(PROVE) $(srcdir)/t; \
	else \
	    $(MAKE) $(AM_MAKEFLAGS) check-TESTS; \
	fi
