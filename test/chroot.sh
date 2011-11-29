#!/bin/sh

srcdir=${srcdir:-.}

pwd=`dirname $0`
abs_top_srcdir=${abs_top_srcdir:-`cd "$pwd/../.." 2>/dev/null && pwd -P`}

if ! command -v chroot >/dev/null; then
    PATH="${PATH:-/usr/bin:/bin}:/usr/sbin:/sbin"
    export PATH
fi

chroot=`command -v chroot 2>/dev/null`
chroot="${chroot:-/usr/sbin/chroot}"

FAKECHROOT_CMD_SUBST=$chroot=$abs_top_srcdir/scripts/chroot.fakechroot:$FAKECHROOT_CMD_SUBST
export FAKECHROOT_CMD_SUBST

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    abs_srcdir=${abs_srcdir:-`cd "$srcdir" 2>/dev/null && pwd -P`}
    destdir="$abs_srcdir/testtree"
fi

if [ $# -gt 0 ]; then
    HOME=/root $chroot $destdir "$@"
    result=$?
else
    HOME=/root $chroot $destdir $SHELL -i
    result=$?
fi

exit $result
