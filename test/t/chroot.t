#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 12

$srcdir/testtree.sh testtree/testtree2
test "`cat testtree/testtree2/CHROOT`" = "testtree/testtree2" || bail_out "testtree/testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for testtree in testtree2 /testtree2 ./testtree2 /./testtree2 testtree2/. testtree2/./.; do
            t=`$srcdir/$chroot.sh testtree chroot $testtree /bin/cat /CHROOT 2>&1`
            test "$t" = "testtree/testtree2" || not
            ok "$chroot chroot $testtree:" $t
        done
    fi

done

cleanup
