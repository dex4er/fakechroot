#!/bin/sh

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

if [ $# -gt 0 ]; then
    HOME=/root chroot `pwd`/$destdir "$@"
else
    HOME=/root chroot `pwd`/$destdir /bin/bash
fi
