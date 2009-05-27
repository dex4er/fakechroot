#!/bin/sh

. ./tap.sh

plan 5

rm -rf testtree

./testtree.sh testtree
test "`cat testtree/CHROOT`" = "testtree" || not
ok "testtree"

t=`./fakechroot.sh testtree /bin/pwd`
test "$t" = "/" || not
ok "fakechroot pwd is /"

export FAKECHROOT_CMD_SUBST="/bin/pwd=$(pwd)/t/cmd-subst-pwd.sh"

t=`./fakechroot.sh testtree /bin/pwd`
test "$t" = "substituted" || not
ok "fakechroot substituted pwd (1)"

export FAKECHROOT_CMD_SUBST="/no/file=foo:/bin/pwd=$(pwd)/t/cmd-subst-pwd.sh"

t=`./fakechroot.sh testtree /bin/pwd`
test "$t" = "substituted" || not
ok "fakechroot substituted pwd (2)"

export FAKECHROOT_CMD_SUBST="/no/file=foo:/other/file=bar"

t=`./fakechroot.sh testtree /bin/pwd`
test "$t" = "/" || not
ok "fakechroot not substituted pwd is /"

rm -rf testtree

end
