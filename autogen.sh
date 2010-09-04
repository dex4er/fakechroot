#!/bin/sh

# Depends: libtool (>= 2.0), automake (>= 1.10), autoconf (>= 2.62)

set -e

autogen () {
    rm -f Makefile Makefile.in aclocal.m4 
    rm -f config.guess config.h config.h.in config.log
    rm -f config.status config.sub configure
    rm -f depcomp install-sh libtool ltmain.sh missing stamp-h1
    rm -f m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 m4/lt~obsolete.m4
    rm -rf autom4te.cache

    autoreconf --verbose --force --install --make
}

set -x
cd $(dirname $0)

autogen
