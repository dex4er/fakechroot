#!/bin/sh

srcdir=${srcdir:-.}

. $srcdir/common.inc

imax=$((180-$(pwd | wc -c)))

plan $((3 + 4*$imax))

rm -rf testtree

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $((1+2*$imax)) "not root"
    else

        echo "something" > testtree/file
        t=`$srcdir/$chroot.sh testtree /bin/cat file 2>&1`
        test "$t" = "something" || not
        ok "$chroot file is" $t

        symlink=
        destfile=
        for i in `seq 1 $imax`; do

            symlink="a$symlink"
            destfile="b$destfile"

            rm -f "testtree/$symlink" "testtree/$destfile"
            t=`$srcdir/$chroot.sh testtree /bin/ln -s $destfile $symlink 2>&1`
            test "$t" = "" || not
            ok "$chroot ln -s [$i]" $t
            t=`$srcdir/$chroot.sh testtree /bin/readlink $symlink 2>&1`
            test "$t" = "$destfile" || not
            ok "$chroot readlink [$i]" ${t#$destfile}

        done

    fi

done

rm -rf testtree

end
