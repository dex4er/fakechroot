#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

plan 5

rm -rf testtree

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 2 "not root"
    else

        t=`$srcdir/$chroot.sh testtree /bin/pwd`
        test "$t" = "/" || not
        ok "$chroot pwd is /"

        t=`$srcdir/$chroot.sh testtree /bin/cat CHROOT`
        test "$t" = "testtree" || not
        ok "$chroot CHROOT is testtree"

    fi    

done

rm -rf testtree

end
