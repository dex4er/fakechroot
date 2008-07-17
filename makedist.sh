#!/bin/bash

if [ ! -x configure ] || [ ! -x fake/configure ]; then
    ./autogen.sh
fi

./configure

pushd fake
    ./configure
popd

make dist
