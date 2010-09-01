#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 2 "not root"
    else

        $srcdir/$chroot.sh testtree /bin/test-socket-af_unix-server /tmp/$chroot-socket >/dev/null 2>&1 &
        server_pid=$!

        sleep 3
        test -S testtree/tmp/$chroot-socket || not
        ok "$chroot af_unix server socket created"

        t=`$srcdir/$chroot.sh testtree /bin/test-socket-af_unix-client /tmp/$chroot-socket something 2>&1`
        test "$t" = "something" || not
        ok "$chroot af_unix client/server returns" $t

        kill $server_pid 2>/dev/null

    fi

done

cleanup
