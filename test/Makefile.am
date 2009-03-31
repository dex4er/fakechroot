SUBDIRS = src

TESTS = \
    t/chroot.t \
    t/escape-nested-chroot.t \
    t/pwd.t \
    t/touch.t \
    #

suffix =

CLEANFILES = testtree

check_SCRIPTS = $(TESTS) chroot.sh common.sh fakechroot.sh tap.sh testtree.sh
EXTRA_DIST = $(check_SCRIPTS)
