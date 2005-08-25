# $Id$

AC_PREREQ(2.59)
AC_INIT([fakechroot], [2.1], [dexter@debian.org])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/libfakechroot.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_LN_S

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([ \
fcntl.h \
shadow.h \
stdlib.h \
string.h \
unistd.h \
utime.h \
sys/xattr.h \
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CHOWN
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([ \
__lxstat \
__lxstat64 \
__open \
__open64 \
__xmknod \
__xstat \
__xstat64 \
access \
acct \
canonicalize_file_name \
chdir \
chmod \
chown \
chroot \
creat \
creat64 \
dlmopen \
dlopen \
euidaccess \
execl \
execle \
execlp \
execv \
execve \
execvp \
fopen \
fopen64 \
freopen \
freopen64 \
get_current_dir_name \
getcwd \
getwd \
getxattr \
glob \
glob64 \
glob_pattern_p \
lchmod \
lchown \
lckpwdf \
lgetxattr \
link \
listxattr \
llistxattr \
lremovexattr \
lsetxattr \
lstat \
lstat64 \
lutimes \
mkdir \
mkdtemp \
mknod \
mkfifo \
mkstemp \
mkstemp64 \
mktemp \
open \
open64 \
opendir \
pathconf \
readlink \
realpath \
remove \
removexattr \
rename \
revoke \
rmdir \
scandir \
scandir64 \
setxattr \
stat \
stat64 \
symlink \
tempnam \
tmpnam \
truncate \
truncate64 \
unlink \
ulckpwdf \
utime \
utimes \
])

AC_CONFIG_FILES([ \
Makefile \
doc/Makefile \
man/Makefile \
src/Makefile \
scripts/Makefile \
test/Makefile \
])
AC_OUTPUT
