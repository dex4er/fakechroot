#!/bin/sh

for d in \
    /bin \
    /etc \
    /lib \
    /root \
    /sbin \
    /usr/bin \
    /usr/lib \
    /usr/sbin
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
    /bin/sh \
    /bin/bash \
    /bin/grep \
    /bin/ls \
    /bin/pwd \
    /bin/sh \
    /usr/bin/id \
    /usr/bin/perl \
    /usr/bin/ltrace \
    /usr/bin/strace
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
    LD_PRELOAD="$LD_PRELOAD libfakechroot-2.1.so"
else
    LD_PRELOAD="$dir/libfakechroot-2.1.so"
fi
export LD_PRELOAD

if [ -n "$*" ]; then
    HOME=/root /usr/sbin/chroot $(pwd)/testtree /bin/sh
else
    HOME=/root /usr/sbin/chroot $(pwd)/testtree "$@"
fi
