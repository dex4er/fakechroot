#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 10

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for template in /tmp/${chroot}mkstempsXXXXXX tmp/${chroot}mkstempsXXXXXX; do
            match=`echo "$template" | tr 'X' '?'`
            t=`$srcdir/$chroot.sh $testtree /bin/test-mkstemps $template 0 2>&1`
            case "$t" in
                $match) ;;
                *) not
            esac
            ok "$chroot test-mkstemps $template returns" $t
        done

        for template in /tmp/${chroot}mkstempsXXXXXXaaa tmp/${chroot}mkstempsXXXXXXaaa; do
            match=`echo "$template" | tr 'X' '?'`
            t=`$srcdir/$chroot.sh $testtree /bin/test-mkstemps $template 3 2>&1`
            case "$t" in
                $match) ;;
                *) not
            esac
            ok "$chroot test-mkstemps $template returns" $t
        done

        t=`$srcdir/$chroot.sh $testtree /bin/test-mkstemps wrong-template 0 2>/dev/null`
        test -z "$t" || not
        ok "$chroot test-mkstemps wrong-template returns" $t

    fi

done

cleanup
