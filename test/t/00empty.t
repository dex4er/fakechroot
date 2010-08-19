#!/bin/sh

# all paths need to be relative to $srcdir variable
srcdir=${srcdir:-.}

# include common script
. $srcdir/common.inc

# how many tests will be?
plan 4

# clean up temporary directory before tests
rm -rf testtree

# general testing rule:
#   do_something
#   check_if_it_is_ok || not
#   ok "message"

# make first-level testtree
$srcdir/testtree.sh testtree
test "`cat testtree/CHROOT 2>&1`" = "testtree" || not
ok "testtree"

# make deeper test tree
$srcdir/testtree.sh testtree/testtree
test "`cat testtree/testtree/CHROOT 2>&1`" = "testtree/testtree" || not
ok "testtree/testtree"

# the same tests for real chroot and fakechroot
for chroot in chroot fakechroot; do

    if [ $chroot = "chroot" ] && ! is_root; then
        # how many tests we need to skip?
        skip 1 "not root"
    else

        # do something
        t=`echo something 2>&1`
        # check if it is ok or print "not"
        test "$t" = "something" || not
        # print "ok" message
        ok "$chroot something"

    fi

done

# clean up temporary directory after tests
rm -rf testtree

# end message
end
