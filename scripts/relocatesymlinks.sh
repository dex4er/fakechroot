#!/bin/sh

# This script adds all symlinks absolute path to fakechroot tree
#
# i.e.
#   before:
#     `/usr/bin/touch' -> `/bin/touch'
#   after:
#     `/usr/bin/touch' -> `/path/to/chroot/tree/bin/touch'

if [ -z "$1" ]; then
    echo "Usage: $0 /path/to/chroot/tree"
    exit 1
fi

root=`cd "$1"; pwd -P`

find "$root" -xdev -type l | while read linkname; do
    target=`readlink "$linkname"`
    case "$target" in
        /*)
            ln -vsf "$root$target" "$linkname"
            ;;
    esac;
done
