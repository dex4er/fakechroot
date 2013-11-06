#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

plan 1

if ! command -v host >/dev/null 2>&1; then
    skip 1 "missing host command"
elif ! host 127.0.0.1 2>&1 | grep -qs 1.0.0.127.in-addr.arpa; then
    skip 1 "host 127.0.0.1 does not work"
else
    PATH=$srcdir/bin:$PATH
    export LIBC_FATAL_STDERR_=1
    t=`fakechroot host 127.0.0.1 2>&1`
    echo "$t" | grep -qs 1.0.0.127.in-addr.arpa || not
    ok "host 127.0.0.1 returns" "$t"
fi

end
