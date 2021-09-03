#!/bin/bash

set -e

./makedist.sh

PACKAGE_VERSION=$(sed -nr "s/PACKAGE_VERSION='(.*)'/\1/p" configure)

FAKECHROOT_DSC_SIGNED=fakechroot_$PACKAGE_VERSION.dsc
FAKECHROOT_DSC_UNSIGNED=fakechroot_$PACKAGE_VERSION.dsc.unsigned
FAKECHROOT_ORIG=fakechroot_$PACKAGE_VERSION.orig.tar.gz
FAKECHROOT_DEBIAN=fakechroot_$PACKAGE_VERSION.debian.tar.gz

cp fakechroot.dsc.in $FAKECHROOT_DSC_UNSIGNED

CHANGELOG=$(cat debian/changelog.in)
CHANGELOG_TOPMSG=$(cat <<EOF
fakechroot ($PACKAGE_VERSION) unstable; urgency=low

  * New upstream release.  

 -- Piotr Roszatycki <dexter@debian.org>  $(date)
EOF
)
printf "$CHANGELOG_TOPMSG\n\n$CHANGELOG" >debian/changelog

cp fakechroot-$PACKAGE_VERSION.tar.gz $FAKECHROOT_ORIG
tar -cvzf $FAKECHROOT_DEBIAN debian/

FAKECHROOT_ORIG_SIZE=$(stat --printf="%s" $FAKECHROOT_ORIG)
FAKECHROOT_ORIG_SHA1=$(shasum $FAKECHROOT_ORIG | sed -nr "s/^([a-f0-9]+).*/\1/p") 
FAKECHROOT_ORIG_SHA256=$(sha256sum $FAKECHROOT_ORIG | sed -nr "s/^([a-f0-9]+).*/\1/p")
FAKECHROOT_ORIG_MD5=$(md5sum $FAKECHROOT_ORIG | sed -nr "s/^([a-f0-9]+).*/\1/p")

FAKECHROOT_DEBIAN_SIZE=$(stat --printf="%s" $FAKECHROOT_DEBIAN)
FAKECHROOT_DEBIAN_SHA1=$(shasum $FAKECHROOT_DEBIAN | sed -nr "s/^([a-f0-9]+).*/\1/p")
FAKECHROOT_DEBIAN_SHA256=$(sha256sum $FAKECHROOT_DEBIAN | sed -nr "s/^([a-f0-9]+).*/\1/p")
FAKECHROOT_DEBIAN_MD5=$(md5sum $FAKECHROOT_DEBIAN | sed -nr "s/^([a-f0-9]+).*/\1/p")

sed -i "s/@PACKAGE_VERSION@/$PACKAGE_VERSION/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_ORIG@/$FAKECHROOT_ORIG/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_DEBIAN@/$FAKECHROOT_DEBIAN/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_ORIG_SIZE@/$FAKECHROOT_ORIG_SIZE/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_DEBIAN_SIZE@/$FAKECHROOT_DEBIAN_SIZE/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_ORIG_SHA1@/$FAKECHROOT_ORIG_SHA1/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_DEBIAN_SHA1@/$FAKECHROOT_DEBIAN_SHA1/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_ORIG_SHA256@/$FAKECHROOT_ORIG_SHA256/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_DEBIAN_SHA256@/$FAKECHROOT_DEBIAN_SHA256/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_ORIG_MD5@/$FAKECHROOT_ORIG_MD5/g" $FAKECHROOT_DSC_UNSIGNED
sed -i "s/@FAKECHROOT_DEBIAN_MD5@/$FAKECHROOT_DEBIAN_MD5/g" $FAKECHROOT_DSC_UNSIGNED

# TODO Signing procedure is not fully implemented
# The user must do: gpg --gen-key
gpg --yes --output $FAKECHROOT_DSC_SIGNED --clearsign $FAKECHROOT_DSC_UNSIGNED

rm -rf fakechroot-2.20.2
dpkg-source --no-check --no-copy -x $FAKECHROOT_DSC_SIGNED
cd fakechroot-$PACKAGE_VERSION
dpkg-buildpackage --no-sign -rfakeroot -b
dpkg-buildpackage --no-sign -rfakeroot -A
