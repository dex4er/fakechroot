#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 18

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for path in / . ..; do
            t=`$srcdir/$chroot.sh $testtree /bin/test-canonicalize_file_name $path 2>&1`
            test "$t" = "/" || not
            ok "$chroot realpath for $path is really" $t
        done

        echo "something" > $testtree/$chroot-file
        mkdir $testtree/$chroot-dir

        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-file 2>&1`
        test "$t" = "something" || not
        ok "$chroot symlink is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-canonicalize_file_name $chroot-file 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot file is really" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "ln -s /$chroot-dir $chroot-symlink-dir; test -h $chroot-symlink-dir && echo exists" 2>&1`
        test "$t" = "exists" || not
        ok "$chroot symlink dir" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "ln -s /$chroot-file $chroot-symlink-dir/$chroot-symlink-file; test -h $chroot-symlink-dir/$chroot-symlink-file && echo exists" 2>&1`
        test "$t" = "exists" || not
        ok "$chroot symlink file" $t

        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-symlink-dir/$chroot-symlink-file 2>&1`
        test "$t" = "something" || not
        ok "$chroot symlink is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-canonicalize_file_name $chroot-symlink-dir/$chroot-symlink-file 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot symlink is really" $t

    fi

done

cleanup
