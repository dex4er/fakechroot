#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc

prepare 4

fakedir=`cd testtree; pwd -P`

for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        skip $(( $tap_plan / 2 )) "not root"
    else

	cat >> testtree/bin/execlp-test <<EOF
#!/bin/bash

echo "Magic bees"
EOF
	chmod a+x testtree/bin/execlp-test

	correct_output=`testtree/bin/execlp-test`

	t=`$srcdir/$chroot.sh testtree execlp-test`
	test "$t" = "$correct_output" || not
	ok "$chroot execlp test script executes ok in shell"

	t=`$srcdir/$chroot.sh testtree test-execlp`
	test "$t" = "$correct_output" || not
	ok "$chroot execlp test script executes ok from execlp()"

    fi
done

cleanup
