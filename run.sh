#!/bin/bash

./autogen.sh
./configure
make clean
make
LD_PRELOAD=src/.libs/libfakechroot.so touch file
LD_PRELOAD=src/.libs/libfakechroot.so ln -s file file.bak
LD_PRELOAD=src/.libs/libfakechroot.so rm -rf file
LD_PRELOAD=src/.libs/libfakechroot.so rm -rf file.bak
