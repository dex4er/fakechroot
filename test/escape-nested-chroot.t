#!/bin/sh

. ./tap.sh

plan 10

rm -rf testtree

./testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

./testtree.sh testtree/testtree
test "`cat testtree/testtree/CHROOT`" = "testtree/testtree" || not
ok "testtree/testtree"

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && [ `id -u` != 0 ]; then
        for i in `seq 1 4`; do
            skip "not root"
        done
    else

        t=`./$chroot.sh testtree /bin/test-chroot / /testtree 'ls -id /' / / 'ls -id /' 2>/dev/null`
        f1=`echo $t | awk '{print $1}'`
        f2=`echo $t | awk '{print $2}'`
        f3=`echo $t | awk '{print $3}'`
        f4=`echo $t | awk '{print $4}'`
        test "$f2" = "/" -a "$f4" = "/" -a "$f1" = "$f3" || not
        ok "$chroot test-chroot . (not escaped)"

        t=`./$chroot.sh testtree /bin/test-chroot / /testtree 'ls -id /' / . 'ls -id /' 2>/dev/null`
        f1=`echo $t | awk '{print $1}'`
        f2=`echo $t | awk '{print $2}'`
        f3=`echo $t | awk '{print $3}'`
        f4=`echo $t | awk '{print $4}'`
        test "$f2" = "/" -a "$f4" = "/" -a "$f1" = "$f3" || not
        ok "$chroot test-chroot . (not escaped)"

        t=`./$chroot.sh testtree /bin/test-chroot / /testtree 'ls -id /' . / 'ls -id /' 2>/dev/null`
        f1=`echo $t | awk '{print $1}'`
        f2=`echo $t | awk '{print $2}'`
        f3=`echo $t | awk '{print $3}'`
        f4=`echo $t | awk '{print $4}'`
        test "$f2" = "/" -a "$f4" = "/" -a "$f1" = "$f3" || not
        ok "$chroot test-chroot . (not escaped)"

        t=`./$chroot.sh testtree /bin/test-chroot / /testtree 'ls -id /' . . 'ls -id /' 2>/dev/null`
        f1=`echo $t | awk '{print $1}'`
        f2=`echo $t | awk '{print $2}'`
        f3=`echo $t | awk '{print $3}'`
        f4=`echo $t | awk '{print $4}'`
        test "$f2" = "/" -a "$f4" = "/" -a "$f1" != "$f3" || not
        ok "$chroot test-chroot . (escaped)"

    fi    

done

rm -rf testtree

end
