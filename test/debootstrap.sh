#!/bin/sh

srcdir=${srcdir:-.}

test -d $srcdir/bin && export PATH=$srcdir/bin:$PATH

run () {
    HOME=/root fakechroot /usr/sbin/chroot $destdir "$@"
}

vendor=${VENDOR:-`lsb_release -s -i`}
release=${RELEASE:-`lsb_release -s -c`}
type=`dpkg-architecture -qDEB_HOST_GNU_TYPE`
systype=${type#*-}
arch=${ARCH:-`dpkg-architecture -t$(arch)-$systype -qDEB_HOST_ARCH 2>/dev/null`}

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=`pwd -P`/testtree
fi

tarball=$vendor-$release-$arch.debs.tgz

export FAKECHROOT_EXCLUDE_PATH=${FAKECHROOT_EXCLUDE_PATH:-/dev:/proc:/sys}
export FAKECHROOT_CMD_SUBST=/sbin/insserv=/bin/true:/sbin/ldconfig=/bin/true:/usr/bin/ischroot=/bin/true:/usr/bin/ldd=/usr/bin/ldd.fakechroot:/usr/bin/mkfifo=/bin/true
export FAKECHROOT_AF_UNIX_PATH=/tmp

debootstrap_opts="--arch=$arch --variant=fakechroot"
if [ ! -f $tarball ]; then
    FAKECHROOT=true fakeroot /usr/sbin/debootstrap --download-only --make-tarball=$tarball --include=build-essential,devscripts,fakeroot,gnupg $debootstrap_opts $release $destdir "$@"
fi

rm -rf $destdir

if ! which chroot >/dev/null; then
    PATH=$PATH:/usr/sbin:/sbin
    export PATH
fi

fakechroot fakeroot /usr/sbin/debootstrap --unpack-tarball="`pwd`/$tarball" $debootstrap_opts $release $destdir

# workaround for bug in mountall package
rm -rf $destdir/dev
mkdir -p $destdir/dev
FAKECHROOT_EXCLUDE_PATH=/proc:/sys FAKECHROOT_CMD_SUBST=/bin/rm=/bin/true HOME=/root fakechroot fakeroot /usr/sbin/chroot $destdir apt-get --force-yes -y --no-install-recommends install mountall || true
HOME=/root fakechroot fakeroot /usr/sbin/chroot $destdir apt-get --force-yes -y --no-install-recommends install -f

cp -v `cd $srcdir; pwd`/../scripts/ldd.pl $destdir/usr/bin/ldd

HOME=/root fakechroot fakeroot /usr/sbin/chroot $destdir apt-get --force-yes -y --no-install-recommends install build-essential devscripts fakeroot gnupg

run sh -c 'cat /etc/apt/sources.list | sed "s/^deb/deb-src/" >> /etc/apt/sources.list'
run fakeroot apt-get --force-yes -y update
run sh -c 'cd /tmp && apt-get --force-yes -y source hello && cd hello-* && debuild --preserve-env -b'
run fakeroot sh -c 'dpkg -i /tmp/hello_*.deb'
run sh -c 'hello'
