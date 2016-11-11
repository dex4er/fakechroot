#!/bin/sh

# env
#
# Replacement for env command which preserves fakechroot environment even with
# --ignore-environment option.
#
# (c) 2013, 2015 Piotr Roszatycki <dexter@debian.org>, LGPL


fakechroot_env_args=
fakechroot_env_extra_args=
fakechroot_env_fakechroot_base=${FAKECHROOT_BASE_ORIG:-$FAKECHROOT_BASE}
fakechroot_env_fakechroot_env=
fakechroot_env_ignore_env=no
fakechroot_env_unset_path=no
fakechroot_env_null=no
fakechroot_env_path=$PATH


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
            fakechroot_env_ignore_env=yes
            shift
            ;;
        -0|--null)
            fakechroot_env_null=yes
            shift
            ;;
        --unset=*)
            fakechroot_env_key=${1#--unset=}
            case "$fakechroot_env_key" in
                LD_LIBRARY_PATH|LD_PRELOAD) ;;
                *) unset $fakechroot_env_key
            esac
            shift
            ;;
        -u|--unset)
            fakechroot_env_key=$2
            case "$fakechroot_env_key" in
                LD_LIBRARY_PATH|LD_PRELOAD) ;;
                *) unset $fakechroot_env_key
            esac
            shift 2
            ;;
        -)
            fakechroot_env_ignore_env=yes
            shift
            break
            ;;
        *)
            break
    esac
done


if [ $# -eq 0 ]; then
    export | while read line; do
        if [ "$line" = "${line#declare -x }" ] && [ "$line" = "${line#export }" ]; then
            continue
        fi
        fakechroot_env_key="${line#declare -x }"
        fakechroot_env_key="${fakechroot_env_key#export }"
        fakechroot_env_key="${fakechroot_env_key%%=*}"
        printf "%s=" "$fakechroot_env_key"
        eval printf "%s" '$'$fakechroot_env_key
        if [ $fakechroot_env_null = yes ]; then
            printf "\0"
        else
            printf "\n"
        fi
    done
else
    if [ $fakechroot_env_null = yes ]; then
        echo 'env: cannot specify --null (-0) with command' 1>&2
        exit 125
    fi

    if [ $fakechroot_env_ignore_env = yes ]; then
        fakechroot_env_keys=`export | while read line; do
            if [ "$line" = "${line#declare -x }" ] && [ "$line" = "${line#export }" ]; then
                continue
            fi
            fakechroot_env_key="${line#declare -x }"
            fakechroot_env_key="${fakechroot_env_key#export }"
            fakechroot_env_key="${fakechroot_env_key%%=*}"
            case "$fakechroot_env_key" in
                FAKEROOTKEY|FAKED_MODE|FAKECHROOT|FAKECHROOT_*|LD_LIBRARY_PATH|LD_PRELOAD) ;;
                *) echo $fakechroot_env_key
            esac
        done`
        for fakechroot_env_key in $fakechroot_env_keys; do
            unset $fakechroot_env_key 2>/dev/null || true
        done
    fi

    while [ $# -gt 1 ]; do
        case "$1" in
            *=*)
                fakechroot_env_key=${1%%=*}
                eval $1
                eval export $fakechroot_env_key
                shift
                ;;
            *)
                break
        esac
    done

    fakechroot_env_cmd=`PATH=$fakechroot_env_path command -v $1 2>/dev/null`
    fakechroot_env_cmd=${fakechroot_env_cmd:-$1}
    shift

    $fakechroot_env_cmd "$@"
    exit $?
fi

exit 0
