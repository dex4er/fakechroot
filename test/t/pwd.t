#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        t=`$srcdir/$chroot.sh testtree /bin/pwd`
        test "$t" = "/" || not
        ok "$chroot pwd is" $t

        t=`$srcdir/$chroot.sh testtree /bin/cat CHROOT`
        test "$t" = "testtree" || not
        ok "$chroot CHROOT is" $t

    fi

done

cleanup
