#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

top_srcdir=${top_srcdir:-..}
abs_top_srcdir=${abs_top_srcdir:-`cd "$top_srcdir" 2>/dev/null && pwd -P`}

. $top_srcdir/config.sh

command -v fakeroot >/dev/null 2>&1 || skip_all 'fakeroot command is missing (sudo apt-get install fakeroot)'
t=`fakeroot $ECHO ok 2>&1`
test "$t" = "ok" || skip_all "fakeroot command failed:" $t

prepare 8

$srcdir/testtree.sh $testtree/testtree2
test "`cat $testtree/testtree2/CHROOT`" = "$testtree/testtree2" || bail_out "$testtree/testtree"

for testtree2 in testtree2 /testtree2 ./testtree2 /./testtree2 testtree2/. testtree2/./. testtree2/ /testtree2/; do
    t=`$srcdir/bin/fakechroot $FAKEROOT $CHROOT $testtree /usr/sbin/chroot $testtree2 /bin/cat /CHROOT 2>&1`
    test "$t" = "testtree-fakeroot-chroot/testtree2" || not
    ok "$chroot chroot $testtree2:" $t
done

cleanup
