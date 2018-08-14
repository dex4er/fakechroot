#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

command -v setfattr >/dev/null 2>&1 || skip_all 'setfattr command is missing (sudo apt-get install attr)'
command -v attr >/dev/null 2>&1     || skip_all 'attr command is missing (weird, since you have setfattr!)'

prepare 10

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo "something" > $testtree/$chroot-file
        $srcdir/$chroot.sh $testtree /usr/bin/setfattr -n user.someattr -v somevalue /$chroot-file || not
        ok "$chroot added xattr 'someattr' to file"

        t=`$srcdir/$chroot.sh $testtree /usr/bin/getfattr -d /$chroot-file 2>&1 | grep ^user`
        test "$t" = 'user.someattr="somevalue"' || not
        ok "$chroot got xattr back" $t

        t=`$srcdir/$chroot.sh $testtree /usr/bin/attr -qg someattr /$chroot-file 2>&1`
        test "$t" = "somevalue" || not
        ok "$chroot got someattr back" $t

        $srcdir/$chroot.sh $testtree /usr/bin/attr -r someattr /$chroot-file || not
        ok "$chroot removed someattr from file"

        t=`$srcdir/$chroot.sh $testtree /usr/bin/getfattr -d /$chroot-file 2>&1`
        test "$t" = "" || not
        ok "$chroot got empty xattrs back" $t

        rm $testtree/$chroot-file

    fi

done

cleanup
