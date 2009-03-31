#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.sh

plan 6

rm -rf testtree

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

$srcdir/testtree.sh testtree/testtree
test "`cat testtree/testtree/CHROOT`" = "testtree/testtree" || not
ok "testtree/testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 2 "not root"
    else

        t=`$srcdir/$chroot.sh testtree /usr/sbin/chroot testtree /bin/cat /CHROOT`
        test "$t" = "testtree/testtree" || not
        ok "$chroot chroot testtree"

        t=`$srcdir/$chroot.sh testtree /usr/sbin/chroot /testtree /bin/cat /CHROOT`
        test "$t" = "testtree/testtree" || not
        ok "$chroot chroot /testtree"

    fi    

done

rm -rf testtree

end
