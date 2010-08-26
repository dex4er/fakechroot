#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 16

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for testtree in testtree ./testtree testtree/. testtree/./.; do
            t=`$srcdir/$chroot.sh $testtree /bin/pwd`
            test "$t" = "/" || not
            ok "$chroot $testtree pwd is" $t

            t=`$srcdir/$chroot.sh $testtree /bin/cat CHROOT`
            test "$t" = "testtree" || not
            ok "$chroot $testtree CHROOT is" $t
        done

    fi

done

cleanup
