#!/bin/sh

# This script setups new environment with Rinse
# (http://www.steve.org.uk/Software/rinse/), installs necessary packages
# with Yum, downloads source package for hello, and builds the binary
# package.
#
# It should work with systems supported by Rinse (CentOS, Fedora,
# OpenSUSE).

srcdir=${srcdir:-.}
abs_srcdir=${abs_srcdir:-`cd "$srcdir" 2>/dev/null && pwd -P`}

test -d "$abs_srcdir/bin" && export PATH="$abs_srcdir/bin:$PATH"

run () {
    HOME=/root fakechroot chroot $destdir "$@"
}

run_fakeroot () {
    HOME=/root fakechroot fakeroot chroot $destdir "$@"
}

vendor=${VENDOR:-`lsb_release -s -i`}
release=${RELEASE:-`lsb_release -s -r`}
arch=${ARCH:-`uname -m`}

case $arch in
    i?86) arch=i386;;
esac

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir="$abs_srcdir/testtree"
fi

tarball=$vendor-$release-$arch.rpms.tgz
distribution=`echo $vendor | tr 'A-Z' 'a-z'`-`echo $release | sed 's/\..*//'`

rm -rf rinse-cache
mkdir rinse-cache

if [ -f $tarball ]; then
    ( cd rinse-cache; tar zxf ../$tarball )
fi

export FAKECHROOT_AF_UNIX_PATH=/tmp

rm -rf "$destdir"

if ! which chroot >/dev/null; then
    PATH=$PATH:/usr/sbin:/sbin
    export PATH
fi

fakechroot fakeroot rinse --distribution $distribution --directory "$destdir" --arch $arch --cache 1 --cache-dir "$abs_srcdir/rinse-cache" "$@"

if [ ! -f $tarball ]; then
    ( cd rinse-cache; tar zcf ../$tarball * )
fi

cp -v /etc/resolv.conf "$destdir/etc"
rm -fv "$destdir"/etc/yum.repos.d/*update*.repo
grep -E -r -l 'baseurl=(ftp|https?)' "$destdir"/etc/yum.repos.d/*.repo | while read file; do
    sed -i 's/^enabled=0/enabled=1/' "$file"
done

run_fakeroot yum -y update

run_fakeroot yum -y install gcc gettext make rpm-build tar yum-utils

# rpmbuild has statically compiled glob() function so buildroot directory have to be the same inside and outside fakechroot.
export FAKECHROOT_EXCLUDE_PATH=/tmp
rm -rf "$destdir/tmp"
ln -s /tmp "$destdir/tmp"

if [ ! -f /tmp/hello-*.src.rpm ]; then
    run_fakeroot sh -c 'cd /tmp && yumdownloader --source hello'
fi
run sh -c 'mkdir -p /usr/src/redhat/SOURCES && cd /usr/src/redhat/SOURCES && rpm2cpio /tmp/hello-*.src.rpm | cpio -idmv "*.tar.gz"'
run sh -c 'mkdir -p /usr/src/redhat/SPECS && cd /usr/src/redhat/SPECS && rpm2cpio /tmp/hello-*.src.rpm | cpio -idmv "*.spec"'
run sh -c 'cd /usr/src/redhat/SPECS && rpm2cpio /tmp/hello-*.src.rpm | cpio -idmv "*.spec"'
run_fakeroot sh -c 'cd /usr/src/redhat/SPECS && mkdir -p /tmp/rpmbuild/BUILDROOT && rpmbuild --buildroot=/tmp/rpmbuild/BUILDROOT -ba hello.spec'
run_fakeroot sh -c 'rpm -i /usr/src/redhat/RPMS/*/*.rpm'
run sh -c 'hello'
