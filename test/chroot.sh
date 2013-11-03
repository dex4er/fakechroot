#!/bin/sh

srcdir=${srcdir:-.}

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    abs_srcdir=${abs_srcdir:-`cd "$srcdir" 2>/dev/null && pwd -P`}
    destdir="$abs_srcdir/testtree"
fi

if [ $# -gt 0 ]; then
    HOME=/root chroot $destdir "$@"
    result=$?
else
    HOME=/root chroot $destdir $SHELL -i
    result=$?
fi

exit $result
