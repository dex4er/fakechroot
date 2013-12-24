#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

abs_srcdir=${abs_srcdir:-`cd "$pwd" 2>/dev/null && pwd -P`}

prepare 10

test_af_unix () {
    n=$1

    $srcdir/$chroot.sh $testtree /bin/test-socket-af_unix-server /$chroot-socket$n >$testtree/$chroot-socket$n.log 2>&1 &
    server_pid=$!

    sleep 3
    test -S "${FAKECHROOT_AF_UNIX_PATH:-$testtree}/$chroot-socket$n" || not
    ok "$chroot af_unix server socket created" `cat $testtree/$chroot-socket$n.log; ls "${FAKECHROOT_AF_UNIX_PATH:-$testtree}/$chroot-socket$n" 2>&1`

    t=`$srcdir/$chroot.sh $testtree /bin/test-socket-af_unix-client /$chroot-socket$n something 2>&1`
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

        if [ $chroot = "chroot" ]; then
            skip 3 "test only for fakechroot"
        else

            tmpdir=`src/test-mkdtemp /tmp/$chroot-socketXXXXXX 2>&1`
            test -d "$tmpdir" || not
            ok "mkdtemp /tmp/$chroot-socketXXXXXX returns $tmpdir"

            export FAKECHROOT_AF_UNIX_PATH="$tmpdir"
            test_af_unix 2

            test -e "$tmpdir/$chroot-socket$n" && rm -rf "$tmpdir/$chroot-socket$n"
            test -d "$tmpdir" && rmdir "$tmpdir"

        fi

    fi

done

cleanup
