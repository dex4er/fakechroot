#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.sh

plan 17

rm -rf testtree

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

if [ -x testtree/usr/bin/touch ]; then
    touch=/usr/bin/touch
elif [ -x testtree/bin/touch ]; then
    touch=/bin/touch
else
    touch=
fi

if [ -z "$touch" ]; then
    skip 16 "touch not found"
else

    for chroot in chroot fakechroot; do

        if [ $chroot = "chroot" ] && ! is_root; then
            skip 8 "not root"
        else

            t=`$srcdir/$chroot.sh testtree $touch /tmp/touch.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch"
            test -f testtree/tmp/touch.txt || not
            ok "$chroot touch.txt exists"

            sleep 1

            t=`$srcdir/$chroot.sh testtree $touch -r /tmp/touch.txt /tmp/touch2.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch -r"
            test -f testtree/tmp/touch2.txt || not
            ok "$chroot touch2.txt exists"
            test testtree/tmp/touch2.txt -nt testtree/tmp/touch.txt && not
            ok "$chroot touch2.txt is not newer than touch.txt"
            test testtree/tmp/touch2.txt -ot testtree/tmp/touch.txt && not
            ok "$chroot touch2.txt is not older than touch.txt"

            sleep 1

            t=`$srcdir/$chroot.sh testtree $touch -m /tmp/touch.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch -m"

            test testtree/tmp/touch.txt -nt testtree/tmp/touch2.txt || not
            ok "$chroot touch.txt is newer than touch2.txt"
        fi    

    done

fi

rm -rf testtree

end
