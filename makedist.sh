#!/bin/bash

./autogen.sh
./configure

( cd man && make fakechroot.1 )

pod2readme man/fakechroot.pod README
rm -f README.bak

make dist
