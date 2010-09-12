if [ -x testtree/usr/bin/touch ]; then
    touch=/usr/bin/touch
elif [ -x testtree/bin/touch ]; then
    touch=/bin/touch
else
    touch=
fi
