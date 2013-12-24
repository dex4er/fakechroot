#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 16

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for d in $testtree ./$testtree $testtree/. $testtree/./.; do
            t=`$srcdir/$chroot.sh $d /bin/pwd`
            test "$t" = "/" || not
            ok "$chroot $d pwd is" $t

            t=`$srcdir/$chroot.sh $d /bin/cat CHROOT`
            test "$t" = "$testtree" || not
            ok "$chroot $d CHROOT is" $t
        done

    fi

done

cleanup
