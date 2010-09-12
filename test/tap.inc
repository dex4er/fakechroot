
plan () {
    tap_plan=$1
    tap_ret=0
    tap_n=1
    echo "1..$tap_plan"
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
    for tap_skip_i in `$SEQ 1 $tap_skip_n`; do
        ok "# SKIP $@"
    done
}

end () {
    exit $tap_ret
}

bail_out () {
    echo "Bail out!  $@"
    tap_ret=255
    end
}
