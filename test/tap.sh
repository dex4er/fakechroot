
plan () {
    tap_ret=0
    tap_n=1
    echo "1..$1"
}

not () {
    printf "not "
    tap_ret=1
}

ok () {
    echo "ok $tap_n $@"
    tap_n=$(( $tap_n + 1 ))
}

skip () {
    tap_skip_n=$1
    shift
    for tap_skip_i in `seq 1 $tap_skip_n`; do
        ok "# SKIP $@"
    done
}

end () {
    exit $tap_ret
}
