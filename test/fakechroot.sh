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

for p in \
    '/bin/bash' \
    '/bin/busybox' \
    '/bin/cat' \
    '/bin/chmod' \
    '/bin/csh' \
    '/bin/cp' \
    '/bin/grep' \
    '/bin/sh' \
    '/bin/ls' \
    '/bin/mkdir' \
    '/bin/ps' \
    '/bin/pwd' \
    '/bin/rm' \
    '/bin/sh' \
    '/lib/ld-linux.so.*' \
    '/lib/ld-uClibc.so.*' \
    '/lib/libacl.so.*' \
    '/lib/libattr.so.*' \
    '/lib/libc.so.*' \
    '/lib/libcrypt.so.*' \
    '/lib/libdl.so.*' \
    '/lib/libgcc_s.so.*' \
    '/lib/libpthread.so.*' \
    '/lib/librt.so.*' \
    '/lib/libselinux.so.*' \
    '/lib/libm.so.*' \
    '/usr/bin/basename' \
    '/usr/bin/dirname' \
    '/usr/bin/find' \
    '/usr/bin/id' \
    '/usr/bin/ltrace' \
    '/usr/bin/perl' \
    '/usr/bin/strace' \
    '/usr/sbin/chroot' \
    '/usr/local/bin/bash' \
    '/usr/local/bin/strace'
do
    for f in $p; do
	cp -pf $PREFIX$f testtree/$(dirname $f)
    done
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
    HOME=/root testtree/usr/sbin/chroot `pwd`/testtree "$@"
else
    HOME=/root testtree/usr/sbin/chroot `pwd`/testtree /bin/bash
fi
