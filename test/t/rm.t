#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 2

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        mkdir -p $testtree/dir-$chroot
        echo 'something' > $testtree/dir-$chroot/file

        $srcdir/$chroot.sh $testtree /bin/sh -c "rm -r /dir-$chroot"
        test -e $testtree/dir-$chroot && not
        ok "$chroot rm -r /dir-$chroot:" $t

    fi

done

cleanup
