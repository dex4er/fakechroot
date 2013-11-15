#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

plan 47

de_dotdot="$srcdir/src/test-de_dotdot"

set -- \
    a a \
    a/ a \
    /a /a \
    /a/ /a \
    /a/// /a \
    /a/. /a \
    /a/././. /a \
    /a/./ /a \
    a/b/c/d a/b/c/d \
    /a/b/c/d /a/b/c/d \
    /a/b/./c/d /a/b/c/d \
    a/b/../c/d a/c/d \
    /a/b/../c/d /a/c/d \
    a/b/../../c/d c/d \
    /a/b/../../c/d /c/d \
    a/b/../../../c/d ../c/d \
    /a/b/../../../c/d /c/d \
    a/../b/../../c/../d ../c/../d \
    /a/../b/../../c/../d /d \
    a/.. . \
    a/../.. .. \
    a/../../.. ../.. \
    a/../../../.. ../../.. \
    /a/.. / \
    /a/../../ / \
    '' . \
    . . \
    / / \
    /. / \
    .. .. \
    /.. / \
    ../.. ../.. \
    /../.. / \
    .././../ ../.. \
    /.././../ / \
    ../a ../a \
    ../../a ../../a \
    abcdef/ghijkl/mnopqr abcdef/ghijkl/mnopqr \
    /abcdef/ghijkl/mnopqr /abcdef/ghijkl/mnopqr \
    //abcdef/ghijkl/mnopqr /abcdef/ghijkl/mnopqr \
    ../abcdef/ghijkl/mnopqr ../abcdef/ghijkl/mnopqr \
    /../abcdef/ghijkl/mnopqr /abcdef/ghijkl/mnopqr \
    abcdef/../ghijkl/mnopqr ghijkl/mnopqr \
    /abcdef/../ghijkl/mnopqr /ghijkl/mnopqr \
    /abcdef/ghijkl/../mnopqr /abcdef/mnopqr \
    abcdef/ghijkl/../mnopqr abcdef/mnopqr \
    /abcdef/ghijkl/mnopqr/.. /abcdef/ghijkl \

while [ $# -gt 1 ]; do
    t=`$de_dotdot "$1" 2>&1`
    test "$t" = "$2" || not
    ok "test-de_dotdot $1 returns" $t
    shift 2
done
