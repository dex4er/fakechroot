#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

case "`uname -s`" in
    Linux|KFreeBSD)
        CP_ARGS=-dp;;
    *)
        CP_ARGS=-a;;
esac

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

        echo 'something' > $testtree/file-$chroot
        ln -s /file-$chroot $testtree/symlink-$chroot

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "cp $CP_ARGS /file-$chroot /file2-$chroot; cat /file2-$chroot" 2>&1`
        test "$t" = "something" || not
        ok "$chroot cp $CP_ARGS /file-$chroot /file2-$chroot:" $t

        t=`$srcdir/$chroot.sh $testtree /bin/sh -c "cp $CP_ARGS /symlink-$chroot /symlink2-$chroot; cat /symlink2-$chroot" 2>&1`
        test "$t" = "something" || not
        ok "$chroot cp $CP_ARGS /symlink-$chroot /symlink2-$chroot:" $t

    fi

done

cleanup
