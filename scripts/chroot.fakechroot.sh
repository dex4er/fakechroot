#!/bin/sh

# chroot
#
# Wrapper for chroot command which sets additional LD_LIBRARY_PATH for fake
# chroot environment.  It copies original LD_LIBRARY_PATH and adds prefix to
# each directory for this variable.
#
# (c) 2011, 2013 Piotr Roszatycki <dexter@debian.org>, LGPL


fakechroot_chroot_load_ldsoconf () {
    fakechroot_chroot_files="$1"
    fakechroot_chroot_newroot="$2"

    for fakechroot_chroot_file in `eval echo $fakechroot_chroot_newroot$fakechroot_chroot_files`; do
        fakechroot_chroot_file="${fakechroot_chroot_file#$fakechroot_chroot_newroot}"

        sed -e 's/#.*//' -e '/^ *$/d' "$fakechroot_chroot_newroot$fakechroot_chroot_file" 2>/dev/null | while read fakechroot_chroot_line; do
            case "$fakechroot_chroot_line" in
                include*)
                    include=`echo "$fakechroot_chroot_line" | sed -e 's/^include  *//' -e 's/ *$//'`
                    for fakechroot_chroot_incfile in `eval echo $fakechroot_chroot_newroot$fakechroot_chroot_include`; do
                        fakechroot_chroot_incfile="${fakechroot_chroot_incfile#$fakechroot_chroot_newroot}"
                        fakechroot_chroot_load_ldsoconf "$fakechroot_chroot_incfile" "$fakechroot_chroot_newroot"
                    done
                    ;;
                *)
                    echo "$fakechroot_chroot_newroot$fakechroot_chroot_line"
                    ;;
            esac
        done

    done
}

fakechroot_chroot_chroot="${FAKECHROOT_CMD_ORIG:-chroot}"

fakechroot_chroot_base="$FAKECHROOT_BASE_ORIG"

for fakechroot_chroot_opt in "$@"; do
    case "$fakechroot_chroot_opt" in
        -*)
            continue
            ;;
        *)
            fakechroot_chroot_newroot="$1"
            break
            ;;
    esac
done

if [ -n "$fakechroot_chroot_newroot" ]; then
    fakechroot_chroot_paths=

    # append newroot to each directory from original LD_LIBRARY_PATH
    fakechroot_chroot_IFS_bak="$IFS" IFS=:
    for fakechroot_chroot_d in $LD_LIBRARY_PATH; do
        fakechroot_chroot_paths="${fakechroot_chroot_paths:+$fakechroot_chroot_paths:}$fakechroot_chroot_base$fakechroot_chroot_newroot/${fakechroot_chroot_d#/}"
    done
    IFS="$fakechroot_chroot_IFS_bak"

    # append newroot to each directory from new /etc/ld.so.conf
    fakechroot_chroot_paths_ldsoconf=""
    if [ -f "$fakechroot_chroot_newroot/etc/ld.so.conf" ]; then
        fakechroot_chroot_paths_ldsoconf=`fakechroot_chroot_load_ldsoconf "/etc/ld.so.conf" "$fakechroot_chroot_newroot" | while read fakechroot_chroot_line; do printf ":%s%s" "$fakechroot_chroot_base" "$fakechroot_chroot_line"; done`
    elif [ -d "$fakechroot_chroot_newroot/etc/ld.so.conf.d" ]; then
        fakechroot_chroot_paths_ldsoconf=`fakechroot_chroot_load_ldsoconf "/etc/ld.so.conf.d/*" "$fakechroot_chroot_newroot" | while read fakechroot_chroot_line; do printf ":%s%s" "$fakechroot_chroot_base" "$fakechroot_chroot_line"; done`
    fi
    fakechroot_chroot_paths_ldsoconf="${fakechroot_chroot_paths_ldsoconf#:}"

    fakechroot_chroot_paths="$fakechroot_chroot_paths${fakechroot_chroot_paths_ldsoconf:+:$fakechroot_chroot_paths_ldsoconf}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fakechroot_chroot_paths="${fakechroot_chroot_paths#:}"
fi

# call real chroot
env -u FAKECHROOT_BASE_ORIG FAKECHROOT_CMD_ORIG= LD_LIBRARY_PATH="$fakechroot_chroot_paths" FAKECHROOT_BASE="$fakechroot_chroot_base" "$fakechroot_chroot_chroot" "$@"
exit $?
