#!/bin/sh

srcdir=${srcdir:-.}

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=$srcdir/testtree
fi

if [ $# -gt 0 ]; then
    HOME=/root exec /usr/sbin/chroot $destdir "$@"
else
    HOME=/root exec /usr/sbin/chroot $destdir $SHELL
fi
