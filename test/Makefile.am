SUBDIRS = src

TESTS = \
    t/00echo.t \
    t/chdir.t \
    t/chroot.t \
    t/cmd-subst.t \
    t/cp.t \
    t/escape-nested-chroot.t \
    t/popen.t \
    t/pwd.t \
    t/readlink.t \
    t/symlink.t \
    t/system.t \
    t/touch.t \
    #

suffix =

CLEANFILES = testtree

EXTRA_DIST = $(TESTS) \
    .proverc \
    chroot.sh \
    common.inc \
    debootstrap.sh \
    fakechroot.sh \
    tap.inc \
    testtree.sh \
    t/cmd-subst-pwd.sh \
    #

test:
	$(MAKE) $(AM_MAKEFLAGS) -C src check
@MK@ifneq (,$(PROVE))
	srcdir=$(srcdir) $(PROVE) $(srcdir)/t
@MK@else
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
@MK@endif
