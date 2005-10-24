#!/bin/sh

VERSION=`grep '^AC_INIT' ../configure.ac | sed 's/.*\[\([0-9][0-9.]*\)\].*/\1/'`

for d in \
    /bin \
    /etc \
    /lib \
    /root \
    /sbin \
    /usr/bin \
    /usr/lib \
    /usr/sbin \
    /usr/local/bin
do
    mkdir -p testtree/$d
done

for d in \
    /dev \
    /proc
do
    rm -f testtree/$d
    ln -sf $d testtree/$d
done

for f in \
    /bin/csh \
    /bin/sh \
    /bin/bash \
    /bin/grep \
    /bin/ls \
    /bin/pwd \
    /bin/sh \
    /usr/bin/id \
    /usr/bin/find \
    /usr/bin/perl \
    /usr/bin/ltrace \
    /usr/bin/strace \
    /usr/sbin/chroot \
    /usr/local/bin/bash \
    /usr/local/bin/strace
do
    ln -sf $f testtree/$f
done

dir=`cd \`pwd\`/../src/.libs; pwd`

#if [ -n "$LD_LIBRARY_PATH" ]; then
#    LD_LIBRARY_PATH="$dir:$LD_LIBRARY_PATH"
#else
#    LD_LIBRARY_PATH="$dir"
#fi
#export LD_LIBRARY_PATH

if [ -n "$LD_PRELOAD" ]; then
    LD_PRELOAD="$LD_PRELOAD libfakechroot.so"
else
    LD_PRELOAD="$dir/libfakechroot.so"
fi
export LD_PRELOAD

if [ -n "$*" ]; then
    HOME=/root /usr/sbin/chroot `pwd`/testtree "$@"
else
    HOME=/root /usr/sbin/chroot `pwd`/testtree /bin/sh
fi
