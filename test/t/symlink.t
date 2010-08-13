#!/bin/sh

srcdir=${srcdir:-.}

. $srcdir/common.sh

plan 22

rm -rf testtree

$srcdir/testtree.sh testtree
t=`cat testtree/CHROOT 2>&1`
test "$t" = "testtree" || not
ok "testtree/CHROOT is" $t

echo something > testtree/file
t=`cat testtree/file 2>&1`
test "$t" = "something" || not
ok "testtree/file is" $t

rm -f testtree/rel-symlink
ln -s file testtree/rel-symlink
t=`cat testtree/rel-symlink 2>&1`
test "$t" = "something" || not
ok "testtree/rel-symlink is" $t
t=`readlink testtree/rel-symlink 2>&1`
test "$t" = "file" || not
ok "testtree/rel-symlink links to" $t

rm -f testtree/abs-symlink
ln -s $(pwd)/testtree/file testtree/abs-symlink
t=`cat testtree/abs-symlink 2>&1`
test "$t" = "something" || not
ok "testtree/abs-symlink is" $t
t=`readlink testtree/abs-symlink 2>&1 | sed "s*$(pwd)*SRCDIR*"`
test "$t" = "SRCDIR/testtree/file" || not
ok "testtree/abs-symlink links to" $t

t=`testtree/bin/test-lstat testtree/rel-symlink 2>&1`
rs=`echo "$t" | grep '^readlink size: ' | sed 's/.*: //'`
ss=`echo "$t" | grep '^stat size: ' | sed 's/.*: //'`
test -n "$rs" -a "$rs" = "$ss" || not
ok "testtree/rel-symlink readlink size [" $rs "] = stat size [" $ss "]"

t=`testtree/bin/test-lstat testtree/abs-symlink 2>&1`
rs=`echo "$t" | grep '^readlink size: ' | sed 's/.*: //'`
ss=`echo "$t" | grep '^stat size: ' | sed 's/.*: //'`
test -n "$rs" -a "$rs" = "$ss" || not
ok "testtree/abs-symlink readlink size [" $rs "] = stat size [" $ss "]"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip 7 "not root"
    else

        t=`$srcdir/$chroot.sh testtree /bin/cat file 2>&1`
        test "$t" = "something" || not
        ok "$chroot file is" $t

        rm -f testtree/rel-symlink
        $srcdir/$chroot.sh testtree /bin/ln -s file rel-symlink
        t=`$srcdir/$chroot.sh testtree /bin/cat rel-symlink 2>&1`
        test "$t" = "something" || not
        ok "$chroot rel-symlink is" $t
        t=`$srcdir/$chroot.sh testtree /bin/readlink rel-symlink 2>&1`
        test "$t" = "file" || not
        ok "$chroot rel-symlink links to" $t

        rm -f testtree/abs-symlink
        $srcdir/$chroot.sh testtree /bin/ln -s /file abs-symlink
        t=`$srcdir/$chroot.sh testtree /bin/cat abs-symlink 2>&1`
        test "$t" = "something" || not
        ok "$chroot abs-symlink is" $t
        t=`$srcdir/$chroot.sh testtree /bin/readlink abs-symlink 2>&1`
        test "$t" = "/file" || not
        ok "$chroot abs-symlink links to" $t

        t=`$srcdir/$chroot.sh testtree /bin/test-lstat rel-symlink 2>&1`
        rs=`echo "$t" | grep '^readlink size: ' | sed 's/.*: //'`
        ss=`echo "$t" | grep '^stat size: ' | sed 's/.*: //'`
        test -n "$rs" -a "$rs" = "$ss" || not
        ok "testtree/rel-symlink readlink size [" $rs "] = stat size [" $ss "]"

        t=`$srcdir/$chroot.sh testtree /bin/test-lstat /abs-symlink 2>&1`
        rs=`echo "$t" | grep '^readlink size: ' | sed 's/.*: //'`
        ss=`echo "$t" | grep '^stat size: ' | sed 's/.*: //'`
        test -n "$rs" -a "$rs" = "$ss" || not
        ok "testtree/abs-symlink readlink size [" $rs "] = stat size [" $ss "]"

    fi

done

rm -rf testtree

end
