#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

plan 1

pwd=`dirname $0`
abs_top_srcdir=${abs_top_srcdir:-`cd "$pwd/../.." 2>/dev/null && pwd -P`}

interp_file=$(file /bin/true | sed 's/^.*, interpreter \([^,]\+\), .*$/\1/')
interp_readelf=$(readelf --string-dump=.interp /bin/true | sed -ne 's/^  \[ \+[0-9]\+\]  //p')

# diag "$interp_file" "$interp_readelf"

test "$interp_file" = "$interp_readelf" || not

# ldd /bin/true | diag

ldd /bin/true | grep --quiet "^[[:space:]]$interp_file (" || not

# "$abs_top_srcdir/scripts/ldd.fakechroot" /bin/true | diag

"$abs_top_srcdir/scripts/ldd.fakechroot" /bin/true | grep --quiet "^[[:space:]]$interp_file (" || not

ok "ldd lists interpreter $interp_file"
