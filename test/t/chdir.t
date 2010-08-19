#!/bin/sh

srcdir=${srcdir:-.}

. $srcdir/common.inc

plan 9

rm -rf testtree

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 4 "not root"
    else

        t=`$srcdir/fakechroot.sh testtree /bin/sh -c 'cd / && ls CHROOT'  2>&1`
        test "$t" = "CHROOT" || not
        ok "$chroot cd /:" $t

        t=`$srcdir/fakechroot.sh testtree /bin/sh -c 'cd .. && ls CHROOT'  2>&1`
        test "$t" = "CHROOT" || not
        ok "$chroot cd ..:" $t

        t=`$srcdir/fakechroot.sh testtree /bin/sh -c 'cd /tmp/.. && ls CHROOT'  2>&1`
        test "$t" = "CHROOT" || not
        ok "$chroot cd /tmp/..:" $t

        t=`$srcdir/fakechroot.sh testtree /bin/sh -c 'cd $FAKECHROOT_BASE && ls CHROOT' 2>&1`
        test "$t" != "CHROOT" || not
        ok "$chroot cd $(pwd)/testtree:" $t

    fi

done

rm -rf testtree

end
