#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

run () {
    HOME=/root $destdir/usr/sbin/chroot `pwd -P`/$destdir "$@"
}

vendor=`lsb_release -s -i`
release=`lsb_release -s -c`
arch=`dpkg-architecture -t$(arch)-linux-gnu -qDEB_HOST_ARCH 2>/dev/null`

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

tarball=$vendor-$release-$arch.debs.tgz

prepare_env

debootstrap_opts="--arch=$arch --variant=fakechroot"
if [ ! -f $tarball ]; then
    fakeroot /usr/sbin/debootstrap --download-only --make-tarball=$tarball --include=build-essential,devscripts,fakeroot,gnupg $debootstrap_opts $release $destdir
fi

rm -rf $destdir
fakeroot /usr/sbin/debootstrap --unpack-tarball="`pwd`/$tarball" $debootstrap_opts $release $destdir

cp -v `cd $srcdir; pwd`/../scripts/ldd.pl $destdir/usr/bin/ldd

HOME=/root fakeroot $destdir/usr/sbin/chroot `pwd -P`/$destdir apt-get --force-yes -y --no-install-recommends install build-essential devscripts fakeroot gnupg

run sh -c 'cat /etc/apt/sources.list | sed "s/^deb/deb-src/" >> /etc/apt/sources.list'
run fakeroot apt-get --force-yes -y update
run sh -c 'cd /tmp && apt-get --force-yes -y source hello && cd hello-* && debuild --preserve-env -b'
run fakeroot sh -c 'dpkg -i /tmp/hello_*.deb'
run sh -c 'hello'
