#!/bin/sh

# all paths need to be relative to $srcdir variable
srcdir=${srcdir:-.}

# include common script
. $srcdir/common.inc.sh

# prepare tests. how many tests will be?
prepare 2

# general testing rule:
#   for chroot and fakechroot
#     skip if not root for real chroot
#     do_something
#     check_if_it_is_ok || not
#     ok "message"

echo=${ECHO:-/bin/echo}

# the same tests for real chroot and fakechroot
for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        # how many tests we need to skip? usually it is plan/2
        skip $(( $tap_plan / 2 )) "not root"
    else

        # echo something
        t=`$srcdir/$chroot.sh $testtree $echo something 2>&1`
        # check if it is ok or print "not"
        test "$t" = "something" || not
        # print "ok" message with unquoted test output
        ok "$chroot echo:" $t

    fi

done

# clean up temporary directory after tests and end tests
cleanup
