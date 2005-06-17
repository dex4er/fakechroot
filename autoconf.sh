#!/bin/bash

automake_version=1.7

rm -f aclocal.m4 configure
aclocal-${automake_version}
autoheader
libtoolize --copy
automake-${automake_version} --add-missing --copy
autoconf
pushd fake
    aclocal-${automake_version}
    automake-${automake_version} --add-missing --copy
    autoconf
popd

touch autoconf-stamp
