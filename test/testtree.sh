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
    /lib32 \
    /lib64 \
    /root \
    /sbin \
    /tmp \
    /usr/bin \
    /usr/lib \
    /usr/sbin \
    /usr/local/bin
do
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
    '/usr/bin/find' \
    '/usr/bin/id' \
    '/usr/bin/ltrace' \
    '/usr/bin/perl' \
    '/usr/bin/seq' \
    '/usr/bin/strace' \
    '/usr/bin/touch' \
    '/usr/sbin/chroot' \
    '/usr/local/bin/bash' \
    '/usr/local/bin/strace'
do
    for f in $p; do
	cp -pf $PREFIX$f $destdir/$(dirname $f) 2>/dev/null
    done
done

for p in \
    'ld-linux.so.*' \
    'ld-linux-x86-64.so.*' \
    'ld-uClibc.so.*' \
    'libacl.so.*' \
    'libattr.so.*' \
    'libc.so.*' \
    'libcrypt.so.*' \
    'libdl.so.*' \
    'libgcc_s.so.*' \
    'libpthread.so.*' \
    'librt.so.*' \
    'libselinux.so.*' \
    'libm.so.*' \
    'libncurses.so.*' \
    'linux-vdso.so.*'
do
    for a in '' 32 64; do
        fp="/lib$a/$p"
        for f in $fp; do
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

echo $destdir > $destdir/CHROOT
