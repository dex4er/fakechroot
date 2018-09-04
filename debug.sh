#!/bin/bash

LD_PRELOAD=~/git/fakechroot/src/.libs/libfakechroot.so FAKECHROOT_DEBUG=true touch tmpfile >/tmp/file 2>&1
