#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        t=`$srcdir/$chroot.sh $testtree test-posix_spawnp /bin/test-hello world 2>&1`
        test "$t" = "Hello, world!" || not
        ok "$chroot posix_spawnp returns" $t

        t=`$srcdir/$chroot.sh $testtree test-posix_spawnp test-hello world 2>&1`
        test "$t" = "Hello, world!" || not
        ok "$chroot posix_spawnp returns" $t

    fi
done

cleanup
