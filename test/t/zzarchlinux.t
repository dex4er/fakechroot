#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

test -n "$TEST_ARCHLINUX" && ! test "$TEST_ARCHLINUX" = 0 || skip_all 'TEST_ARCHLINUX is false'
command -v fakeroot    >/dev/null 2>&1 || skip_all 'fakeroot command is missing (sudo pacman -S fakeroot)'

if [ -n "$TEST_ARCHLINUX_CACHE" ]; then
    ARCHLINUX_CACHE=$TEST_ARCHLINUX_CACHE
    export ARCHLINUX_CACHE
fi

plan 1

unset FAKECHROOT_CMD_SUBST FAKECHROOT_DEBUG FAKECHROOT_EXCLUDE_PATH

rm -rf $testtree

chroot=fakechroot

./archlinux.sh $testtree 2>&1 | diag

t=`LC_ALL=C $srcdir/$chroot.sh $testtree hello 2>&1`
test "$t" = "Hello, world!" || not
ok "$chroot hello returns" $t

cleanup
