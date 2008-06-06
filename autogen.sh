#!/bin/bash

autogen () {
    automake_version=1.10

    rm -f aclocal.m4 configure

    aclocal-${automake_version} -I m4
    autoheader
    libtoolize --force --copy
    automake-${automake_version} --add-missing --copy
    autoconf
    rm -rf autom4te.cache
    rm -f config.h.in~
}

cd $(dirname $0)
autogen
cd fake
autogen
