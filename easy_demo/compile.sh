#!/bin/bash

gcc -shared -fPIC -o getpid.so getpid.c -ldl
gcc -o main main.c
