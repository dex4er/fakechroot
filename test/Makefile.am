SUBDIRS = src

TESTS = \
    t/00empty.t \
    t/chroot.t \
    t/cmd-subst.t \
    t/escape-nested-chroot.t \
    t/pwd.t \
    t/readlink.t \
    t/symlink.t \
    t/touch.t \
    #

suffix =

CLEANFILES = testtree

check_SCRIPTS = $(TESTS) \
    chroot.sh \
    common.sh \
    fakechroot.sh \
    tap.sh \
    testtree.sh \
    t/cmd-subst-pwd.sh \
    #

EXTRA_DIST = $(check_SCRIPTS)
