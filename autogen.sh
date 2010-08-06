#!/bin/bash

# Depends: libtool (>= 2.0), automake1.10

set -e

autogen () {
    automake_version=1.10

    rm -f Makefile Makefile.in aclocal.m4 
    rm -f config.guess config.h config.h.in config.log
    rm -f config.status config.sub configure
    rm -f depcomp install-sh libtool ltmain.sh missing stamp-h1
    rm -rf autom4te.cache

    aclocal-${automake_version} "$@"
    autoheader
    libtoolize --force --copy
    automake-${automake_version} --add-missing --copy
    autoconf

    rm -rf autom4te.cache
    rm -f config.h.in~
}

set -x
cd $(dirname $0)
autogen -I m4
pushd fake
    autogen
popd

pushd man
    ./makeman.sh
popd
