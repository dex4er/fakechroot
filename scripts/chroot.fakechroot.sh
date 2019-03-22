#!@SHELL@

# chroot
#
# Wrapper for chroot command which sets additional LD_LIBRARY_PATH for fake
# chroot environment.  It copies original LD_LIBRARY_PATH and adds prefix to
# each directory for this variable.
#
# (c) 2011, 2013, 2016 Piotr Roszatycki <dexter@debian.org>, LGPL


fakechroot_chroot_load_ldsoconf () {
    fakechroot_chroot_files="$1"
    fakechroot_chroot_newroot="$2"

    for fakechroot_chroot_file in `eval echo $fakechroot_chroot_newroot$fakechroot_chroot_files`; do
        fakechroot_chroot_file="${fakechroot_chroot_file#$fakechroot_chroot_newroot}"

        sed -e 's/#.*//' -e '/^ *$/d' "$fakechroot_chroot_newroot$fakechroot_chroot_file" 2>/dev/null | while read fakechroot_chroot_line; do
            case "$fakechroot_chroot_line" in
                include*)
                    fakechroot_chroot_include=`echo "$fakechroot_chroot_line" | sed -e 's/^include  *//' -e 's/ *$//'`
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

# records the content of LD_LIBRARY_PATH at first chroot invocation
if [ -z "$fakechroot_chroot_base" -a -n "$LD_LIBRARY_PATH" ]; then
    FAKECHROOT_LDLIBPATH="$LD_LIBRARY_PATH"
    export FAKECHROOT_LDLIBPATH
fi

fakechroot_chroot_n=0
for fakechroot_chroot_opt in "$@"; do
    fakechroot_chroot_n=$(($fakechroot_chroot_n + 1))
    case "$fakechroot_chroot_opt" in
        -*)
            continue
            ;;
        *)
            fakechroot_chroot_requested_newroot="$fakechroot_chroot_opt"
            break
            ;;
    esac
done

# absolute paths in fakechroot_chroot_opt are relative to FAKECHROOT_BASE_ORIG
if [ "${fakechroot_chroot_requested_newroot#/}" != "$fakechroot_chroot_requested_newroot" ]; then
    fakechroot_chroot_newroot="${fakechroot_chroot_base}${fakechroot_chroot_requested_newroot}"
else
    fakechroot_chroot_newroot="$fakechroot_chroot_requested_newroot"
fi

if [ -d "$fakechroot_chroot_newroot" ]; then
    fakechroot_chroot_newroot=`cd "$fakechroot_chroot_newroot"; pwd -P`

    fakechroot_chroot_paths=

    # append newroot to each directory from original LD_LIBRARY_PATH
    fakechroot_chroot_IFS_bak="$IFS" IFS=:
    for fakechroot_chroot_d in $FAKECHROOT_LDLIBPATH; do
        fakechroot_chroot_paths="${fakechroot_chroot_paths:+$fakechroot_chroot_paths:}$fakechroot_chroot_newroot/${fakechroot_chroot_d#/}"
    done
    IFS="$fakechroot_chroot_IFS_bak"

    # append newroot to each directory from new /etc/ld.so.conf
    fakechroot_chroot_paths_ldsoconf=""
    if [ -f "$fakechroot_chroot_newroot/etc/ld.so.conf" ]; then
        fakechroot_chroot_paths_ldsoconf=`fakechroot_chroot_load_ldsoconf "/etc/ld.so.conf" "$fakechroot_chroot_newroot" | while read fakechroot_chroot_line; do printf ":%s" "$fakechroot_chroot_line"; done`
    elif [ -d "$fakechroot_chroot_newroot/etc/ld.so.conf.d" ]; then
        fakechroot_chroot_paths_ldsoconf=`fakechroot_chroot_load_ldsoconf "/etc/ld.so.conf.d/*" "$fakechroot_chroot_newroot" | while read fakechroot_chroot_line; do printf ":%s" "$fakechroot_chroot_line"; done`
    fi
    fakechroot_chroot_paths_ldsoconf="${fakechroot_chroot_paths_ldsoconf#:}"

    # append newroot to extra directories because some important commands use runpath
    fakechroot_chroot_IFS_bak="$IFS" IFS=:
    fakechroot_chroot_paths_extra=""
    for fakechroot_chroot_d in ${FAKECHROOT_EXTRA_LIBRARY_PATH:-/lib/systemd:/usr/lib/man-db}; do
        if [ -d "$fakechroot_chroot_newroot$fakechroot_chroot_d" ]; then
            fakechroot_chroot_paths_extra="$fakechroot_chroot_paths_extra${fakechroot_chroot_paths_extra:+:}$fakechroot_chroot_newroot$fakechroot_chroot_d"
        fi
    done
    IFS="$fakechroot_chroot_IFS_bak"

    fakechroot_chroot_paths="$fakechroot_chroot_paths${fakechroot_chroot_paths_ldsoconf:+:$fakechroot_chroot_paths_ldsoconf}${fakechroot_chroot_paths_extra:+:$fakechroot_chroot_paths_extra}${FAKECHROOT_LDLIBPATH:+:$FAKECHROOT_LDLIBPATH}"
    fakechroot_chroot_paths="${fakechroot_chroot_paths#:}"
fi

# correct newroot if we chroot into the root
if [ -n "$FAKECHROOT_BASE_ORIG" -a "$fakechroot_chroot_newroot" = "$FAKECHROOT_BASE_ORIG" ]; then
    fakechroot_chroot_final_newroot="/"
else
    fakechroot_chroot_final_newroot="${fakechroot_chroot_newroot#$FAKECHROOT_BASE_ORIG}"
fi

# call real chroot
if [ -n "$fakechroot_chroot_newroot" ] && [ $fakechroot_chroot_n -le $# ]; then
    if ( test "$1" = "${@:1:$((1+0))}" ) 2>/dev/null; then
        # shell with arrays and built-in expr
        env -u FAKECHROOT_BASE_ORIG FAKECHROOT_CMD_ORIG= LD_LIBRARY_PATH="$fakechroot_chroot_paths" FAKECHROOT_BASE="$fakechroot_chroot_base" \
            "$fakechroot_chroot_chroot" "${@:1:$(($fakechroot_chroot_n - 1))}" "$fakechroot_chroot_final_newroot" "${@:$(($fakechroot_chroot_n + 1))}"
        exit $?
    else
        # POSIX shell
        fakechroot_chroot_args=$#
        for fakechroot_chroot_i in `@SEQ@ 1 $fakechroot_chroot_args`; do
            if [ $fakechroot_chroot_i = $fakechroot_chroot_n ]; then
                fakechroot_chroot_arg="$fakechroot_chroot_final_newroot"
                eval fakechroot_chroot_arg_$fakechroot_chroot_i=\"\$fakechroot_chroot_arg\"
            else
                eval fakechroot_chroot_arg_$fakechroot_chroot_i=\"\$1\"
            fi
            shift
        done

        set --

        for fakechroot_chroot_i in `@SEQ@ 1 $fakechroot_chroot_args`; do
            eval fakechroot_chroot_arg=\"\$fakechroot_chroot_arg_$fakechroot_chroot_i\"
            set -- "$@" "$fakechroot_chroot_arg"
        done

        env -u FAKECHROOT_BASE_ORIG FAKECHROOT_CMD_ORIG= LD_LIBRARY_PATH="$fakechroot_chroot_paths" FAKECHROOT_BASE="$fakechroot_chroot_base" \
            "$fakechroot_chroot_chroot" "$@"
        exit $?
    fi
else
    # original arguments
    env -u FAKECHROOT_BASE_ORIG FAKECHROOT_CMD_ORIG= LD_LIBRARY_PATH="$fakechroot_chroot_paths" FAKECHROOT_BASE="$fakechroot_chroot_base" \
        "$fakechroot_chroot_chroot" "$@"
    exit $?
fi
