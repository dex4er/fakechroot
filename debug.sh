#!/bin/bash

LD_PRELOAD=~/git/fakechroot/src/.libs/libfakechroot.so __LOG_LEVEL=debug FAKECHROOT_DEBUG=true CONTAINER_WORKSPACE=/tmp touch tmpfile >/tmp/file 2>&1
