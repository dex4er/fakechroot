#!/bin/bash

echo "making 64bit library"
A64="x86_64"
if [ -d $A64 ];then
  rm -rf $A64 
fi
mkdir -p $A64
./configure
make clean
make
cp src/.libs/libfakechroot.so $A64/

echo "making 32bit library"
A32="i686"
if [ -d $A32 ];then
  rm -rf $A32 
fi
mkdir -p $A32
./configure --host=i686-linux-gnu "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32"
make clean
make
cp src/.libs/libfakechroot.so $A32/
