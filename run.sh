#!/bin/bash

#./autogen.sh
#./configure
make clean
make
cp src/.libs/libfakechroot.so ~/go/src/github.com/jasonyangshadow/lpmx/build/
cp src/.libs/libfakechroot.so /tmp/lpmx_test/.lpmx/fakechroot/
