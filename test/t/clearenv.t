#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

prepare 4

chroot=fakechroot

CLEARED=paranoid
export CLEARED

t=`LC_ALL=C $srcdir/$chroot.sh $testtree /bin/sh -c 'CLEARED=something; echo $CLEARED' 2>&1`
test "$t" = "something" || not
ok "$chroot echo \$CLEARED returns" $t

t=`LC_ALL=C $srcdir/$chroot.sh $testtree /bin/sh -c 'CLEARED=something; test-clearenv "/bin/sh -c \"echo \\\$CLEARED\""' 2>&1`
test -z "$t" || not
ok "$chroot test-clearenv echo \$CLEARED returns" $t

t=`LC_ALL=C $srcdir/$chroot.sh $testtree /bin/sh -c 'test-clearenv "/bin/sh -c \"echo \\\$FAKECHROOT\""' 2>&1`
test "$t" = "true" || not
ok "$chroot test-clearenv echo \$FAKECHROOT returns" $t

t=`LC_ALL=C $srcdir/$chroot.sh $testtree /bin/sh -c 'test-clearenv "/bin/sh -c \"echo \\\$FAKECHROOT_VERSION\""' 2>&1`
test -n "$t" || not
ok "$chroot test-clearenv echo \$FAKECHROOT_VERSION returns" $t

cleanup
