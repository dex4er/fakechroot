AUTOMAKE_OPTIONS=foreign

TESTS = t.echoarg t.mknod t.tar t.truereturn t.falsereturn t.option \
	t.touchinstall

suffix =
TESTS_ENVIRONMENT = libfakeroot=libfakechroot.so.0 suffix=$(suffix)

CLEANFILES = tmp* tartest.tar.gz hda3

check_SCRIPTS = $(TESTS) echo_arg tartest compare-tar
EXTRA_DIST = $(check_SCRIPTS) tartest.tar.gz.uue
