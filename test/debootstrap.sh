#!/bin/sh

# This script setups new environment with debootstrap, installs necessary
# packages with APT, downloads source package for hello, and builds the
# binary package.
#
# It should work with any Debian-based system.

srcdir=${srcdir:-.}
abs_srcdir=${abs_srcdir:-`cd "$srcdir" 2>/dev/null && pwd -P`}

top_srcdir=${top_srcdir:-..}
abs_top_srcdir=${abs_top_srcdir:-`cd "$top_srcdir" 2>/dev/null && pwd -P`}

. $top_srcdir/config.sh

test -d "$abs_srcdir/bin" && export PATH="$abs_srcdir/bin:$PATH"
test -f "$abs_top_srcdir/scripts/debootstrap.env" && . "$abs_top_srcdir/scripts/debootstrap.env"

run () {
    HOME=/root fakechroot chroot $destdir "$@"
}

die () {
    echo "$@" 1>&2
    exit 1
}

command -v $DEBOOTSTRAP >/dev/null 2>&1 || die 'debootstrap command is missing (sudo apt-get install debootstrap)'
command -v fakeroot     >/dev/null 2>&1 || die 'fakeroot command is missing (sudo apt-get install fakeroot)'
command -v lsb_release  >/dev/null 2>&1 || die 'lsb_release command is missing (sudo apt-get install lsb-release)'
command -v xzcat        >/dev/null 2>&1 || die 'xzcat command is missing (sudo apt-get install xz-utils)'

vendor=${VENDOR:-`lsb_release -s -i`}
release=${RELEASE:-`lsb_release -s -c`}
variant=$VARIANT
type=`dpkg-architecture -qDEB_HOST_GNU_TYPE 2>/dev/null`
systype=${type#*-}
arch=${ARCH:-`dpkg-architecture -t$(arch)-$systype -qDEB_HOST_ARCH 2>/dev/null`}

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir="$abs_srcdir/testtree"
fi

if [ -n "$DEBOOTSTRAP_CACHE" ]; then
    mkdir -p "$DEBOOTSTRAP_CACHE"
fi

tarball=`test -d "$DEBOOTSTRAP_CACHE" && cd "$DEBOOTSTRAP_CACHE"; pwd`/$vendor-$release${variant:+-$variant}-$arch.debs.tgz

debootstrap_opts="--arch=$arch ${variant:+--variant=$variant}"
if [ ! -f $tarball ]; then
    FAKECHROOT=true fakeroot $DEBOOTSTRAP --make-tarball=$tarball --include=fakeroot $debootstrap_opts $release $destdir "$@"
fi

rm -rf $destdir

ls -l $tarball

fakechroot fakeroot $DEBOOTSTRAP --unpack-tarball="$tarball" $debootstrap_opts $release $destdir || cat $destdir/debootstrap/debootstrap.log

unset CC CFLAGS LDFLAGS EXTRA_CFLAGS EXTRA_LDFLAGS V

HOME=/root fakechroot fakeroot $CHROOT $destdir apt-get -y --no-install-recommends install build-essential devscripts fakeroot gnupg

run sh -c 'cat /etc/apt/sources.list | sed "s/^deb/deb-src/" >> /etc/apt/sources.list'
run fakeroot apt-get -y update
run fakeroot apt-get -y --no-install-recommends build-dep hello
run sh -c 'cd /tmp && apt-get -y source hello && cd hello-* && debuild --preserve-env -b -uc -us'
run fakeroot sh -c 'dpkg -i /tmp/hello_*.deb'
run sh -c 'hello'
