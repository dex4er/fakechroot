#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

command -v dpkg >/dev/null 2>&1 || skip_all 'dpkg command is missing'
arch=`dpkg --print-architecture`

command -v javac >/dev/null 2>&1 || skip_all 'javac command is missing (sudo apt-get install openjdk-7-jdk)'
test -d /usr/lib/jvm/java-7-openjdk-$arch >/dev/null 2>&1 || skip_all "/usr/lib/jvm/java-7-openjdk-$arch directory is missing (sudo apt-get install openjdk-7-jdk)"

unset JAVA_TOOL_OPTIONS

(
    set -x
    java -version
    javac -version
    javac java/Hello.java
    ( cd java && jar cvfm hello.jar manifest.txt Hello.class )
) 2>&1 | while read line; do
    echo "# $line"
done

test -f java/hello.jar >/dev/null 2>&1 || skip_all "java/hello.jar is missing: some problem with java compiler"

prepare 1

mkdir -p $testtree/usr/lib/jvm
cp -pfR /usr/lib/jvm/java-7-openjdk-$arch $testtree/usr/lib/jvm
ln -sf /usr/lib/jvm/java-7-openjdk-$arch/bin/java $testtree/usr/bin/java
cp -pf java/hello.jar $testtree/tmp

# java doesn't like non-available /proc file system so we don't try to run as root

t=`$srcdir/fakechroot.sh $testtree java -jar /tmp/hello.jar 2>&1`
test "$t" = "Hello world!" || not
ok "fakechroot java -jar hello.jar:" $t

rm -f java/*.class java/*.jar

cleanup
