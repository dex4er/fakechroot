#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 28

buf=`for i in $($SEQ 1 1024); do printf "A"; done`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        for path in / . ..; do
            t=`$srcdir/$chroot.sh $testtree /bin/test-realpath $path 2>&1`
            test "$t" = "/" || not
            ok "$chroot realpath for $path is really" $t

            t=`$srcdir/$chroot.sh $testtree /bin/test-realpath $path $buf 2>&1`
            test "$t" = "/" || not
            ok "$chroot file's realpath with buf for $path is really" $t
        done

        echo "something" > $testtree/$chroot-file
        mkdir $testtree/$chroot-dir

        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-file 2>&1`
        test "$t" = "something" || not
        ok "$chroot symlink is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-realpath $chroot-file 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot file's realpath is really" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-realpath $chroot-file $buf 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot file's realpath with buf is really" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "ln -s /$chroot-dir $chroot-symlink-dir; test -h $chroot-symlink-dir && echo exists" 2>&1`
        test "$t" = "exists" || not
        ok "$chroot symlink dir" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "ln -s /$chroot-file $chroot-symlink-dir/$chroot-symlink-file; test -h $chroot-symlink-dir/$chroot-symlink-file && echo exists" 2>&1`
        test "$t" = "exists" || not
        ok "$chroot symlink file" $t

        t=`$srcdir/$chroot.sh $testtree /bin/cat $chroot-symlink-dir/$chroot-symlink-file 2>&1`
        test "$t" = "something" || not
        ok "$chroot symlink is" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-realpath $chroot-symlink-dir/$chroot-symlink-file 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot symlink's realpath is really" $t

        t=`$srcdir/$chroot.sh $testtree /bin/test-realpath $chroot-symlink-dir/$chroot-symlink-file $buf 2>&1`
        test "$t" = "/$chroot-file" || not
        ok "$chroot symlink's realpath with buf is really" $t

    fi

done

cleanup
