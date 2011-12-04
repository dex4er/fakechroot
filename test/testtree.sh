#!/bin/sh

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

for d in \
    /bin \
    /etc \
    /lib \
    /lib/*-*-* \
    /lib32 \
    /lib64 \
    /usr/lib \
    /usr/lib/*-*-* \
    /usr/lib32 \
    /usr/lib64 \
    /libexec \
    /root \
    /sbin \
    /tmp \
    /usr/bin \
    /usr/lib \
    /usr/sbin \
    /usr/local/bin \
    /usr/local/lib
do
    test -d $d || continue
    mkdir -p $destdir/$d
done

for d in \
    /dev \
    /proc
do
    rm -f $destdir/$d
    ln -sf $d $destdir/$d
done

for p in \
    '/bin/bash' \
    '/bin/busybox' \
    '/bin/cat' \
    '/bin/chmod' \
    '/bin/csh' \
    '/bin/cp' \
    '/bin/dash' \
    '/bin/echo' \
    '/bin/grep' \
    '/bin/ln' \
    '/bin/ls' \
    '/bin/mkdir' \
    '/bin/ps' \
    '/bin/pwd' \
    '/bin/readlink' \
    '/bin/rm' \
    '/bin/sh' \
    '/bin/sleep' \
    '/bin/touch' \
    '/usr/bin/basename' \
    '/usr/bin/dirname' \
    '/usr/bin/env' \
    '/usr/bin/find' \
    '/usr/bin/id' \
    '/usr/bin/ischroot' \
    '/usr/bin/less' \
    '/usr/bin/ltrace' \
    '/usr/bin/more' \
    '/usr/bin/perl' \
    '/usr/bin/readlink' \
    '/usr/bin/seq' \
    '/usr/bin/sort' \
    '/usr/bin/strace' \
    '/usr/bin/touch' \
    '/usr/sbin/chroot' \
    '/usr/local/bin/bash' \
    '/usr/local/bin/gseq' \
    '/usr/local/bin/ltrace' \
    '/usr/local/bin/strace'
do
    for f in $p; do
        cp -pf $PREFIX$f $destdir/$(dirname $f) 2>/dev/null
    done
done

for p in \
    'ld-*.so' \
    'ld-*.so.*' \
    'ld-linux.so.*' \
    'ld-linux-x86-64.so.*' \
    'ld-uClibc.so.*' \
    'ld.so.*' \
    'libacl.so.*' \
    'libattr.so.*' \
    'libc.so.*' \
    'libcrypt.so.*' \
    'libdl.so.*' \
    'libedit.so.*' \
    'libgcc_s.so.*' \
    'libm.so.*' \
    'libncurses.so.*' \
    'libpthread.so.*' \
    'librt.so.*' \
    'libselinux.so.*' \
    'libtinfo.so.*' \
    'libutil.so.*' \
    'linux-vdso.so.*'
do
    for d in \
        /lib \
        /lib/*-*-* \
        /lib32 \
        /lib64 \
        /usr/lib \
        /usr/lib/*-*-* \
        /usr/lib32 \
        /usr/lib64 \
        /usr/local/lib
    do
        test -d "$d" || continue
        for f in $d/$p; do
            cp -pf $PREFIX$f $destdir/$(dirname $f) 2>/dev/null
        done
    done
done

for p in \
    src/test-*
do
    test -x $p || continue
    cp -pf $p $destdir/bin
done

chflags -R 0 $destdir 2>/dev/null || true

echo $destdir > $destdir/CHROOT
