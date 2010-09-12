#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

$srcdir/bin/fakechroot $srcdir/chroot.sh "$@"
