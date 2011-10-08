AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = man src scripts test

EXTRA_DIST = LICENSE NEWS README THANKS autogen.sh makedist.sh

test: all
	( cd test && $(MAKE) $(AM_MAKEFLAGS) test )

.PHONY: test
