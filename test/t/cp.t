#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo 'something' > testtree/file-$chroot
        ln -s /file-$chroot testtree/symlink-$chroot

        t=`$srcdir/$chroot.sh testtree /bin/sh -c "cp -dp /file-$chroot /file2-$chroot; cat /file2-$chroot" 2>&1`
        test "$t" = "something" || not
        ok "$chroot cp -dp /file-$chroot /file2-$chroot:" $t

        t=`$srcdir/$chroot.sh testtree /bin/sh -c "cp -dp /symlink-$chroot /symlink2-$chroot; cat /symlink2-$chroot" 2>&1`
        test "$t" = "something" || not
        ok "$chroot cp -dp /symlink-$chroot /symlink2-$chroot:" $t

    fi

done

cleanup
