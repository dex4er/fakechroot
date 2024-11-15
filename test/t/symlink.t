#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

readlink=`command -v readlink 2>/dev/null`
readlink=${readlink:-/bin/readlink}

prepare 19

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo "something" > $testtree/$chroot-file
        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-file 2>&1`
        test "$t" = "something" || not
        ok "$chroot file is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "ln -s $chroot-file $chroot-rel-symlink; test -h $chroot-rel-symlink && echo exists" 2>&1`
        test "$t" = "exists" || not
        ok "$chroot rel-symlink" $t

        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-rel-symlink 2>&1`
        test "$t" = "something" || not
        ok "$chroot rel-symlink is" $t

        t=`$srcdir/$chroot.sh $testtree $readlink $chroot-rel-symlink 2>&1`
        test "$t" = "$chroot-file" || not
        ok "$chroot rel-symlink links to" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-lstat $chroot-rel-symlink 2>&1`
        rs=`echo "$t" | grep '^readlink size: ' | sed 's/.*: //'`
        ss=`echo "$t" | grep '^stat size: ' | sed 's/.*: //'`
        test -n "$rs" -a "$rs" = "$ss" || not
        ok "$chroot rel-symlink readlink size [" $rs "] = stat size [" $ss "]"

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "ln -s /$chroot-file $chroot-abs-symlink; test -h $chroot-abs-symlink && echo exists" 2>&1`
        test "$t" = "exists" || not
        ok "$chroot abs-symlink" $t

        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-abs-symlink 2>&1`
        test "$t" = "something" || not
        ok "$chroot abs-symlink is" $t

        t=`$srcdir/$chroot.sh $testtree $readlink $chroot-abs-symlink 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot abs-symlink links to" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-lstat /$chroot-abs-symlink 2>&1`
        rs=`echo "$t" | grep '^readlink size: ' | sed 's/.*: //'`
        ss=`echo "$t" | grep '^stat size: ' | sed 's/.*: //'`
        test -n "$rs" -a "$rs" = "$ss" || not
        ok "$chroot abs-symlink readlink size [" $rs "] = stat size [" $ss "]"

        t=`$srcdir/$chroot.sh $testtree /bin/test-lstat /$chroot-file 2>&1`
        test "$t" = "regular file size: 10" || not
        ok "$chroot-file is of size 10"

    fi

done

cleanup
