#!/bin/sh

srcdir=${srcdir:-.}
FAKECHROOT="${FAKECHROOT:-$srcdir/bin/fakechroot}"

"$FAKECHROOT" "$srcdir/chroot.sh" "$@"
exit $?
