#!/bin/sh

# This script restores uids and gids of files saved previously
# with savemode.sh script

tar zxf savemode.dat1 --numeric-owner

zcat savemode.dat2 | while read uid gid mode file; do
    chown $uid:$gid $file
    chmod $mode $file
done
