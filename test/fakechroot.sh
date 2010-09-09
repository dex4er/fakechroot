#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare_env

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

if [ $# -gt 0 ]; then
    HOME=/root $destdir/usr/sbin/chroot `pwd -P`/$destdir "$@"
else
    echo "LD_PRELOAD=$LD_PRELOAD"
    HOME=/root $destdir/usr/sbin/chroot `pwd -P`/$destdir $SHELL
fi
