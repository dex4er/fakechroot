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
               [-e|--environment type] [-c|--config-dir directory]
               [--] [command]
    fakechroot -v|--version
    fakechroot -h|--help"
}


if [ "$FAKECHROOT" = "true" ]; then
    die "fakechroot: nested operation is not supported"
fi


# Default settings
lib=libfakechroot.so
paths=@libpath@
sysconfdir=@sysconfdir@
confdir=
environment=


# Get options
getopttest=`getopt --version`
case $getopttest in
    getopt*)
        # GNU getopt
        opts=`getopt -q -l lib: -l use-system-libs -l config-dir: -l environment -l version -l help -- +l:sc:e:vh "$@"`
        ;;
    *)
        # POSIX getopt ?
        opts=`getopt l:sc:e:vh "$@"`
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
        -c|--config-dir)
            confdir=$1
            shift
            ;;
        -e|--environment)
            environment=$1
            shift
            ;;
        --)
            break
            ;;
    esac
done

if [ -z "$environment" ]; then
    if [ "$1" = "fakeroot" ]; then
        environment=`basename "$2"`
    else
        environment=`basename "$1"`
    fi
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
export FAKECHROOT FAKECHROOT_VERSION


# Additional environment setting from configuration file
for e in $environment default; do
    for d in $confdir $HOME/.fakechroot $sysconfdir; do
        f=$d/$environment.env
        if [ -f $f ]; then
            . $f
            break 2
        fi
    done
done


# Execute command
if [ -z "$*" ]; then
    exec ${SHELL:-/bin/sh}
else
    exec "$@"
fi

die "fakechroot: cannot execute: $@"
