#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 8

test_af_unix () {
    n=$1

    $srcdir/$chroot.sh testtree /bin/test-socket-af_unix-server /tmp/$chroot-socket$n >testtree/tmp/$chroot-socket$n.log 2>&1 &
    server_pid=$!

    sleep 3
    test -S testtree/tmp/$chroot-socket$n || not
    ok "$chroot af_unix server socket created" $(cat testtree/tmp/$chroot-socket$n.log; ls testtree/tmp/$chroot-socket$n 2>&1)

    t=`$srcdir/$chroot.sh testtree /bin/test-socket-af_unix-client /tmp/$chroot-socket$n something 2>&1`
    test "$t" = "something" || not
    ok "$chroot af_unix client/server returns" $t

    kill $server_pid 2>/dev/null
}

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    elif [ "`pwd -P | wc -c`" -gt 75 ]; then
        skip $(( $tap_plan / 2 )) "current working directory is too long"
    else

        unset FAKECHROOT_AF_UNIX_PATH
        test_af_unix 1

        export FAKECHROOT_AF_UNIX_PATH=`pwd -P`/testtree
        test_af_unix 2

    fi

done

cleanup
