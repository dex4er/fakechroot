#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 16

$srcdir/testtree.sh $testtree/testtree2
test "`cat $testtree/testtree2/CHROOT`" = "$testtree/testtree2" || bail_out "$testtree/testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for testtree2 in testtree2 /testtree2 ./testtree2 /./testtree2 testtree2/. testtree2/./. testtree2/ /testtree2/; do
            t=`$srcdir/$chroot.sh $testtree /usr/sbin/chroot $testtree2 /bin/cat /CHROOT 2>&1`
            test "$t" = "$testtree/testtree2" || not
            ok "$chroot chroot $testtree2:" $t
        done
    fi

done

cleanup
