![logo](http://fakechroot.alioth.debian.org/img/fakechroot_logo.png)
---

**Home** | [Download](https://github.com/fakechroot/fakechroot/wiki/Download) | [Documentation](https://github.com/fakechroot/fakechroot/blob/master/man/fakechroot.pod) | [ChangeLog](https://github.com/fakechroot/fakechroot/blob/master/NEWS.md) | [Development](https://github.com/fakechroot/fakechroot/wiki/Development) | [ToDo](https://github.com/fakechroot/fakechroot/wiki/Todo) | [Links](https://github.com/fakechroot/fakechroot/wiki/Links)

[![Build Status](https://travis-ci.org/dex4er/fakechroot.png?branch=master)](https://travis-ci.org/dex4er/fakechroot)

# What is it?

fakechroot runs a command in an environment were is additional possibility to
use `chroot`(8) command without root privileges.  This is useful for allowing
users to create own chrooted environment with possibility to install another
packages without need for root privileges.

# News

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

### 16 Dec 2010

Version 2.14 is released. The source code was refactored: all functions was
moved to separated files. The `opendir`(3) function is compiled only if it
doesn't call other function internally.  It fixes `opendir`(3), `fts_open`(3)
and `ftw`(3) functions. The `fts_*`(3) functions were reimplemented based on
OpenBSD source. The `__opendir2`(3) function was reimplemented based on
FreeBSD source. Fixes were made for older GNU C Library.

# How does it work?

fakechroot replaces more library functions (`chroot`(2), `open`(2), etc.) by
ones that simulate the effect the real library functions would have had, had
the user really been in chroot.  These wrapper functions are in a shared
library `/usr/lib/fakechroot/libfakechroot.so` which is loaded through the
`LD_PRELOAD` mechanism of the dynamic loader.  (See `ld.so`(8))

In fake chroot you can install Debian bootstrap with `debootstrap
--variant=fakechroot` command.  In this environment you can use i.e.
`apt-get`(8) command to install another packages from common user's account.

# Where is it used?

fakechroot is mainly used as:

* a variant of [debootstrap](http://code.erisian.com.au/Wiki/debootstrap), the tool which can set up new Debian or Ubuntu system.
* a helper for [febootstrap](http://et.redhat.com/~rjones/febootstrap/), the tool which can set up new Fedora system.

fakechroot had found another purposes:

* to be a part of [Klik](http://klik.atekon.de) application installer as kfakechroot project
* to be a supporter for [pbuilder](http://pbuilder.alioth.debian.org/) building system
* to be a supporter for [Apport](https://wiki.ubuntu.com/Apport) retracer
* to be a supporter for [libguestfs tools](http://libguestfs.org/) for accessing and modifying virtual machine disk images
* to be a part of [Slind](https://www.slind.org/Main_Page) - a minimal Debian-based distro for embedded devices as libfakechroot-cross project
* to be a part of [cuntubuntu](https://play.google.com/store/apps/details?id=com.cuntubuntu) - Ubuntu for Android without root
