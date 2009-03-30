#!/bin/sh

. ./tap.sh

plan 17

rm -rf testtree

./testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

if [ ! -x testtree/usr/bin/touch ]; then
    skip 16 "/usr/bin/touch not found"
else

    for chroot in chroot fakechroot; do

        if [ $chroot = "chroot" ] && [ `id -u` != 0 ]; then
            skip 8 "not root"
        else

            t=`./$chroot.sh testtree /usr/bin/touch /tmp/touch.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch"
            test -f testtree/tmp/touch.txt || not
            ok "$chroot touch.txt exists"

            sleep 1

            t=`./$chroot.sh testtree /usr/bin/touch -r /tmp/touch.txt /tmp/touch2.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch -r"
            test -f testtree/tmp/touch2.txt || not
            ok "$chroot touch2.txt exists"
            test testtree/tmp/touch2.txt -nt testtree/tmp/touch.txt && not
            ok "$chroot touch2.txt is not newer than touch.txt"
            test testtree/tmp/touch2.txt -ot testtree/tmp/touch.txt && not
            ok "$chroot touch2.txt is not older than touch.txt"

            sleep 1

            t=`./$chroot.sh testtree /usr/bin/touch -m /tmp/touch.txt 2>&1`
            test "$t" = "" || not
            ok "$chroot touch -m"

            test testtree/tmp/touch.txt -nt testtree/tmp/touch2.txt || not
            ok "$chroot touch.txt is newer than touch2.txt"
        fi    

    done

fi

rm -rf testtree

end
