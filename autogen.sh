#!/bin/sh

# Depends: libtool (>= 2.0), automake (>= 1.10), autoconf (>= 2.62)

set -e

clean () {
    rm -f Makefile Makefile.in aclocal.m4
    rm -f config.h config.h.in config.log
    rm -f config.status configure
    rm -f libtool test-driver
    rm -f stamp-h1
    rm -f m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 m4/lt~obsolete.m4
    rm -rf autom4te.cache
    rm -rf build-aux
}

autogen () {
    autoreconf --verbose --force --install --make
}

set -x
cd $(dirname $0)

clean
test "$1" = clean && exit 0

autogen
rm -rf autom4te.cache
