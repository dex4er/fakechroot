#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.sh

plan 5

rm -rf testtree

pwd=`cd $srcdir; pwd`

$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

t=`$srcdir/fakechroot.sh testtree /bin/pwd`
test "$t" = "/" || not
ok "fakechroot pwd is /"

export FAKECHROOT_CMD_SUBST="/bin/pwd=$pwd/cmd-subst-pwd.sh"

t=`$srcdir/fakechroot.sh testtree /bin/pwd`
test "$t" = "substituted" || not
ok "fakechroot substituted pwd (1)"

export FAKECHROOT_CMD_SUBST="/no/file=foo:/bin/pwd=$pwd/cmd-subst-pwd.sh"

t=`$srcdir/fakechroot.sh testtree /bin/pwd`
test "$t" = "substituted" || not
ok "fakechroot substituted pwd (2)"

export FAKECHROOT_CMD_SUBST="/no/file=foo:/other/file=bar"

t=`$srcdir/fakechroot.sh testtree /bin/pwd`
test "$t" = "/" || not
ok "fakechroot not substituted pwd is /"

rm -rf testtree

end
