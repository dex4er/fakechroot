#!/bin/sh
if [ "$(find ~ -maxdepth 0 -printf '%U\n')" != 0 ]; then
    echo "Use in fakechroot environment"
    exit 1
fi
test -f savemode.dat1 && mv -f savemode.dat1 savemode.dat1~
test -f savemode.dat2 && mv -f savemode.dat2 savemode.dat2~
LANG=C find . \( -type b -o -type c -o -type p -o -type s \) \
    | tar czPf savemode.dat1 -T-
find . \( -type f -o -type d -o -type l \) \
    -a \( ! -uid 0 -o ! -gid 0 \) -printf "%U %G %m %p\n" \
    | gzip -9 > savemode.dat2
