#!/bin/bash

./autogen.sh
./configure

( cd man && make fakechroot.1 )

make dist
