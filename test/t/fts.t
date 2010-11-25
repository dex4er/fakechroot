#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 40

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for option in 0 1 2 4 8 16 32 64 128 255; do

            mkdir -p testtree/$chroot-$option-dir/a/b/c
            echo "something" > testtree/$chroot-$option-dir/a/b/c/d

            t=`echo $($srcdir/$chroot.sh testtree /bin/test-fts $option /$chroot-$option-dir 2>&1 | sort)`
            test "$t" = "/$chroot-$option-dir /$chroot-$option-dir/a /$chroot-$option-dir/a/b /$chroot-$option-dir/a/b/c /$chroot-$option-dir/a/b/c/d" || not
            ok "$chroot fts with option $option returns" $t

            t=`echo $($srcdir/$chroot.sh testtree /bin/test-fts $option $chroot-$option-dir 2>&1 | sort)`
            test "$t" = "$chroot-$option-dir $chroot-$option-dir/a $chroot-$option-dir/a/b $chroot-$option-dir/a/b/c $chroot-$option-dir/a/b/c/d" || not
            ok "$chroot fts with option $option returns" $t

        done

    fi

done

cleanup
