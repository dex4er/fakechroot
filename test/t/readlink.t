#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh
. $srcdir/readlink.inc.sh

imax=$(( 180 - $(pwd | wc -c) ))

prepare $(( 4 * $imax ))

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        symlink=$chroot-
        destfile=$chroot-

        for i in `$SEQ 1 $imax`; do

            symlink="${symlink}a"
            destfile="${destfile}b"

            if ! touch "testtree/$destfile" 2>/dev/null; then
                skip 2 "File name too long"
            else

                rm -f "testtree/$symlink"

                t=`$srcdir/$chroot.sh testtree /bin/ln -s $destfile $symlink 2>&1`
                test "$t" = "" || not
                ok "$chroot ln -s [$i]" $t

                t=`$srcdir/$chroot.sh testtree $readlink $symlink 2>&1`
                test "$t" = "$destfile" || not
                ok "$chroot readlink [$i]" $t

            fi

        done

    fi

done

cleanup
