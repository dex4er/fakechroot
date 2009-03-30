
plan () {
    ret=0
    n=1
    echo "1..$1"
}

not () {
    printf "not "
    ret=1
}

ok () {
    echo "ok $n $@"
    n=$(($n+1))
}

skip () {
    skip_n=$1
    shift
    for skip_i in `seq 1 $skip_n`; do
        ok "# SKIP $@"
    done
}

end () {
    exit $ret
}
