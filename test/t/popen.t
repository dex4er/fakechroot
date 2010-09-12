#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

case "`uname -s`" in
    Linux|KFreeBSD)
        exe=exe;;
    *)
        exe=file;;
esac

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo 'something' > testtree/$chroot-file
        echo "cat /$chroot-file" > testtree/$chroot-popen.sh

        if [ ! -d /proc/1 ]; then
            skip 1 "/proc filesystem not found"
        else
            t=`($srcdir/$chroot.sh testtree /bin/test-popen 'echo $$ > PID; while :; do sleep 1; done' &); sleep 3; pid=$(cat testtree/PID 2>&1); readlink /proc/$pid/$exe 2>&1; kill $pid 2>/dev/null`
            test "$t" = "$fakedir/bin/sh" || not
            ok "$chroot popen shell is" $t
        fi

        t=`$srcdir/$chroot.sh testtree /bin/test-popen ". /$chroot-popen.sh" 2>&1`
        test "$t" = "something" || not
        ok "$chroot popen returns" $t

    fi

done

cleanup
