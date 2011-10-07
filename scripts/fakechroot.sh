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
    fakechroot [-l|--lib fakechrootlib] [-s|--use-system-libs] [--] [command]
    fakechroot -v|--version
    fakechroot -h|--help"
}


if [ "x$FAKECHROOT" = "xtrue" ]; then
    die "fakechroot: nested operation is not supported"
fi


# Default location of preloaded library
lib=libfakechroot.so
paths=@libpath@


# Get options
getopttest=`getopt --version`
case $getopttest in
getopt*)
    # GNU getopt
    opts=`getopt -l lib: -l use-system-libs -l version -l help -- +l:svh "$@"`
    ;;
*)
    # POSIX getopt ?
    opts=`getopt l:svh "$@"`
    ;;
esac

if test "$?" -ne 0; then
    usage
fi

eval set -- "$opts"

while test "x$1" != "x--"; do
    case "$1" in
        -l|--lib)
            shift
            lib=`eval echo "$1"`
            paths=
            ;;
        -v|--version)
            echo "fakechroot version $FAKECHROOT_VERSION"
            exit 0
            ;;
        -s|--use-system-libs)
            paths="$PATHS:/usr/lib:/lib"
            ;;
        -h|--help)
            usage
            ;;
    esac
    shift
done

shift


# Make sure the preload is available
libfound=no

if [ -n "$paths" ]
then
    new_paths=
    for dir in `echo $paths | sed 's/:/ /g'`
    do
        dir=`eval echo $dir`
        new_paths="${new_paths:+$new_paths:}$dir"
        if test -r "$dir/$lib"
        then
            libfound=yes
        fi
    done
    paths=$new_paths
else
    if test -r "$lib"
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

if test -z "$*"; then
    exec ${SHELL:-/bin/sh}
else
    exec "$@"
fi

die "fakechroot: cannot execute: $@"
