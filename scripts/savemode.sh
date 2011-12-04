#!/bin/sh

# This script saves uids and gids of files to savemode.dat? files
# which can be restored further with restoremode.sh script
#
# Only files with uid and gid different than current are saved.
#
# It should be started with root privileges or fakeroot command.

test -f savemode.dat1 && mv -f savemode.dat1 savemode.dat1~
test -f savemode.dat2 && mv -f savemode.dat2 savemode.dat2~

uid=`id -u`
gid=`id -g`

LANG=C find . \( -type b -o -type c -o -type p -o -type s \) \
    | tar czPf savemode.dat1 -T-

LANG=C find . \( -type f -o -type d -o -type l \) \
    -a \( ! -uid $uid -o ! -gid $gid \) -printf "%U %G %m %p\n" \
    | gzip -9 > savemode.dat2
