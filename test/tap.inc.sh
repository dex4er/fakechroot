plan () {
    tap_plan=$1
    tap_ret=0
    tap_n=0
    echo "1..$tap_plan"
}

no_plan () {
    tap_plan=0
    tap_ret=0
    tap_n=0
}

skip_all () {
    echo "1..0 # SKIP $@"
    exit 0
}

not () {
    printf "not "
    tap_ret=1
}

ok () {
    tap_n=$(( $tap_n + 1 ))
    echo "ok $tap_n $@"
}

skip () {
    tap_skip_n=$1
    shift
    for tap_skip_i in `$SEQ 1 $tap_skip_n`; do
        ok "# SKIP $@"
    done
}

done_testing () {
    echo "1..$tap_n"
}

end () {
    exit $tap_ret
}

bail_out () {
    echo "Bail out!  $@"
    tap_ret=255
    end
}

note () {
    if [ $# -eq 0 ]; then
        while read l; do
            printf '# %s\n' "$l"
        done
    else
        echo "$@" | while read l; do
            printf '# %s\n' "$l"
        done
    fi
}

diag () {
    note "$@" 1>&2
}
