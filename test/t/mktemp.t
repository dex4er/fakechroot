#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 18

for chroot in chroot fakechroot; do

    for mktemp in mktemp mkstemp mkdtemp; do

        if [ $chroot = "chroot" ] && ! is_root; then
            skip $(( $tap_plan / 6 )) "not root"
        else

            for template in /tmp/${chroot}${mktemp}XXXXXX tmp/${chroot}${mktemp}XXXXXX; do
                match=`echo "$template" | tr 'X' '?'`
                t=`$srcdir/$chroot.sh $testtree /bin/test-$mktemp $template 2>&1`
                case "$t" in
                    $match) ;;
                    *) not
                esac
                ok "$chroot test-$mktemp $template returns" $t
            done

            t=`$srcdir/$chroot.sh $testtree /bin/test-$mktemp wrong-template 2>/dev/null`
            test -z "$t" || not
            ok "$chroot test-$mktemp wrong-template returns" $t

        fi

    done

done

cleanup
