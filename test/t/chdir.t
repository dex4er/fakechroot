#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 8

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for d in / .. /tmp/..; do
            t=`$srcdir/$chroot.sh testtree /bin/sh -c "cd $d && ls CHROOT"  2>&1`
            test "$t" = "CHROOT" || not
            ok "$chroot cd $d:" $t
        done

        for d in '$FAKECHROOT_BASE'; do
            t=`$srcdir/$chroot.sh testtree /bin/sh -c "cd $d && ls CHROOT"  2>&1`
            test "$t" != "CHROOT" || not
            ok "$chroot not cd $d:" $t
        done

    fi

done

cleanup

