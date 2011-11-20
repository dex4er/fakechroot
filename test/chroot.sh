#!/bin/sh

srcdir=${srcdir:-.}

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    abs_srcdir=${abs_srcdir:-`cd $srcdir 2>/dev/null && pwd -P`}
    destdir="$abs_srcdir/testtree"
fi

if [ $# -gt 0 ]; then
    HOME=/root exec /usr/sbin/chroot $destdir "$@"
else
    HOME=/root exec /usr/sbin/chroot $destdir $SHELL
fi
