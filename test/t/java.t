#!/bin/sh

srcdir=${srcdir:-.}
. $srcdir/common.inc.sh

command -v dpkg >/dev/null 2>&1 || skip_all 'dpkg command is missing'
arch=`dpkg --print-architecture`

JAVA_HOME=/usr/lib/jvm/java-7-openjdk-$arch

command -v javac >/dev/null 2>&1 || skip_all 'javac command is missing (sudo apt-get install openjdk-7-jdk)'
test -d $JAVA_HOME >/dev/null 2>&1 || skip_all "$JAVA_HOME directory is missing (sudo apt-get install openjdk-7-jdk)"

unset JAVA_TOOL_OPTIONS

(
    $JAVA_HOME/bin/java -version
    $JAVA_HOME/bin/javac -version
) 2>&1 | diag
(
    $JAVA_HOME/bin/javac java/Hello.java >/dev/null
    ( cd java && $JAVA_HOME/bin/jar cvfm hello.jar manifest.txt Hello.class )
) >/dev/null 2>&1

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
