#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo 'something' > testtree/$chroot-file
        echo "cat /$chroot-file" > testtree/$chroot-popen.sh

        t=`( $srcdir/$chroot.sh testtree /bin/test-popen 'trap "exit 0" USR1; for i in $(seq 1 10240); do echo $$; done' | while read pid; do readlink /proc/$pid/exe; kill -USR1 $pid; exit; done ) 2>&1`
        test "$t" = "$fakedir/bin/sh" || not
        ok "$chroot popen shell is" $t

        t=`$srcdir/$chroot.sh testtree /bin/test-popen ". /$chroot-popen.sh" 2>&1`
        test "$t" = "something" || not
        ok "$chroot popen returns" $t

    fi

done

cleanup
