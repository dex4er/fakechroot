#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 8

$srcdir/testtree.sh testtree/testtree
test "`cat testtree/testtree/CHROOT`" = "testtree/testtree" || bail_out "testtree/testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for paths in '/ /' '/ .' '. /'; do
            t=`$srcdir/$chroot.sh testtree /bin/test-chroot / /testtree 'ls -id /' $paths 'ls -id /' 2>&1`
            f1=`echo $t | awk '{print $1}'`
            f2=`echo $t | awk '{print $2}'`
            f3=`echo $t | awk '{print $3}'`
            f4=`echo $t | awk '{print $4}'`
            test "$f2" = "/" -a "$f4" = "/" -a "$f1" = "$f3" || not
            ok "$chroot test-chroot $paths (not escaped):" $t
        done

        for paths in '. .'; do
            t=`$srcdir/$chroot.sh testtree /bin/test-chroot / /testtree 'ls -id /' $paths 'ls -id /' 2>&1`
            f1=`echo $t | awk '{print $1}'`
            f2=`echo $t | awk '{print $2}'`
            f3=`echo $t | awk '{print $3}'`
            f4=`echo $t | awk '{print $4}'`
            test "$f2" = "/" -a "$f4" = "/" -a "$f1" != "$f3" || not
            ok "$chroot test-chroot $paths (escaped):" $t
        done

    fi

done

cleanup
