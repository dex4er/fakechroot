#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 8

cwd=`pwd`
cmddir=`cd $srcdir; pwd`/t

t=`$srcdir/bin/fakechroot /bin/pwd 2>&1`
test "$t" = "$cwd" || not
ok "fakechroot pwd [1] is" $t

t=`$srcdir/fakechroot.sh $testtree /bin/pwd 2>&1`
test "$t" = "/" || not
ok "fakechroot pwd [2] is" $t

export FAKECHROOT_CMD_SUBST="/bin/pwd=$cmddir/cmd-subst-pwd.sh"

t=`$srcdir/bin/fakechroot /bin/pwd 2>&1`
test "$t" = "/bin/pwd" || not
ok "fakechroot pwd [3] is" $t

t=`$srcdir/fakechroot.sh $testtree /bin/pwd 2>&1`
test "$t" = "/bin/pwd" || not
ok "fakechroot pwd [4] is" $t

export FAKECHROOT_CMD_SUBST="/no/file=foo:/bin/pwd=$cmddir/cmd-subst-pwd.sh"

t=`$srcdir/bin/fakechroot /bin/pwd 2>&1`
test "$t" = "/bin/pwd" || not
ok "fakechroot pwd [5] is" $t

t=`$srcdir/fakechroot.sh $testtree /bin/pwd 2>&1`
test "$t" = "/bin/pwd" || not
ok "fakechroot pwd [6] is" $t

export FAKECHROOT_CMD_SUBST="/no/file=foo:/other/file=bar"

t=`$srcdir/bin/fakechroot /bin/pwd 2>&1`
test "$t" = "$cwd" || not
ok "fakechroot pwd [7] is" $t

t=`$srcdir/fakechroot.sh $testtree /bin/pwd 2>&1`
test "$t" = "/" || not
ok "fakechroot pwd [8] is" $t

cleanup
