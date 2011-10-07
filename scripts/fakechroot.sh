#!/bin/sh

# fakechroot
#
# Script which sets fake chroot environment
#
# (c) 2011 Piotr Roszatycki <dexter@debian.org>, LGPL


FAKECHROOT_VERSION=@VERSION@


die () {
    echo "$@" 1>&2
    exit 1
}


usage () {
    die "Usage:
    fakechroot [-l|--lib fakechrootlib] [-s|--use-system-libs]
               [-e|--environment type] [--] [command]
    fakechroot -v|--version
    fakechroot -h|--help

Environment types:
    clean
    chroot
    debootstrap"
}


if [ "x$FAKECHROOT" = "xtrue" ]; then
    die "fakechroot: nested operation is not supported"
fi


# Default settings
lib=libfakechroot.so
paths=@libpath@
environment=default


# Get options
getopttest=`getopt --version`
case $getopttest in
    getopt*)
        # GNU getopt
        opts=`getopt -q -l lib: -l use-system-libs -l environment -l version -l help -- +l:se:vh "$@"`
        ;;
    *)
        # POSIX getopt ?
        opts=`getopt l:se:vh "$@"`
        ;;
esac

if [ "$?" -ne 0 ]; then
    usage
fi

eval set -- "$opts"

while [ $# -gt 0 ]; do
    opt=$1
    shift
    case "$opt" in
        -h|--help)
            usage
            ;;
        -v|--version)
            echo "fakechroot version $FAKECHROOT_VERSION"
            exit 0
            ;;
        -l|--lib)
            lib=`eval echo "$1"`
            paths=
            shift
            ;;
        -s|--use-system-libs)
            paths="${paths:+$paths:}/usr/lib:/lib"
            ;;
        -e|--environment)
            case "$1" in
                clean|chroot|debootstrap)
                    environment=$1
                    ;;
                *)
                    die "fakechroot: unknown environment: $1"
                    ;;
            esac
            shift
            ;;
        --)
            break
            ;;
    esac
done

if [ "x$environment" = xdefault ]; then
    cmd=`basename "$1"`
    case "$cmd" in
        chroot|debootstrap)
            environment=$cmd
            ;;
        *)
            environment=clean
            ;;
    esac
fi


# Make sure the preload is available
libfound=no

if [ -n "$paths" ]
then
    new_paths=
    for dir in `echo $paths | sed 's/:/ /g'`
    do
        dir=`eval echo $dir`
        new_paths="${new_paths:+$new_paths:}$dir"
        if [ -r "$dir/$lib" ]
        then
            libfound=yes
        fi
    done
    paths=$new_paths
else
    if [ -r "$lib" ]
    then
        libfound=yes
    fi
fi

if [ $libfound = no ]
then
    die "fakechroot: preload library not found, aborting."
fi


# Set new environment
FAKECHROOT=true
LD_LIBRARY_PATH="$paths${LD_LIBRARY_PATH:+${paths:+:}$LD_LIBRARY_PATH}"
LD_PRELOAD="$lib${LD_PRELOAD:+ $LD_PRELOAD}"
export FAKECHROOT FAKECHROOT_VERSION LD_LIBRARY_PATH LD_PRELOAD


if [ -z "$*" ]; then
    exec ${SHELL:-/bin/sh}
else
    exec "$@"
fi

die "fakechroot: cannot execute: $@"
