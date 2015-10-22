#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

found_jemalloc=`
    for f in /lib/libjemalloc.so* /lib/*-*-*/libjemalloc.so* /usr/lib/libjemalloc.so* /usr/lib/*-*-*/libjemalloc.so*; do
        test -f $f && echo yes && exit 0
    done
    echo no
`
test $found_jemalloc = "yes" || skip_all 'jemalloc library is missing (sudo apt-get install libjemalloc1)'

prepare 1

t=`$srcdir/fakechroot.sh $testtree sh -c 'LD_PRELOAD="$LD_PRELOAD /usr/lib/x86_64-linux-gnu/libjemalloc.so.1" cat /CHROOT' 2>&1`
test "$t" = "testtree-jemalloc" || not
ok "fakechroot LD_PRELOAD=libjemalloc.so.1 cat:" $t

cleanup
