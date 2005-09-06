#!/bin/bash

automake_version=1.9

rm -f aclocal.m4 configure

aclocal-${automake_version}
autoheader
libtoolize --force --copy
automake-${automake_version} --add-missing --copy
autoconf
rm -rf autom4te.cache
