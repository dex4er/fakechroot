#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

libjemalloc=`
    for f in /lib/libjemalloc.so* /lib/*-*-*/libjemalloc.so* /usr/lib/libjemalloc.so* /usr/lib/*-*-*/libjemalloc.so*; do
        test -f $f && echo $f && exit 0
    done
    echo no
`
test $libjemalloc = "no" && skip_all 'jemalloc library is missing (sudo apt-get install libjemalloc-dev)'

prepare 1

t=`$srcdir/fakechroot.sh $testtree sh -c 'LD_PRELOAD="$LD_PRELOAD '$libjemalloc'" cat /CHROOT' 2>&1`
test "$t" = "testtree-jemalloc" || not
ok "fakechroot LD_PRELOAD=$libjemalloc cat:" $t

cleanup
