#!/bin/sh

# chroot
#
# Wrapper for chroot command which sets additional LD_LIBRARY_PATH for fake
# chroot environment.  It copies original LD_LIBRARY_PATH and adds prefix to
# each directory for this variable.
#
# (c) 2011 Piotr Roszatycki <dexter@debian.org>, LGPL


load_ldsoconf () {
    file="$1"
    newroot="$2"

    sed -e 's/#.*//' -e '/^ *$/d' "$newroot$file" 2>/dev/null | while read line; do
        case "$line" in
            include*)
                include=`echo "$line" | sed -e 's/^include  *//' -e 's/ *$//'`
                for incfile in `eval echo $newroot$include`; do
                    incfile="${incfile#$newroot}"
                    load_ldsoconf "$incfile" "$newroot"
                done
                ;;
            *)
                echo "$newroot$line"
                ;;
        esac
    done
}

chroot="${FAKECHROOT_CMD_ORIG:-chroot}"
FAKECHROOT_CMD_ORIG=

base="$FAKECHROOT_BASE_ORIG"
unset FAKECHROOT_BASE_ORIG

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
        paths="${paths:+$paths:}$base$newroot/${d#/}"
    done
    IFS="$IFS_bak"

    # append newroot to each directory from new /etc/ld.so.conf
    paths_ldsoconf=`load_ldsoconf "/etc/ld.so.conf" "$newroot" | while read line; do printf ":%s%s" "$base" "$line"; done`
    paths_ldsoconf="${paths_ldsoconf#:}"

    paths="$paths${paths_ldsoconf:+:$paths_ldsoconf}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    paths="${paths#:}"
fi

# call real chroot
env LD_LIBRARY_PATH="$paths" FAKECHROOT_BASE="$base" "$chroot" "$@"
exit $?
