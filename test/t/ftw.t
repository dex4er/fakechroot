#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        mkdir -p $testtree/$chroot-dir/a/b/c
        echo "something" > $testtree/$chroot-dir/a/b/c/d

        t=`echo $($srcdir/$chroot.sh $testtree /bin/test-ftw /$chroot-dir 2>&1 | sort)`
        test "$t" = "/$chroot-dir /$chroot-dir/a /$chroot-dir/a/b /$chroot-dir/a/b/c /$chroot-dir/a/b/c/d" || not
        ok "$chroot ftw returns" $t

        t=`echo $($srcdir/$chroot.sh $testtree /bin/test-ftw $chroot-dir 2>&1 | sort)`
        test "$t" = "$chroot-dir $chroot-dir/a $chroot-dir/a/b $chroot-dir/a/b/c $chroot-dir/a/b/c/d" || not
        ok "$chroot ftw returns" $t

    fi

done

cleanup
