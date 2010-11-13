#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

export PATH=$srcdir/bin:$PATH

run () {
    HOME=/root fakechroot /usr/sbin/chroot $destdir "$@"
}

vendor=${VENDOR:-`lsb_release -s -i | tr 'A-Z' 'a-z'`}
release=${RELEASE:-`lsb_release -s -r`}
arch=${ARCH:-`uname -m`}

if [ $# -gt 0 ]; then
    destdir=$1
    shift
else
    destdir=testtree
fi

export FAKECHROOT_CMD_SUBST=/sbin/ldconfig=/bin/true:/usr/sbin/glibc_post_upgrade.i686=/bin/true:/usr/sbin/glibc_post_upgrade.x86_64=/bin/true:/usr/sbin/build-locale-archive=/bin/true:/usr/sbin/libgcc_post_upgrade=/bin/true:/sbin/new-kernel-pkg=/bin/true:/usr/sbin/nscd=/bin/true
export FAKECHROOT_AF_UNIX_PATH=/tmp

rm -rf $destdir

if ! which chroot >/dev/null; then
    PATH=$PATH:/usr/sbin:/sbin
    export PATH
fi

febootstrap $vendor-$release $destdir

for i in dev proc sys; do
    rm -rf $destdir/$i
    ln -s /$i $destdir/$i
done

rm -rf $destdir/tmp
mkdir -p $destdir/tmp

cp -v `cd $srcdir; pwd`/../scripts/ldd.pl $destdir/usr/bin/ldd
cp -v /etc/resolv.conf $destdir/etc
rm -fv $destdir/etc/yum.repos.d/*update*.repo
sed -i 's/^enabled=0/enabled=1/' $destdir/etc/yum.repos.d/*.repo

HOME=/root fakeroot fakechroot /usr/sbin/chroot $destdir yum -y update
HOME=/root fakeroot fakechroot /usr/sbin/chroot $destdir yum -y install fakeroot gcc gettext make rpm-build yum-utils

run fakeroot sh -c 'cd /tmp && yumdownloader --source hello'
run sh -c 'mkdir -p /root/rpmbuild/SOURCES && cd /root/rpmbuild/SOURCES && rpm2cpio /tmp/hello-*.src.rpm | cpio -idmv "*.tar.gz"'
run sh -c 'mkdir -p /root/rpmbuild/SPECS && cd /root/rpmbuild/SPECS && rpm2cpio /tmp/hello-*.src.rpm | cpio -idmv "*.spec"'
run sh -c 'cd /root/rpmbuild/SPECS && rpm2cpio /tmp/hello-*.src.rpm | cpio -idmv "*.spec"'
run fakeroot sh -c 'cd /root/rpmbuild/SPECS && rpmbuild -ba hello.spec'
run fakeroot sh -c 'rpm -i /root/rpmbuild/RPMS/*/*.rpm'
run sh -c 'hello'
