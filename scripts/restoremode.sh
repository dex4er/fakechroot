#!/bin/sh
if [ "$(find ~ -maxdepth 0 -printf '%U\n')" != 0 ]; then
    echo "Use in fakechroot environment"
    exit 1
fi
tar zxf savemode.dat1 --numeric-owner
zcat savemode.dat2 | while read uid gid mode file; do
    chown $uid:$gid $file
    chmod $mode $file
done
