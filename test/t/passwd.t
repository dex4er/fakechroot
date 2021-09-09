#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

for chroot in chroot fakechroot; do
    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        t=`$srcdir/$chroot.sh $testtree /bin/test-passwd user 2>&1`
        test "$t" = "1337" || not
        ok "$chroot uid is" $t

        t=`$srcdir/$chroot.sh $testtree getent group user 2>&1`
        test "$t" = "user:x:1337:" || not
        ok "$chroot getent group user is" $t
    fi
done

cleanup
