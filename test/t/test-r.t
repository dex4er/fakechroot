#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 2

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo "something" > testtree/$chroot-test-r.txt

        # /bin/dash uses stat64(2) and /bin/bash uses faccessat(2)

        t=`$srcdir/$chroot.sh testtree ${SHELL:-/bin/sh} -c "test -r /$chroot-test-r.txt && echo ok || echo not ok" 2>&1`
        test "$t" = "ok" || not
        ok "$chroot test -r is" $t
    fi

done

cleanup
