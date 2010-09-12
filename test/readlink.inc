if [ -x /bin/readlink ]; then
    readlink=/bin/readlink
elif [ -x /usr/bin/readlink ]; then
    readlink=/usr/bin/readlink
else
    readlink=readlink
fi
