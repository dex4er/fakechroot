#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

case "`uname -s`" in
    Linux|GNU/kFreeBSD)
        exe=exe;;
    *)
        exe=file;;
esac

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 2 "not root"
    else

        echo 'something' > testtree/$chroot-file
        echo "cat /$chroot-file" > testtree/$chroot-system.sh

        if [ ! -d /proc/1 ]; then
            skip 1 "/proc filesystem not found"
        else
            t=`($srcdir/$chroot.sh testtree /bin/test-system 'echo $$ > PID; while :; do sleep 1; done' &); sleep 3; pid=$(cat testtree/PID 2>&1); readlink /proc/$pid/$exe 2>&1; kill $pid 2>/dev/null`
            test "$t" = "$fakedir/bin/sh" || not
            ok "$chroot system shell is" $t
        fi

        t=`$srcdir/$chroot.sh testtree /bin/test-system ". /$chroot-system.sh" 2>&1`
        test "$t" = "something" || not
        ok "$chroot system returns" $t

    fi

done

cleanup
