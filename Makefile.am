AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = man src scripts test

EXTRA_DIST = COPYING LICENSE NEWS.md README.md THANKS autogen.sh makedist.sh

test: all
	( cd test && $(MAKE) $(AM_MAKEFLAGS) test )

.PHONY: test
