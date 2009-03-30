#!/bin/sh

. ./tap.sh

plan 5

rm -rf testtree

./testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && [ `id -u` != 0 ]; then
        skip 2 "not root"
    else

        t=`./$chroot.sh testtree /bin/pwd`
        test "$t" = "/" || not
        ok "$chroot pwd is /"

        t=`./$chroot.sh testtree /bin/cat CHROOT`
        test "$t" = "testtree" || not
        ok "$chroot CHROOT is testtree"

    fi    

done

rm -rf testtree

end
