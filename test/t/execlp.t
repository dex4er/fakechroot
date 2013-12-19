#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        t=`$srcdir/$chroot.sh $testtree /bin/test-execlp echo something 2>&1`
        test "$t" = "something" || not
        ok "$chroot execlp with echo returns" $t

        printf "#!/bin/sh\necho \$@\n" > $testtree/bin/test-echo
        chmod a+x $testtree/bin/test-echo

        t=`$srcdir/$chroot.sh $testtree /bin/test-execlp test-echo something 2>&1`
        test "$t" = "something" || not
        ok "$chroot execlp with test-echo returns" $t

    fi
done

cleanup
