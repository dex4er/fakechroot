AUTOMAKE_OPTIONS=foreign

TESTS = t.echoarg t.mknod t.tar t.truereturn t.falsereturn t.option \
	t.touchinstall

TESTS_ENVIRONMENT = libfakeroot=libfakechroot.so.0

CLEANFILES = tmp* tartest.tar.gz hda3

EXTRA_DIST = t.echoarg t.mknod t.tar t.truereturn t.falsereturn t.option \
	t.touchinstall tartest tartest.tar.gz.uue compare-tar
