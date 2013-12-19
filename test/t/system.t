#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 2

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo 'something' > $testtree/$chroot-file
        echo "cat /$chroot-file" > $testtree/$chroot-system.sh

        t=`$srcdir/$chroot.sh $testtree /bin/test-system ". /$chroot-system.sh" 2>&1`
        test "$t" = "something" || not
        ok "$chroot system returns" $t

    fi

done

cleanup
