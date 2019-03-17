#!/bin/sh

# This script setups new environment for Arch Linux system, installs necessary
# packages with pacman, downloads source package for hello, and builds the
# binary package.

srcdir=${srcdir:-.}
abs_srcdir=${abs_srcdir:-`cd "$srcdir" 2>/dev/null && pwd -P`}

test -d "$abs_srcdir/bin" && export PATH="$abs_srcdir/bin:$PATH"

run () {
    HOME=/root fakechroot chroot $destdir "$@"
}

run_root () {
    HOME=/root fakechroot fakeroot chroot $destdir "$@"
}

die () {
    echo "$@" 1>&2
    exit 1
}

command -v fakeroot >/dev/null 2>&1 || die 'fakeroot command is missing (sudo apt-get install fakeroot)'

mirror=${MIRROR:-http://mirrors.kernel.org/archlinux}
release=${RELEASE:-2019.04.01}
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

if [ -n "$ARCHLINUX_CACHE" ]; then
    mkdir -p "$ARCHLINUX_CACHE"
fi

url=$mirror/iso/$release/archlinux-bootstrap-$release-$arch.tar.gz
tarball=`test -d "$ARCHLINUX_CACHE" && cd "$ARCHLINUX_CACHE"; pwd`/`basename $url`

if ! [ -f "$tarball" ]; then
    wget -O "$tarball.part" $url
    mv -f "$tarball.part" "$tarball"
fi

export FAKECHROOT_AF_UNIX_PATH=/tmp

if ! command -v chroot >/dev/null 2>&1; then
    PATH=$PATH:/usr/sbin:/sbin
    export PATH
fi

rm -rf "$destdir"

ls -l "$tarball"

mkdir -p "$destdir"
tar zx --strip-components=1 --directory="$destdir" -f $tarball

rm -f "$destdir/etc/mtab"
echo "rootfs / rootfs rw 0 0" > "$destdir/etc/mtab"
cp "/etc/resolv.conf" "$destdir/etc/"
sed -ni '/^[ \t]*CheckSpace/ !p' "$destdir/etc/pacman.conf"
sed -i "s/^[ \t]*SigLevel[ \t].*/SigLevel = Never/" "$destdir/etc/pacman.conf"
echo "Server = $mirror/\$repo/os/$arch" >> "$destdir/etc/pacman.d/mirrorlist"

run_root pacman -Sy

mkdir -p "$destdir/tmp/hello"
cat > "$destdir/tmp/hello/PKGBUILD" << 'END'
pkgname=hello
pkgver=2.10
pkgrel=1
pkgdesc="GNU Hello"
arch=('i686' 'x86_64')
url="http://www.gnu.org/software/hello/"
license=('GPL')
source=(http://ftp.gnu.org/gnu/hello/hello-${pkgver}.tar.gz)
md5sums=('6cd0ffea3884a4e79330338dcc2987d6')


build() {
  cd ${srcdir}/${pkgname}-${pkgver}
  ./configure --prefix=/usr
  make
}

check() {
  cd ${srcdir}/${pkgname}-${pkgver}
  make check
}
  
package() {
  cd ${srcdir}/${pkgname}-${pkgver}
  make DESTDIR=${pkgdir} install
}
END

run_root pacman -S --noconfirm diffutils fakeroot file gcc grep make
run sh -c 'cd /tmp/hello && makepkg'
run_root sh -c 'cd /tmp/hello && pacman -U --noconfirm hello-*.pkg.tar.*'
run sh -c 'hello'
