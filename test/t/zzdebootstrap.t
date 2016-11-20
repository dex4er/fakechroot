#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

top_srcdir=${top_srcdir:-..}
. $top_srcdir/config.sh

test -n "$TEST_DEBOOTSTRAP" && ! test "$TEST_DEBOOTSTRAP" = 0 || skip_all 'TEST_DEBOOTSTRAP is false'
command -v $DEBOOTSTRAP >/dev/null 2>&1 || skip_all 'debootstrap command is missing (sudo apt-get install debootstrap)'
command -v fakeroot     >/dev/null 2>&1 || skip_all 'fakeroot command is missing (sudo apt-get install fakeroot)'
command -v xzcat        >/dev/null 2>&1 || skip_all 'xzcat command is missing (sudo apt-get install xz-utils)'
command -v lsb_release  >/dev/null 2>&1 || skip_all 'lsb_release command is missing (sudo apt-get install lsb-release)'

if [ -n "$TEST_DEBOOTSTRAP_CACHE" ]; then
    DEBOOTSTRAP_CACHE=$TEST_DEBOOTSTRAP_CACHE
    export DEBOOTSTRAP_CACHE
fi

plan 1

unset FAKECHROOT_CMD_SUBST FAKECHROOT_DEBUG FAKECHROOT_EXCLUDE_PATH

rm -rf $testtree

chroot=fakechroot

./debootstrap.sh $testtree 2>&1 | diag

t=`LC_ALL=C $srcdir/$chroot.sh $testtree hello 2>&1`
test "$t" = "Hello, world!" || not
ok "$chroot hello returns" $t

cleanup
