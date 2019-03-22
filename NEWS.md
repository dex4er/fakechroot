# NEWS

## Version 2.20.1

22 Mar 2019

* Fixed problem when tests couldn't be started with root user and glibc 2.28.
* Fixed problem with too verbose test for `debootstrap`(8) command.
* Fixed problem with tests for `fts_*` functions when started on `btrfs`
  filesystem.
* The `debootstrap`(8) command honors `FAKECHROOT_EXTRA_LIBRARY_PATH`
  environment variable too.
* Workaround has been added for `systemd` package installed by
  `debootstrap`(8) command.
* Better support for link-time optimizer. Run
  `./configure EXTRA_CFLAGS='-Wall -flto' EXTRA_LDFLAGS='-flto' AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib`
  to use it.

## Version 2.20

17 Mar 2019

* The `fts64_*` functions were added: glibc supports them since 2.23.
* The `renameat2`(2) function were added: glibc supports it since 2.28
  and `mv` command from coreutils 8.30 uses it.
* The `chroot`(8) wrapper and `fakechroot`(1) command can work with POSIX
  shell.
* The `chroot`(8) wrapper allows to chroot into root directory.
* The `chroot`(8) wrapper appends directories from
  `FAKECHROOT_EXTRA_LIBRARY_PATH` environment variable because some commands
  use runpath. The default value is `/lib/systemd:/usr/lib/man-db` for
  `systemctl`(1) and `man`(1) commands.
* The `ldd`(1) wrapper doesn't lose a leading slash in absolute paths.
* The `fakechroot`(1) command unsets `CDPATH` environment variable and swaps
  `libfakeroot` and `libfakechroot` in `LD_PRELOAD` environment variable if
  needed.
* Can be compiled with gcc 7.1 `-Wformat-truncation`.
* Can be compiled with clang 5.0 `-O2` and `-std=c11`.

## Version 2.19

20 Nov 2016

* The `FAKECHROOT_EXCLUDE_PATH` list has to contain at most 100 elements.
* The `env`(1) wrapper works with dash.
* The `fakeroot`(1) works if `chroot`(8) is invoked twice.
* Fixes were made for `chdir`(2). It was possible to change to a directory
  outside of fakechroot.
* The `fakechroot`(1) command sets `FAKECHROOT_CMD_ORIG` environment variable
  for wrapped command.
* The `ldd`(1) wrapper distinguishes different paths with the same beginning.
* The `ldd`(1) wrapper does not fail if the path is not existing outside
  fakechroot.
* Can be compiled with clang 4.0.

## Version 2.18

26 Oct 2015

* The `fakechroot`(1) command accepts new option `--bindir`.
* The `chfn`(1) command runs correctly on Ubuntu vidid and Debian stretch.
* The `env`(1) wrapper works correctly if there is variable with multilined
  content.
* New functions `ldaddr`(3) and `dl_iterate_phdr`(3) were added. The
  `dlopen`(3) function was fixed. The `java`(1) command should work correctly.
* New functions `posix_spawn`(3) and `posix_spawnp`(3) were added. Some new
  applications like `clang`(1) should run properly.
* Fixes were made for `lstat`(2) and `lstat64`(2) functions. The ending
  slash is not removed by normalization of a path name.
* Fixes were made for `readlink`(2) function. The `fakechroot`(1) command runs
  properly with `libjemalloc` library.
* The tilde `~` character in `FAKECHROOT_*` variables is not expanded anymore.
* Fixes were made for `getpeeraddr`(3) and `getsockaddr`(3) functions.

## Version 2.17.2

24 Dec 2013

* Fixes were made for `fakechroot`(1) command with `-h` option.
* The path for function `chroot`(2) is sanitized so it works correctly when
  path is ended with `/`.
* The `make -n test` was fixed and doesn't call any command.

## Version 2.17.1

5 Dec 2013

* The `fakechroot`(1) command runs proper wrapper rather than original
  command, if it is listed on `FAKECHROOT_CMD_SUBST` variable.
* Fixes were made for `chroot`(8) wrapper. It scanned /etc/ld.so.conf
  incorrectly and now enters to physical path, avoiding symlinks.
* The `chroot`(2) function allows to use more than 2048 bytes for
  `LD_LIBRARY_PATH` environment variable.

## Version 2.17

24 Nov 2013

* The `FAKECHROOT_ELFLOADER` environment variable changes the default dynamic
  linker.  The `fakechroot` script provides `--elfloader` option.
* The `FAKECHROOT_EXCLUDE_PATH` environment variable overrides the default
  settings.
* The `ldd`(1) wrapper can work if overridden with `FAKECHROOT_CMD_SUBST`
  environment variable.
* New `env`(1) wrapper was added.  It preserves fakechroot environment even
  for `--ignore-environment` option.
* The special environment `none` means that no environment settings are loaded
  at all.
* The function `setenv`(3) and `unsetenv`(3) was reimplemented, to prevent
  problems with binaries which brings own implementation of these functions.
* New function `clearenv`(3) was added.  It preserves fakechroot environment.
* It is safe to use relative paths which won't escape from fake chroot.
* Fixes were made for `readline`(2) function if destination path is similar to
  `FAKECHROOT_PATH`.
* Fixes were made for `mktemp`(3) function if used on a path in
  `FAKECHROOT_EXCLUDE_PATHS`.
* The `_xftw64`(glibc) function is reenabled. It had wrong wrapper.
* Fixes were made for `__realpath_chk`(glibc) function when `__chk_fail`
  function is missing.
* New functions `mkostemp`(3), `mkostemp64`(3), `mkostemps`(3),
  `mkostemps64`(3), `mkstemps`(3) and `mkstemps64`(3) were implemented.
  It fixes `sed -i` command.

## Version 2.16

11 Dec 2011

* The fakechroot script loads additional environment settings from
  configuration directory (`--config-dir` option). By default additional
  settings are provided for `chroot`(8) and `debootstrap`(8) commands.
* Wrapped `chroot`(8) command loads `ld.so.conf` paths to `LD_LIBRARY_PATH`
  environment variable.
* The `debootstrap`(8) command works with default, buildd and minbase
  variants.
* Fixes were made for `getpeeraddr`(3) and `getsockaddr`(3) functions.
  `lwp-request` command is working correctly.

## Version 2.15

29 Sep 2011

* New function `faccessat`(2) was added.  It fixes `test -r` command.
  Thanks to Johannes Schauer.
* The `popen`(3) function were reimplemented based on OpenBSD source to
  prevent some coredumps with newer GNU C Library.

## Version 2.14

18 Dec 2010

* The source code was refactored: all functions was moved to separated files.
* The `opendir`(3) function is compiled only if it doesn't call other function
  internally.  It fixes `opendir`(3), `fts_open`(3) and `ftw`(3) functions.
* The `fts_*`(3) functions were reimplemented based on OpenBSD source.
* The `ftw_*`(3) functions were reimplemented based on GNU C Library source.
* The `__opendir2`(3) function was reimplemented based on FreeBSD source.
* Fixes were made for older GNU C Library and for cross-compiling environment.

## Version 2.13.1

30 Nov 2010

* Fixes were made for older GNU C Library.

## Version 2.13

29 Nov 2010

* Fixes were made for `realpath`(3) and `ftw`(3) functions.  `apt-ftparchive`
  command currently is working.
* Fixes were made for `canonicalize_file_name`(3), `ftw64`(3), `nftw`(3) and
  `nftw64`(3) functions.
* New functions `fts_children`(3) and `fts_read`(3) were added.
* Lazy dynamic symbol loading was implemented.  Thanks to Alexander Shishkin.
* Fixed bug when `FAKECHROOT_EXCLUDE_PATH` list was sometimes ignored.
* All source code is now on LGPL license again.

## Version 2.12

10 Nov 2010

* New function `execlp`(3).  Thanks to Robert Spanton.
* New functions `statfs`(2) and `statvfs`(2).  Thanks to Paweł Stawicki.
* Better support for GNU/kFreeBSD was made.

## Version 2.11

12 Sep 2010

* Fixes were made for `getpeername`(2) and `getsockname`(2) functions.
  `host 127.0.0.1` command currently is working.  Thanks to Daniel Tschan.
* Fixes were made for `canonicalize_file_name`(3) function.  `man-db` command
  currently is working.
* New functions `__*_chk`(3) were added for GNU libc with fortify `level > 0`.
* New environment variable `FAKECHROOT_AF_UNIX_PATH` defines an optional
  prefix for unix sockets.
* Better support for FreeBSD was made.
* Autoconf scripts were refactored.
* More test units were added.

## Version 2.10

25 Aug 2010

* Handle properly GNU libc `open`(2), `open64`(2), `openat`(2) and
  `openat64`(2) inline functions.
* Compatibility were improved for `scandir`(3) and `scandir64`(3) functions
  on the latest GNU libc.
* Fixed `lstat`(2) function returns correct `st_size`.  Thanks to Daniel
  Baumann.
* Fixed `readlink`(2) function.  Correctly truncates content if buffer is too
  small.
* Fixed `undefined symbol: rpl_malloc` error for compiled 32-bit library on
  64-bit architecture by dropping `AC_FUNC_MALLOC` in `configure.ac`.
* Fixed `chroot`(2) function for path which contains dot character.  Thanks to
  Robie Basak.
* New function `utimensat`(2) fixes `cp` command on Debian sid.
* New functions `linkat`(2), `mknodat`(2), `mkfifoat`(2), `readlinkat`(2)
  and `symlinkat`(2).
* New functions `popen`(3) and `system`(3) for GNU libc.
* `dpkg-shlibdeps(1)` works again because `cd $FAKECHROOT_BASE` does not
  change current working directory to root directory.
* New `FAKECHROOT_CMD_SUBST` environment variable handles a list of
  substituted commands.  Thanks to Richard W.M. Jones.
* New scripts ldd and ldconfig with .fakechroot suffix are installed.
* New `make test` target uses Perl's `prove` tool to run test units.

## Version 2.9

31 Mar 2009

* Fixes were made for `getpeername`(2) and `getsockname`(2) functions.  Thanks to
  Axel Thimm and Fedora people.
* Fixes were made for `execve`(2):
  * Always copy necessary variables to new environment.
  * Do not make duplicates of environment variables.  Thanks to Richard
    W.M. Jones.
* Fixed were made for `chroot`(2).  It is possible to escape chroot.  Thanks
  to Richard W.M. Jones.
* Fixed were made for `mktemp`(2).  There was a buffer overflow.  Thanks to
  Mikhail Gusarov.
* New function: `futimesat`(2).  Fixes `touch -m` command.
* New functions: `bindtextdomain`(3), `inotify_add_watch`(2).
* More test units was added, `make check` works as expected.

## Version 2.8

25 Jul 2008

* Fixes were made for `__fxstatat64`(3) function which broke `chown` command
  on i386 architecture.
* Better support for FreeBSD was made.

## Version 2.7.1

17 Jul 2008

* Compiles with older GNU libc: readlink type of return value is detected.
* Compiles with uClibc: existence of getwd function is detected.

## Version 2.7

15 Jul 2008

* Stability and compatibility were improved for the latest libc.
* Fixes were made for latest version of coreutils.
* Fixed `readlink`(2) function to be ssize_t as it is in newer libc.
* New functions: `__fxstatat`(2), `__fxstatat64`(2), `fchmodat`(2),
  `fchownat`(2), `__openat`(2), `__openat64`(2), `unlinkat`(2).  It fixes
  last coreutils.
* New functions: `mkdirat`(2), `renameat`(2).
* Fixed `chroot`(2) function to not change current working directory.
* Fixed `chroot`(2) function to handle relative path.
* Fixed `execve`(2) function to not expand argv0 and handle `#!` correctly.
* New `eaccess`(3) function backported from Klik.
* New functions: `bind`(2), `connect`(2), `getpeername`(2), `getsockname`(2).
  They support `PF_UNIX` sockets.
* More memory allocation for fakechroot_init.

## Version 2.6

5 May 2007

* New environment variable `FAKECHROOT_EXCLUDE_PATH`.
* Fixed `getcwd`(3).  Unbreaks `cd` command.
* Fixed `readlink`(2) function.  Unbreaks `readlink` command.
* Fixed `mktemp`(3) function.  Unbreaks `patch` and `ranlib` command.
* The `chroot`(2) function is now recursive and allows nested chroots.
