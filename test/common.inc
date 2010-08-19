
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

prepare_env () {
    if [ -f src/.libs/libfakechroot.so ]; then
        dir=`cd \`pwd\`/src/.libs 2>/dev/null && pwd`
    elif [ -f ../src/.libs/libfakechroot.so ]; then
        dir=`cd \`pwd\`/../src/.libs 2>/dev/null && pwd`
    else
        dir=/usr/lib/fakechroot
    fi

    if [ -n "$LD_PRELOAD" ]; then
        LD_PRELOAD="$LD_PRELOAD $dir/libfakechroot.so"
    else
        LD_PRELOAD="$dir/libfakechroot.so"
    fi
    export LD_PRELOAD

    export FAKECHROOT=true
}
