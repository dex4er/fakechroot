#!/bin/sh

# env
#
# Wrapper for env command which preserves fakechroot enviroment even with
# --ignore-environment option.
#
# (c) 2013 Piotr Roszatycki <dexter@debian.org>, LGPL


args=
extra_args=
fakechroot_base=${FAKECHROOT_BASE_ORIG:-$FAKECHROOT_BASE}
fakechroot_env=
ignore_env=no

while [ $# -gt 0 ]; do
    case "$1" in
        -i|--ignore-environment)
            ignore_env=yes
            shift
            ;;
        -0|--null|--unset=*)
            args="$args $1"
            shift
            ;;
        -u|--unset)
            args="$args $1 $2"
            shift 2
            ;;
        -)
            ignore_env=yes
            shift
            break
            ;;
        *)
            break
    esac
done


unset FAKECHROOT_BASE FAKECHROOT_BASE_ORIG FAKECHROOT_CMD_ORIG

if [ $ignore_env = yes ]; then
    args="$args -"
    fakechroot_env=`export | sed 's/^export //; s/^declare -x //' | @EGREP@ "^(FAKE|LD_LIBRARY_PATH=|LD_PRELOAD=)"`
fi

if [ -n "$fakechroot_base" ]; then
    fakechroot_env="$fakechroot_env FAKECHROOT_BASE='$fakechroot_base'"
fi

while [ $# -gt 0 ]; do
    extra_args="$extra_args '$1'"
    shift
done

eval @ENV@ $args $fakechroot_env $extra_args
exit $?
