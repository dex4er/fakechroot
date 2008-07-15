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
    /bin/bash \
    /bin/busybox \
    /bin/cat \
    /bin/chmod \
    /bin/csh \
    /bin/cp \
    /bin/grep \
    /bin/sh \
    /bin/ls \
    /bin/mkdir \
    /bin/ps \
    /bin/pwd \
    /bin/rm \
    /bin/sh \
    /lib/ld-linux.so.2 \
    /lib/libacl.so.1 \
    /lib/libattr.so.1 \
    /lib/libc.so.6 \
    /lib/libdl.so.2 \
    /lib/libpthread.so.0 \
    /lib/librt.so.1 \
    /lib/libselinux.so.1 \
    /usr/bin/basename \
    /usr/bin/dirname \
    /usr/bin/find \
    /usr/bin/id \
    /usr/bin/ltrace \
    /usr/bin/perl \
    /usr/bin/strace \
    /usr/sbin/chroot \
    /usr/local/bin/bash \
    /usr/local/bin/strace
do
    cp -pf $f testtree/$f
done

dir=`cd \`pwd\`/../src/.libs 2>/dev/null && pwd`
test -n "$dir" || dir=/usr/lib/fakechroot

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

echo "LD_PRELOAD=$LD_PRELOAD"

if [ -n "$*" ]; then
    HOME=/root /usr/sbin/chroot `pwd`/testtree "$@"
else
    HOME=/root /usr/sbin/chroot `pwd`/testtree /bin/bash
fi
