![logo](http://fakechroot.alioth.debian.org/img/fakechroot_logo.png)
---

**Home** | [Download](https://github.com/fakechroot/fakechroot/wiki/Download) | [Documentation](https://github.com/fakechroot/fakechroot/blob/master/man/fakechroot.pod) | [ChangeLog](https://github.com/fakechroot/fakechroot/blob/master/NEWS.md) | [Development](https://github.com/fakechroot/fakechroot/wiki/Development) | [ToDo](https://github.com/fakechroot/fakechroot/wiki/Todo) | [Links](https://github.com/fakechroot/fakechroot/wiki/Links)

[![Build Status](https://travis-ci.org/dex4er/fakechroot.png?branch=master)](https://travis-ci.org/dex4er/fakechroot)

What is it?
===========

fakechroot runs a command in an environment were is additional possibility to
use `chroot`(8) command without root privileges.  This is useful for allowing
users to create own chrooted environment with possibility to install another
packages without need for root privileges.


News
====

### 24 Nov 2013

Version 2.17 is released. The `FAKECHROOT_ELFLOADER` environment variable
changes the default dynamic linker. New `env`(1) wrapper was added. It is safe
to use relative paths which won't escape from fake chroot. Fixes were make for
`readline`(2), `mktemp`(3), `_xftw64`(glibc), `__realpath_chk`(glibc)
functions. New functions `mkostemp`(3), `mkostemp64`(3), `mkostemps`(3),
`mkostemps64`(3), `mkstemps`(3) and `mkstemps64`(3) were implemented.

### 11 Dec 2011

Version 2.16 is released. The fakechroot script loads additional environment
settings from configuration directory (`--config-dir` option). By default
additional settings are provided for `chroot`(8) and `debootstrap`(8)
commands. Wrapped `chroot`(8) command loads `ld.so.conf` paths to
`LD_LIBRARY_PATH` environment variable. Fixes were made for `getpeeraddr`(3)
and `getsockaddr`(3) functions.

### 29 Sep 2011

Version 2.15 is released. New function `faccessat`(2) was added: it fixes
`test -r` command. The `popen`(3) function were reimplemented based on OpenBSD
source to prevent some coredumps with newer GNU C Library.


How does it work?
=================

fakechroot replaces more library functions (`chroot`(2), `open`(2), etc.) by
ones that simulate the effect the real library functions would have had, had
the user really been in chroot.  These wrapper functions are in a shared
library `/usr/lib/fakechroot/libfakechroot.so` which is loaded through the
`LD_PRELOAD` mechanism of the dynamic loader.  (See `ld.so`(8))

In fake chroot you can install Debian bootstrap with `debootstrap` command.
In this environment you can use i.e. `apt-get`(8) command to install another
packages from common user's account.


Where is it used?
=================

fakechroot is mainly used as:

* a variant of [debootstrap](http://code.erisian.com.au/Wiki/debootstrap), the tool which can set up new Debian or Ubuntu system.

fakechroot had found another purposes:

* to be a part of [Klik](http://klik.atekon.de) application installer as kfakechroot project
* to be a supporter for [pbuilder](http://pbuilder.alioth.debian.org/) building system
* to be a supporter for [Apport](https://wiki.ubuntu.com/Apport) retracer
* to be a supporter for [libguestfs tools](http://libguestfs.org/) for accessing and modifying virtual machine disk images
* to be a part of [Slind](https://www.slind.org/Main_Page) - a minimal Debian-based distro for embedded devices as libfakechroot-cross project
* to be a supporter for [febootstrap](http://et.redhat.com/~rjones/febootstrap/), the tool which can set up new Fedora system.
* to be a part of [cuntubuntu](https://play.google.com/store/apps/details?id=com.cuntubuntu) - Ubuntu for Android without root
