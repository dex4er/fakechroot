#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 2

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo 'something' > $testtree/$chroot-file
        echo "cat /$chroot-file" > $testtree/$chroot-popen.sh

        t=`$srcdir/$chroot.sh $testtree /bin/test-popen ". /$chroot-popen.sh" 2>&1`
        test "$t" = "something" || not
        ok "$chroot popen returns" $t

    fi

done

cleanup
