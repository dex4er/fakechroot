#!/bin/sh

srcdir=${srcdir:-.}

. $srcdir/common.inc

plan 5

rm -rf testtree

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 2 "not root"
    else

        echo 'something' > testtree/file
        echo 'cat /file' > testtree/system.sh

        t=`( $srcdir/$chroot.sh testtree /bin/test-system 'trap "exit 0" USR1; for i in $(seq 1 10240); do echo $$; done' | while read pid; do readlink /proc/$pid/exe; kill -USR1 $pid; exit; done ) 2>&1`
        test "$t" = "$fakedir/bin/sh" || not
        ok "$chroot system shell is" $t

        t=`$srcdir/$chroot.sh testtree /bin/test-system '. /system.sh' 2>&1`
        test "$t" = "something" || not
        ok "$chroot system returns" $t

    fi

done

rm -rf testtree

end
