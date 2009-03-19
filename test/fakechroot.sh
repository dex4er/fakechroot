#!/bin/sh

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

dir=`cd \`pwd\`/../src/.libs 2>/dev/null && pwd`
test -n "$dir" || dir=/usr/lib/fakechroot

#if [ -n "$LD_LIBRARY_PATH" ]; then
#    LD_LIBRARY_PATH="$dir:$LD_LIBRARY_PATH"
#else
#    LD_LIBRARY_PATH="$dir"
#fi
#export LD_LIBRARY_PATH

if [ -n "$LD_PRELOAD" ]; then
    LD_PRELOAD="$LD_PRELOAD libfakechroot.so"
else
    LD_PRELOAD="$dir/libfakechroot.so"
fi
export LD_PRELOAD

if [ $# -gt 0 ]; then
    HOME=/root $destdir/usr/sbin/chroot `pwd`/$destdir "$@"
else
    echo "LD_PRELOAD=$LD_PRELOAD"
    HOME=/root $destdir/usr/sbin/chroot `pwd`/$destdir /bin/bash
fi
