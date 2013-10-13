seq_sh () {
    if [ -n "$2" ]; then
        seq_first=$1
        seq_last=$2
    elif [ -n "$1" ]; then
        seq_first=1
        seq_last=$1
    else
        echo seq: missing argument 1>&2
        exit 1
    fi

    seq_n=$seq_first
    while [ $seq_n -le $seq_last ]; do
        echo $seq_n
        seq_n=$(( $seq_n + 1 ))
    done
}

SEQ=${SEQ:-seq_sh}
