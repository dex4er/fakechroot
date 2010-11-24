#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 2

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        mkdir testtree/$chroot-dir
        mkdir testtree/$chroot-dir/subdir
        echo "something" > testtree/$chroot-dir/file

        t=`$srcdir/$chroot.sh testtree /bin/test-ftw /$chroot-dir 2>&1 | sort | xargs echo`
        test "$t" = "/$chroot-dir /$chroot-dir/file /$chroot-dir/subdir" || not
        ok "$chroot ftw returns" $t

    fi

done

#cleanup
