#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 16

. $srcdir/touch.inc.sh

if [ -z "$touch" ]; then
    skip 16 "touch not found"
else

    for chroot in chroot fakechroot; do

        if [ $chroot = "chroot" ] && ! is_root; then
            skip $(( $tap_plan / 2 )) "not root"
        else

            t=`$srcdir/$chroot.sh $testtree $touch /tmp/$chroot-touch.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch" $t
            test -f $testtree/tmp/$chroot-touch.txt || not
            ok "$chroot $chroot-touch.txt exists"

            sleep 1

            t=`$srcdir/$chroot.sh $testtree $touch -r /tmp/$chroot-touch.txt /tmp/$chroot-touch2.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch -r" $t
            test -f $testtree/tmp/$chroot-touch2.txt || not
            ok "$chroot $chroot-touch2.txt exists"
            test $testtree/tmp/$chroot-touch2.txt -nt $testtree/tmp/$chroot-touch.txt && not
            ok "$chroot $chroot-touch2.txt is not newer than touch.txt"
            test $testtree/tmp/$chroot-touch2.txt -ot $testtree/tmp/$chroot-touch.txt && not
            ok "$chroot $chroot-touch2.txt is not older than $chroot-touch.txt"

            sleep 1

            t=`$srcdir/$chroot.sh $testtree $touch -m /tmp/$chroot-touch.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch -m" $t

            test $testtree/tmp/$chroot-touch.txt -nt $testtree/tmp/$chroot-touch2.txt || not
            ok "$chroot $chroot-touch.txt is newer than $chroot-touch2.txt"
        fi

    done

fi

cleanup
