#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 6

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo "something" > $testtree/$chroot-test-r.txt

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "/usr/bin/test -r /$chroot-test-r.txt && echo true || echo false" 2>&1`
        test "$t" = "true" || not
        ok "$chroot /usr/bin/test -r /$chroot-test-r.txt is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "/usr/bin/test -r ../$chroot-test-r.txt && echo true || echo false" 2>&1`
        test "$t" = "true" || not
        ok "$chroot /usr/bin/test -r ../$chroot-test-r.txt is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "/usr/bin/test -r '' && echo true || echo false" 2>&1`
        test "$t" = "false" || not
        ok "$chroot /usr/bin/test -r '' is" $t
    fi

done

cleanup
