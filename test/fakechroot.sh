#!/bin/sh

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

if [ -f src/.libs/libfakechroot.so ]; then
    dir=`cd \`pwd\`/src/.libs 2>/dev/null && pwd`
elif [ -f ../src/.libs/libfakechroot.so ]; then
    dir=`cd \`pwd\`/../src/.libs 2>/dev/null && pwd`
else
    dir=/usr/lib/fakechroot
fi

if [ -n "$LD_PRELOAD" ]; then
    LD_PRELOAD="$LD_PRELOAD $dir/libfakechroot.so"
else
    LD_PRELOAD="$dir/libfakechroot.so"
fi
export LD_PRELOAD

if [ $# -gt 0 ]; then
    HOME=/root $destdir/usr/sbin/chroot `pwd -P`/$destdir "$@"
else
    echo "LD_PRELOAD=$LD_PRELOAD"
    HOME=/root $destdir/usr/sbin/chroot `pwd -P`/$destdir /bin/bash
fi
