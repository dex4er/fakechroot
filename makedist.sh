#!/bin/bash

./autogen.sh

./configure
( cd fake && ./configure )

make dist
