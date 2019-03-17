#!@SHELL@

# fakechroot
#
# Script which sets fake chroot environment
#
# (c) 2011, 2013 Piotr Roszatycki <dexter@debian.org>, LGPL


FAKECHROOT_VERSION=@VERSION@


fakechroot_die () {
    echo "$@" 1>&2
    exit 1
}


fakechroot_usage () {
    fakechroot_die "Usage:
    fakechroot [-l|--lib fakechrootlib]
               [-d|--elfloader ldso]
               [-s|--use-system-libs]
               [-e|--environment type]
               [-c|--config-dir directory]
               [-b|--bindir directory]
               [--] [command]
    fakechroot -v|--version
    fakechroot -h|--help"
}


fakechroot_next_cmd () {
    if [ "$1" = "fakeroot" ]; then
        shift
        # skip the options
        while [ $# -gt 0 ]; do
            case "$1" in
                -h|-v)
                    break
                    ;;
                -u|--unknown-is-real)
                    shift
                    ;;
                -l|--lib|--faked|-s|-i|-b)
                    shift 2
                    ;;
                --)
                    shift
                    break
                    ;;
                *)
                    break
                    ;;
            esac
        done
    fi

    if [ -n "$1" -a "$1" != "-v" -a "$1" != "-h" ]; then
        fakechroot_environment=`basename -- "$1"`
    fi
}


if [ "$FAKECHROOT" = "true" ]; then
    fakechroot_die "fakechroot: nested operation is not supported"
fi


# fakechroot doesn't work with CDPATH correctly
unset CDPATH


# Default settings
fakechroot_lib=libfakechroot.so
fakechroot_paths=@libpath@
fakechroot_sysconfdir=@sysconfdir@
fakechroot_confdir=
fakechroot_environment=
fakechroot_bindir=

if [ "$fakechroot_paths" = "no" ]; then
    fakechroot_paths=
fi

if command -v which >/dev/null; then
    fakechroot_echo=`which echo`
    fakechroot_echo=${fakechroot_echo:-@ECHO@}
else
    fakechroot_echo=@ECHO@
fi


# Get options
fakechroot_getopttest=`getopt --version`
case $fakechroot_getopttest in
    getopt*)
        # GNU getopt
        fakechroot_opts=`getopt -q -l lib: -l elfloader: -l use-system-libs -l config-dir: -l environment: -l bindir: -l version -l help -- +l:d:sc:e:b:vh "$@"`
        ;;
    *)
        # POSIX getopt ?
        fakechroot_opts=`getopt l:d:sc:e:b:vh "$@"`
        ;;
esac

if [ "$?" -ne 0 ]; then
    fakechroot_usage
fi

eval set -- "$fakechroot_opts"

while [ $# -gt 0 ]; do
    fakechroot_opt=$1
    shift
    case "$fakechroot_opt" in
        -h|--help)
            fakechroot_usage
            ;;
        -v|--version)
            echo "fakechroot version $FAKECHROOT_VERSION"
            exit 0
            ;;
        -l|--lib)
            fakechroot_lib=`eval echo "$1"`
            fakechroot_paths=
            shift
            ;;
        -d|--elfloader)
            FAKECHROOT_ELFLOADER=$1
            export FAKECHROOT_ELFLOADER
            shift
            ;;
        -s|--use-system-libs)
            fakechroot_paths="${fakechroot_paths:+$fakechroot_paths:}/usr/lib:/lib"
            ;;
        -c|--config-dir)
            fakechroot_confdir=$1
            shift
            ;;
        -e|--environment)
            fakechroot_environment=$1
            shift
            ;;
        -b|--bindir)
            fakechroot_bindir=$1
            shift
            ;;
        --)
            break
            ;;
    esac
done

if [ -z "$fakechroot_environment" ]; then
    fakechroot_next_cmd "$@"
fi


# Autodetect if dynamic linker supports --argv0 option
if [ -n "$FAKECHROOT_ELFLOADER" ]; then
    fakechroot_detect=`$FAKECHROOT_ELFLOADER --argv0 echo $fakechroot_echo yes 2>&1`
    if [ "$fakechroot_detect" = yes ]; then
        FAKECHROOT_ELFLOADER_OPT_ARGV0="--argv0"
        export FAKECHROOT_ELFLOADER_OPT_ARGV0
    fi
fi


# Swap libfakechroot and libfakeroot in LD_PRELOAD if needed
# libfakeroot must come first
# an alternate fakeroot library may be given
# in the FAKEROOT_ALT_LIB environment variable
if [ -n "$FAKEROOT_ALT_LIB" ]; then
    lib_libfakeroot="$FAKEROOT_ALT_LIB"
else
    lib_libfakeroot="libfakeroot-sysv.so"
fi

for preload in $(echo "$LD_PRELOAD" | tr ':' ' '); do
    case "$preload" in
        "$lib_libfakeroot")
            lib_libfakeroot_to_preload="$preload"
            ;;
        *)
            lib_to_preload="${lib_to_preload:+${lib_to_preload}:}$preload"
            ;;
    esac
done


# Make sure the preload is available
fakechroot_paths="$fakechroot_paths${LD_LIBRARY_PATH:+${fakechroot_paths:+:}$LD_LIBRARY_PATH}"
fakechroot_lib="${lib_libfakeroot_to_preload:+${lib_libfakeroot_to_preload}:}$fakechroot_lib${lib_to_preload:+:$lib_to_preload}"

fakechroot_detect=`LD_LIBRARY_PATH="$fakechroot_paths" LD_PRELOAD="$fakechroot_lib" FAKECHROOT_DETECT=1 $fakechroot_echo 2>&1`
case "$fakechroot_detect" in
    fakechroot*)
        fakechroot_libfound=yes
        ;;
    *)
        fakechroot_libfound=no
esac

if [ $fakechroot_libfound = no ]; then
    fakechroot_die "fakechroot: preload library not found, aborting."
fi


# Additional environment setting from configuration file
if [ "$fakechroot_environment" != "none" ]; then
    for fakechroot_e in "$fakechroot_environment" "${fakechroot_environment%.*}" default; do
        for fakechroot_d in "$fakechroot_confdir" "$HOME/.fakechroot" "$fakechroot_sysconfdir"; do
            fakechroot_f="$fakechroot_d/$fakechroot_e.env"
            if [ -f "$fakechroot_f" ]; then
                . "$fakechroot_f"
                break 2
            fi
        done
    done
fi


# Check if substituted command is called
fakechroot_cmd=`command -v "$1"`

fakechroot_cmd_wrapper=`
    IFS=:
    for fakechroot_cmd_subst in $FAKECHROOT_CMD_SUBST; do
        case "$fakechroot_cmd_subst" in
            "$fakechroot_cmd="*)
                echo "${fakechroot_cmd_subst#*=}"
                break 2
                ;;
        esac
    done
`

# Set FAKECHROOT_CMD_ORIG if wrapped
if [ -n "$fakechroot_cmd_wrapper" ]; then
    FAKECHROOT_CMD_ORIG="$fakechroot_cmd"
    export FAKECHROOT_CMD_ORIG
fi

fakechroot_cmd=${fakechroot_cmd_wrapper:-$1}


# Execute command
if [ -z "$*" ]; then
    LD_LIBRARY_PATH="$fakechroot_paths" LD_PRELOAD="$fakechroot_lib" ${SHELL:-/bin/sh}
    exit $?
else
    if [ -n "$fakechroot_cmd" ]; then
        # Call substituted command
        shift
        LD_LIBRARY_PATH="$fakechroot_paths" LD_PRELOAD="$fakechroot_lib" "$fakechroot_cmd" "$@"
        exit $?
    else
        # Call original command
        LD_LIBRARY_PATH="$fakechroot_paths" LD_PRELOAD="$fakechroot_lib" "$@"
        exit $?
    fi
fi
