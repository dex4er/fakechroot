AC_PREREQ(2.64)
AC_INIT([fakechroot], [2.16], [dexter@debian.org], [fakechroot], [http://fakechroot.alioth.debian.org/])
AC_CANONICAL_HOST
AC_CANONICAL_BUILD

AM_INIT_AUTOMAKE([1.10 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/libfakechroot.c])
AC_CONFIG_HEADER([config.h])
AC_LANG([C])

# --disable-static by default
AC_DISABLE_STATIC

# --with-libpath
AC_ARG_WITH([libpath],
    [AS_HELP_STRING([--with-libpath@<:@=LIBPATH@:>@],
        [path list where library might be located @<:@default=$pkglibdir@:>@])],
    [with_libpath=$withval],
    [with_libpath=$libdir/$PACKAGE_NAME])
AS_IF([test "x$with_libpath" = x],
    [AC_MSG_FAILURE([invalid libpath specified])])
AC_SUBST(libpath, $with_libpath)

# Additional C macros
AC_DEFINE([FAKECHROOT], [PACKAGE_VERSION], [FAKECHROOT])

# Checks for programs.
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_LN_S
AC_CHECK_PROGS([PROVE], [prove])
AC_CHECK_PROGS([SEQ], [seq gseq])
AC_PATH_PROG([CHROOT], [chroot], [/usr/sbin/chroot], [/usr/sbin:/sbin:/usr/bin:/bin:$PATH])

# Checks for compilator features
ACX_CHECK_C_ALIGNOF
ACX_CHECK_C_ATTRIBUTE([constructor])
ACX_CHECK_C_ATTRIBUTE_SECTION([data.fakechroot])
ACX_CHECK_C_ATTRIBUTE_VISIBILITY

# Checks for libraries.
AC_CHECK_LIB([dl], [dlsym])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(m4_normalize([
    alloca.h
    dirent.h
    dlfcn.h
    errno.h
    fcntl.h
    fts.h
    ftw.h
    glob.h
    libintl.h
    pwd.h
    shadow.h
    signal.h
    stdarg.h
    stdio.h
    stdlib.h
    string.h
    sys/inotify.h
    sys/mount.h
    sys/param.h
    sys/socket.h
    sys/stat.h
    sys/statfs.h
    sys/statvfs.h
    sys/times.h
    sys/types.h
    sys/un.h
    sys/vfs.h
    sys/wait.h
    sys/xattr.h
    unistd.h
    utime.h
]))

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for additional typedefs.
AC_CHECK_MEMBERS([struct sockaddr_un.sun_len],,, ACX_INCLUDES_HEADERS([sys/un.h]))
AC_CHECK_MEMBERS([struct _ftsent.fts_fts],,, ACX_INCLUDES_HEADERS([sys/types.h sys/stat.h fts.h]))
ACX_CHECK_FTS_NAME_TYPE

# Checks for library functions.
AC_CHECK_FUNCS(m4_normalize([
    __chk_fail
    __fxstatat
    __fxstatat64
    __getcwd_chk
    __getwd_chk
    __lxstat
    __lxstat64
    __open
    __open_2
    __open64
    __open64_2
    __openat_2
    __openat64_2
    __opendir2
    __realpath_chk
    __readlink_chk
    __readlinkat_chk
    __statfs
    __xmknod
    __xmknodat
    __xstat
    __xstat64
    _xftw
    _xftw64
    access
    acct
    bind
    bindtextdomain
    canonicalize_file_name
    chdir
    chmod
    chown
    chroot
    connect
    creat
    creat64
    dlmopen
    dlopen
    eaccess
    euidaccess
    execl
    execle
    execlp
    execv
    execve
    execvp
    faccessat
    fchmodat
    fchownat
    fopen
    fopen64
    freopen
    freopen64
    fts_children
    fts_open
    fts_read
    ftw
    ftw64
    futimesat
    get_current_dir_name
    getcwd
    getpeername
    getsockname
    getwd
    getxattr
    glob
    glob64
    glob_pattern_p
    inotify_add_watch
    lchmod
    lchown
    lckpwdf
    lgetxattr
    link
    linkat
    listxattr
    llistxattr
    lremovexattr
    lsetxattr
    lstat
    lstat64
    lutimes
    mempcpy
    mkdir
    mkdirat
    mkdtemp
    mknod
    mknodat
    mkfifo
    mkfifoat
    mkstemp
    mkstemp64
    mktemp
    nftw
    nftw64
    open
    open64
    openat
    openat64
    opendir
    pathconf
    popen
    rawmemchr
    readlink
    readlinkat
    realpath
    remove
    removexattr
    rename
    renameat
    revoke
    rmdir
    scandir
    scandir64
    setenv
    setxattr
    stat
    stat64
    statfs
    statfs64
    statvfs
    statvfs64
    stpcpy
    strchrnul
    symlink
    symlinkat
    system
    tempnam
    tmpnam
    truncate
    truncate64
    unlink
    unlinkat
    unsetenv
    ulckpwdf
    utime
    utimensat
    utimes
    vfork
]))
AC_FUNC_ALLOCA
ASX_NO_CROSS_COMPILING([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])

# Check for internal dependencies
ACX_CHECK_OPENDIR_CALLS_INTERNALLY([__open])
ACX_CHECK_OPENDIR_CALLS_INTERNALLY([__opendir2])

# Check return type and arguments types for functions
m4_foreach_w([function], [bind connect],
    [ACX_CHECK_FUNC_ARGTYPES(function,
        [
#define _GNU_SOURCE
        ], [sys/types.h sys/socket.h],
        [[int], [int _], [const struct sockaddr *_], [socklen_t _]],
        [[int]],
        [[int _]],
        [[__CONST_SOCKADDR_ARG _], [const struct sockaddr *_]],
        [[socklen_t _]])])

m4_foreach_w([function], [getpeername getsockname],
    [ACX_CHECK_FUNC_ARGTYPES(function,
        [
#define _GNU_SOURCE
        ], [sys/types.h sys/socket.h],
        [[int], [int _], [struct sockaddr *_], [socklen_t _]],
        [[int]],
        [[int _]],
        [[__SOCKADDR_ARG _], [struct sockaddr *_]],
        [[socklen_t *__restrict _], [socket_t *_]])])

ACX_CHECK_FUNC_ARGTYPES([fts_open],
    [], [sys/types.h sys/stat.h fts.h],
    [[FTS *], [char * const *_], [int], [int (*_)(const FTSENT **, const FTSENT **)]],
    [[FTS *]],
    [[char * const *_]],
    [[int _]],
    [[int (*_)(const FTSENT * const *, const FTSENT * const *)], [int (*_)(const FTSENT **, const FTSENT **)]])

ACX_CHECK_FUNC_ARGTYPES([readlink],
    [
#define _BSD_SOURCE 1
    ], [unistd.h],
    [[ssize_t], [const char *_], [char *_], [size_t _]],
    [[ssize_t], [int]],
    [[const char *_]],
    [[char *_]],
    [[size_t _], [int _]])

ACX_CHECK_FUNC_ARGTYPES([scandir],
    [
#define _BSD_SOURCE 1
    ], [dirent.h],
    [[int], [const char *_], [struct dirent ***_], [int(*_)(const struct dirent *)], [int(*_)(const void *,const void *)]],
    [[int]],
    [[const char *_]],
    [[struct dirent ***_]],
    [[int(*_)(const struct dirent *)], [int(*_)(struct dirent *)]],
    [[int(*_)(const struct dirent **, const struct dirent **)], [int(*_)(const void *,const void *)]])

ACX_CHECK_FUNC_ARGTYPES([scandir64],
    [
#define _BSD_SOURCE 1
#define _GNU_SOURCE 1
    ], [dirent.h],
    [[int], [const char *_], [struct dirent64 ***_], [int(*_)(const struct dirent64 *)], [int(*_)(const void *,const void *)]],
    [[int]],
    [[const char *_]],
    [[struct dirent64 ***_]],
    [[int(*_)(const struct dirent64 *)], [int(*_)(struct dirent64 *)]],
    [[int(*_)(const struct dirent64 **, const struct dirent64 **)], [int(*_)(const void *,const void *)]])

ACX_CHECK_FUNC_ARGTYPES([utimes],
    [], [sys/time.h],
    [[int], [const char *_], [const struct timeval _@<:@2@:>@]],
    [[int]],
    [[const char *_]],
    [[const struct timeval _@<:@2@:>@], [struct timeval _@<:@2@:>@]])

# List of Makefiles
AC_CONFIG_FILES(m4_normalize([
    Makefile
    man/Makefile
    src/Makefile
    scripts/Makefile
    test/Makefile
    test/src/Makefile
]))

# Generate output
AC_OUTPUT
