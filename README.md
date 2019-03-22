# fakechroot

![logo](https://github.com/dex4er/fakechroot/wiki/img/fakechroot_logo.png)

<!-- markdownlint-disable MD013 -->
**Home** | [Download](https://github.com/fakechroot/fakechroot/wiki/Download) | [Documentation](https://github.com/fakechroot/fakechroot/blob/master/man/fakechroot.pod) | [ChangeLog](https://github.com/fakechroot/fakechroot/blob/master/NEWS.md) | [Development](https://github.com/fakechroot/fakechroot/wiki/Development) | [ToDo](https://github.com/fakechroot/fakechroot/wiki/Todo) | [Links](https://github.com/fakechroot/fakechroot/wiki/Links)

[![Travis CI](https://travis-ci.org/dex4er/fakechroot.png?branch=master)](https://travis-ci.org/dex4er/fakechroot)
[![Salsa GitLab CI](https://salsa.debian.org/dexter/fakechroot/badges/master/pipeline.svg)](https://salsa.debian.org/dexter/fakechroot/commits/master)
<!-- markdownlint-enable MD013 -->

## What is it

fakechroot runs a command in an environment were is additional possibility to
use `chroot`(8) command without root privileges.  This is useful for allowing
users to create own chrooted environment with possibility to install another
packages without need for root privileges.

## How does it work

fakechroot replaces some libc library functions (`chroot`(2), `open`(2), etc.)
by ones that simulate the effect of being called with root privileges.

These wrapper functions are in a shared library `libfakechroot.so` which is
loaded through the `LD_PRELOAD` mechanism of the dynamic loader.  (See
`ld.so`(8))

In fake chroot you can install Debian bootstrap with `debootstrap` command. In
this environment you can use i.e. `apt-get`(8) command to install another
packages. You don't need a special privileges and you can run it from common
user's account.

## An example session

```sh
$ id
uid=1000(dexter) gid=1000(dexter) groups=1000(dexter)

$ fakechroot fakeroot debootstrap sid /tmp/sid
I: Retrieving Release
I: Retrieving Release.gpg
I: Checking Release signature
...
I: Base system installed successfully.

$ fakechroot fakeroot chroot /tmp/sid apt-get install -q hello
Reading package lists...
Building dependency tree...
Reading state information...
The following NEW packages will be installed:
  hello
0 upgraded, 1 newly installed, 0 to remove and 0 not upgraded.
Need to get 57.4 kB of archives.
After this operation, 558 kB of additional disk space will be used.
Get:1 http://ftp.us.debian.org/debian/ sid/main hello amd64 2.8-4 [57.4 kB]
Fetched 57.4 kB in 0s (127 kB/s)
Selecting previously unselected package hello.
(Reading database ... 24594 files and directories currently installed.)
Unpacking hello (from .../archives/hello_2.8-4_amd64.deb) ...
Processing triggers for man-db ...
Processing triggers for install-info ...
Setting up hello (2.8-4) ...

$ fakechroot chroot /tmp/sid hello
Hello, world!
```

## Where is it used

fakechroot is mainly used as:

* a supporter for [debirf](http://cmrg.fifthhorseman.net/wiki/debirf), DEBian on
  Initial Ram Filesystem
* a variant of [debootstrap](http://code.erisian.com.au/Wiki/debootstrap), the
  tool which can set up new Debian or Ubuntu system

fakechroot had found another purposes:

* to be a part of [Klik](http://klik.atekon.de) application installer as
  kfakechroot project
* to be a supporter for [pbuilder](http://pbuilder.alioth.debian.org/) building
  system
* to be a supporter for [Apport](https://wiki.ubuntu.com/Apport) retracer
* to be a supporter for [libguestfs tools](http://libguestfs.org/) for accessing
  and modifying virtual machine disk images
* to be a supporter for
  [febootstrap](http://et.redhat.com/~rjones/febootstrap/), the tool which can
  set up new Fedora system
* to be a part of
  [cuntubuntu](https://play.google.com/store/apps/details?id=com.cuntubuntu) -
  Ubuntu for Android without root
* to be a supporter for
  [selenium-chroot](https://github.com/gagern/selenium-chroot) setup
