#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

top_srcdir=${top_srcdir:-..}

. $top_srcdir/config.sh

prepare 8

$srcdir/testtree.sh $testtree/$testtree
test "`cat $testtree/$testtree/CHROOT`" = "$testtree/$testtree" || bail_out "$testtree/$testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for paths in '/ /' '/ .' '. /'; do
            t=`$srcdir/$chroot.sh $testtree /bin/test-chroot / /$testtree $paths 2>&1`
            f1=`echo $t | $AWK '{print $1}'`
            f2=`echo $t | $AWK '{print $2}'`
            f3=`echo $t | $AWK '{print $3}'`
            f4=`echo $t | $AWK '{print $4}'`
            test "$f2" = "/" -a "$f4" = "/" -a "$f1" = "$f3" || not
            ok "$chroot test-chroot $paths (not escaped):" $t
        done

        for paths in '. .'; do
            t=`$srcdir/$chroot.sh $testtree /bin/test-chroot / /$testtree $paths 2>&1`
            f1=`echo $t | $AWK '{print $1}'`
            f2=`echo $t | $AWK '{print $2}'`
            f3=`echo $t | $AWK '{print $3}'`
            f4=`echo $t | $AWK '{print $4}'`
            test "$f2" = "/" -a "$f4" = "/" -a "$f1" != "$f3" || not
            ok "$chroot test-chroot $paths (escaped):" $t
        done

    fi

done

cleanup
