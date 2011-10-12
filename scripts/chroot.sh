#!/bin/sh

# chroot
#
# Wrapper for chroot command which sets additional LD_LIBRARY_PATH for fake
# chroot environment.  It copies original LD_LIBRARY_PATH and adds prefix to
# each directory for this variable.
#
# (c) 2011 Piotr Roszatycki <dexter@debian.org>, LGPL

if [ "$FAKECHROOT_CMD_SUBST" != "${FAKECHROOT_CMD_SUBST#/usr/sbin/chroot.real=}" ]; then
    chroot=/usr/sbin/chroot.real
else
    chroot=chroot
fi

for opt in "$@"; do
    case "$opt" in
        -*)
            continue
            ;;
        *)
            newroot="$1"
            break
            ;;
    esac
done

if [ -n "$newroot" ]; then
    paths=

    # append newroot to each directory from original LD_LIBRARY_PATH
    IFS_bak="$IFS" IFS=:
    for d in $LD_LIBRARY_PATH; do
        paths="${paths:+$paths:}$newroot/${d#/}"
    done
    IFS="$IFS_bak"

    # append newroot to each directory from new /etc/ld.so.conf
    # ...

    paths="$paths:$LD_LIBRARY_PATH"
fi

# echo "$paths"
LD_LIBRARY_PATH="$paths" exec $chroot "$@"
