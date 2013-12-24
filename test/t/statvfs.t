#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 2

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        mkdir $testtree/testdir-$chroot
        t=`$srcdir/$chroot.sh $testtree test-statvfs /testdir-$chroot 2>&1`
        if echo "$t" | grep "[^0-9]" >/dev/null; then not; fi
        ok "$chroot statvfs returns" $t

    fi
done

cleanup
