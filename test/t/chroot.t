#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 12

$srcdir/testtree.sh testtree/testtree
test "`cat testtree/testtree/CHROOT`" = "testtree/testtree" || bail_out "testtree/testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for testtree in testtree /testtree ./testtree /./testtree testtree/. testtree/./.; do
            t=`$srcdir/$chroot.sh testtree /usr/sbin/chroot $testtree /bin/cat /CHROOT`
            test "$t" = "testtree/testtree" || not
            ok "$chroot chroot $testtree:" $t
        done
    fi

done

cleanup
