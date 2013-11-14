#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

test -n "$TEST_DEBOOTSTRAP" && ! test "$TEST_DEBOOTSTRAP" = 0 || skip_all 'TEST_DEBOOTSTRAP is false'
command -v debootstrap >/dev/null 2>&1 || skip_all 'debootstrap command is missing (sudo apt-get install debootstrap)'
command -v lsb_release >/dev/null 2>&1 || skip_all 'lsb_release command is missing (sudo apt-get install lsb-release)'

plan 1

unset FAKECHROOT_CMD_SUBST FAKECHROOT_DEBUG FAKECHROOT_EXCLUDE_PATH

rm -rf testtree

chroot=fakechroot

./debootstrap.sh 2>&1 | diag

t=`LC_ALL=C $srcdir/$chroot.sh testtree hello 2>&1`
test "$t" = "Hello, world!" || not
ok "$chroot hello returns" $t

cleanup
