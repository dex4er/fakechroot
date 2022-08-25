#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 1

PATH=$srcdir/bin:$PATH

t=`$srcdir/fakechroot.sh $testtree /bin/test-dladdr`
[ "$t" != "0" ] && not
ok "dladdr returns" $t

cleanup
