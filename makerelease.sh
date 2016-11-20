#!/bin/sh

name=`grep AC_INIT configure.ac | perl -pe '$_ = (/\[(.*?)\]/g)[0]'`
version=`grep AC_INIT configure.ac | perl -pe '$_ = (/\[(.*?)\]/g)[1]'`

tarball="$name-$version.tar.gz"

test -f "$tarball" || exit 1

cat > .git/gbp.conf << END
[import-orig]
dch = False
import-msg = New release version %(version)s
interactive = False
merge = False
pristine-tar = False
symlink-orig = False
upstream-branch = release
upstream-tag = %(version)s
END

set -x

git fetch origin +release:release
git tag -a -m "Release $version" master/$version
gbp import-orig "$tarball"
