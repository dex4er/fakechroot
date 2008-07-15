#!/bin/sh

if [ ! -x configure ] || [ ! -x fake/configure ]; then
    ./autogen.sh
fi

./configure
make dist
