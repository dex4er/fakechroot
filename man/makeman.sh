#!/bin/sh

for i in *.pod; do
    eval pod2man $(head -n1 $i | sed 's/^# pod2man //') $i > $(basename $i .pod).$(head -n1 $i | sed 's/.*--section=//; s/ .*//')
done
