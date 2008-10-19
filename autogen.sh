#!/bin/bash

# Depends: libtool (>= 2.0), automake1.10

autogen () {
    automake_version=1.10

    rm -f aclocal.m4 configure

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

if [ -x /usr/bin/yada ]; then
    yada rebuild
    rm -f debian/packages-tmp*
fi
