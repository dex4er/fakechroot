#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

export FAKECHROOT_EXCLUDE_PATH=${FAKECHROOT_EXCLUDE_PATH:-/dev:/proc}
$srcdir/bin/fakechroot $srcdir/chroot.sh "$@"
