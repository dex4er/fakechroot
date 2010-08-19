
. $srcdir/tap.inc

TRUE=0
FALSE=1

is_root () {
    if [ `id -u` = 0 ] && [ -z "$FAKEROOTKEY" ]; then
        return $TRUE
    else
        return $FALSE
    fi
}
