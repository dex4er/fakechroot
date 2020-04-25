#!/bin/sh

# unshare
#
# Replacement for unshare command that calls program directly
#
# (c) 2020 Gaël PORTAY <gael.portay@gmail.com>, LGPL

SHELL="${SHELL:-/bin/sh}"
while [ $# -gt 0 ]; do
    case "$1" in
        -*)
            ;;
        --)
            shift
            break
            ;;
        *)
            break
            ;;
    esac
    shift
done

exec "${@:-$SHELL}"
