SUBDIRS = src

TESTS = \
    t/00echo.t \
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

check_SCRIPTS = $(TESTS) \
    chroot.sh \
    common.inc \
    fakechroot.sh \
    tap.inc \
    testtree.sh \
    t/cmd-subst-pwd.sh \
    #

EXTRA_DIST = $(check_SCRIPTS)

test:
	$(MAKE) $(AM_MAKEFLAGS) -C src check
@MK@ifneq (,$(PROVE))
	$(PROVE) t
@MK@else
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
@MK@endif
