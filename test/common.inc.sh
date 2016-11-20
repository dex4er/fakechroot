TRUE=0
FALSE=1

LANG=C
LC_ALL=C

testtree=testtree-`basename $0 .t`

is_root () {
    if [ `id -u` = 0 ] && [ -z "$FAKEROOTKEY" ]; then
        return $TRUE
    else
        return $FALSE
    fi
}

cleanup () {
    test -n "$TEST_NO_CLEANUP" && ! test "$TEST_NO_CLEANUP" = 0 || rm -rf $testtree

    end
}

prepare () {
    plan $1

    rm -rf $testtree
    "$srcdir/testtree.sh" $testtree
    test "`cat $testtree/CHROOT 2>&1`" = "$testtree" || bail_out "cannot create $testtree"

    unset FAKECHROOT_CMD_SUBST FAKECHROOT_DEBUG FAKECHROOT_EXCLUDE_PATH
}

. "$srcdir/seq.inc.sh"
. "$srcdir/tap.inc.sh"
