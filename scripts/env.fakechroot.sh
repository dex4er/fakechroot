#!/bin/sh

# env
#
# Replacement for env command which preserves fakechroot enviroment even with
# --ignore-environment option.
#
# (c) 2013 Piotr Roszatycki <dexter@debian.org>, LGPL


__env_args=
__env_extra_args=
__env_fakechroot_base=${FAKECHROOT_BASE_ORIG:-$FAKECHROOT_BASE}
__env_fakechroot_env=
__env_ignore_env=no
__env_unset_path=no
__env_null=no
__env_path=$PATH


help () {
    cat << END
Usage: env [OPTION]... [-] [NAME=VALUE]... [COMMAND [ARG]...]
Set each NAME to VALUE in the environment and run COMMAND.

  -i, --ignore-environment  start with an empty environment
  -0, --null           end each output line with 0 byte rather than newline
  -u, --unset=NAME     remove variable from the environment
      --help     display this help and exit
      --version  output version information and exit

A mere - implies -i.  If no COMMAND, print the resulting environment.
END
    exit 0
}


while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            help
            ;;
        -i|--ignore-environment)
            __env_ignore_env=yes
            shift
            ;;
        -0|--null)
            __env_null=yes
            shift
            ;;
        --unset=*)
            __env_key=${1#--unset=}
            case "$__env_key" in
                FAKEROOTKEY|FAKED_MODE|FAKECHROOT|FAKECHROOT_*|LD_LIBRARY_PATH|LD_PRELOAD) ;;
                *) unset $__env_key
            esac
            shift
            ;;
        -u|--unset)
            __env_key=$2
            case "$__env_key" in
                FAKEROOTKEY|FAKED_MODE|FAKECHROOT|FAKECHROOT_*|LD_LIBRARY_PATH|LD_PRELOAD) ;;
                *) unset $__env_key
            esac
            shift 2
            ;;
        -)
            __env_ignore_env=yes
            shift
            break
            ;;
        *)
            break
    esac
done


if [ $# -eq 0 ]; then
    export | while read line; do
        __env_key="${line#declare -x }"
        __env_key="${__env_key#export }"
        __env_key="${__env_key%%=*}"
        printf "%s=" "$__env_key"
        eval printf "%s" '$'$__env_key
        if [ $__env_null = yes ]; then
            printf "\0"
        else
            printf "\n"
        fi
    done
else
    if [ $__env_null = yes ]; then
        echo 'env: cannot specify --null (-0) with command' 1>&2
        exit 125
    fi

    if [ $__env_ignore_env = yes ]; then
        __env_keys=`export | while read line; do
            __env_key="${line#declare -x }"
            __env_key="${__env_key#export }"
            __env_key="${__env_key%%=*}"
            case "$__env_key" in
                FAKEROOTKEY|FAKED_MODE|FAKECHROOT|FAKECHROOT_*|LD_LIBRARY_PATH|LD_PRELOAD) ;;
                *) echo $__env_key
            esac
        done`
        for __env_key in $__env_keys; do
            unset $__env_key
        done
    fi

    while [ $# -gt 1 ]; do
        case "$1" in
            *=*)
                __env_key=${1%%=*}
                eval $1
                eval export $__env_key
                shift
                ;;
            *)
                break
        esac
    done

    __env_cmd=`PATH=$__env_path command -v $1 2>/dev/null`
    __env_cmd=${__env_cmd:-$1}
    shift

    $__env_cmd "$@"
    exit $?
fi

exit 0
