#!/bin/bash

echo "making 64bit binaries"
mkdir -p x86_64
gcc -shared -fPIC -o x86_64/getpid.so getpid.c -ldl
gcc -o x86_64/pid pid.c 
gcc -o x86_64/loop1 loop1.c
gcc -o x86_64/loop2 loop2.c

echo "making 32bit binaries"
mkdir -p i686
gcc -m32 -shared -fpic -o i686/getpid.so getpid.c -ldl
gcc -m32 -o i686/pid pid.c
gcc -m32 -o i686/loop1 loop1.c
gcc -m32 -o i686/loop2 loop2.c
