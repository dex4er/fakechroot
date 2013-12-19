#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        mkdir -p $testtree/$chroot-dir
        for file in 1 2 3 4; do
            echo $file > $testtree/$chroot-dir/$file
        done

        t=`echo $($srcdir/$chroot.sh $testtree /bin/test-opendir $chroot-dir 2>&1 | sort)`
        test "$t" = ". .. 1 2 3 4" || not
        ok "$chroot opendir for $chroot-dir returns" $t

        t=`echo $($srcdir/$chroot.sh $testtree /bin/test-opendir /$chroot-dir 2>&1 | sort)`
        test "$t" = ". .. 1 2 3 4" || not
        ok "$chroot opendir for /$chroot-dir returns" $t

    fi

done

cleanup
