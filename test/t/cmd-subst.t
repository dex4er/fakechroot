#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

cmddir=`cd $srcdir; pwd`/t

t=`$srcdir/fakechroot.sh testtree /bin/pwd 2>&1`
test "$t" = "/" || not
ok "fakechroot pwd [1] is" $t

export FAKECHROOT_CMD_SUBST="/bin/pwd=$cmddir/cmd-subst-pwd.sh"

t=`$srcdir/fakechroot.sh testtree /bin/pwd 2>&1`
test "$t" = "substituted" || not
ok "fakechroot pwd [2] is" $t

export FAKECHROOT_CMD_SUBST="/no/file=foo:/bin/pwd=$cmddir/cmd-subst-pwd.sh"

t=`$srcdir/fakechroot.sh testtree /bin/pwd 2>&1`
test "$t" = "substituted" || not
ok "fakechroot pwd [3] is" $t

export FAKECHROOT_CMD_SUBST="/no/file=foo:/other/file=bar"

t=`$srcdir/fakechroot.sh testtree /bin/pwd 2>&1`
test "$t" = "/" || not
ok "fakechroot pwd [4] is" $t

cleanup
