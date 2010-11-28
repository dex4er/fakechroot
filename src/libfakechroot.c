/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2003, 2005, 2007, 2008, 2009, 2010 Piotr Roszatycki
    <dexter@debian.org>
    Copyright (c) 2007 Mark Eichin <eichin@metacarta.com>
    Copyright (c) 2006, 2007 Alexander Shishkin <virtuoso@slind.org>

    klik2 support -- give direct access to a list of directories
    Copyright (c) 2006, 2007 Lionel Tricon <lionel.tricon@free.fr>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <config.h>

#define _ATFILE_SOURCE
#define _GNU_SOURCE
#define __BSD_VISIBLE

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/times.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <glob.h>
#include <utime.h>
#include <pwd.h>
#include <signal.h>
#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif
#ifdef HAVE_FTS_H
#include <fts.h>
#endif
#ifdef HAVE_FTW_H
#include <ftw.h>
#endif
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif
#include <sys/wait.h>
#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#if defined(PATH_MAX)
#define FAKECHROOT_PATH_MAX PATH_MAX
#elif defined(_POSIX_PATH_MAX)
#define FAKECHROOT_PATH_MAX _POSIX_PATH_MAX
#elif defined(MAXPATHLEN)
#define FAKECHROOT_PATH_MAX MAXPATHLEN
#else
#define FAKECHROOT_PATH_MAX 2048
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#ifdef AF_UNIX
#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif
#endif

#define SOCKADDR_UN(addr) (struct sockaddr_un *)(addr)
#define SOCKADDR_UN_UNION(addr) (struct sockaddr_un *)((addr).__sockaddr_un__)

#if __USE_FORTIFY_LEVEL > 0 && defined __extern_always_inline && defined __va_arg_pack_len
#define USE_ALIAS 1
#endif

#ifndef __set_errno
# define __set_errno(e) (errno = (e))
#endif

#ifndef HAVE_VFORK
# define vfork fork
#endif

#define narrow_chroot_path(path, fakechroot_path, fakechroot_ptr) \
    { \
        if ((path) != NULL && *((char *)(path)) != '\0') { \
            fakechroot_path = getenv("FAKECHROOT_BASE"); \
            if (fakechroot_path != NULL) { \
                fakechroot_ptr = strstr((path), fakechroot_path); \
                if (fakechroot_ptr == (path)) { \
                    if (strlen((path)) == strlen(fakechroot_path)) { \
                        ((char *)(path))[0] = '/'; \
                        ((char *)(path))[1] = '\0'; \
                    } else { \
                        memmove((void*)(path), (path)+strlen(fakechroot_path), 1+strlen((path))-strlen(fakechroot_path)); \
                    } \
                } \
            } \
        } \
    }

#define expand_chroot_path(path, fakechroot_path, fakechroot_buf) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL && *((char *)(path)) == '/') { \
                fakechroot_path = getenv("FAKECHROOT_BASE"); \
                if (fakechroot_path != NULL) { \
                    strcpy(fakechroot_buf, fakechroot_path); \
                    strcat(fakechroot_buf, (path)); \
                    (path) = fakechroot_buf; \
                } \
            } \
        } \
    }

#define expand_chroot_path_malloc(path, fakechroot_path, fakechroot_buf) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL && *((char *)(path)) == '/') { \
                fakechroot_path = getenv("FAKECHROOT_BASE"); \
                if (fakechroot_path != NULL) { \
                    if ((fakechroot_buf = malloc(strlen(fakechroot_path)+strlen(path)+1)) == NULL) { \
                        errno = ENOMEM; \
                        return NULL; \
                    } \
                    strcpy(fakechroot_buf, fakechroot_path); \
                    strcat(fakechroot_buf, (path)); \
                    (path) = fakechroot_buf; \
                } \
            } \
        } \
    }

typedef void (*fakechroot_wrapperfn_t)(void);

struct fakechroot_wrapper {
    fakechroot_wrapperfn_t func;
    fakechroot_wrapperfn_t nextfunc;
    const char *name;
};

#define wrapper_proto(function, return_type, arguments) \
    extern return_type function arguments; \
    typedef return_type (*fakechroot_##function##_fn_t) arguments; \
    struct fakechroot_wrapper fakechroot_##function##_wrapper_decl __attribute__((section("data.fakechroot"))) = { \
        .func = (fakechroot_wrapperfn_t) function, \
        .nextfunc = NULL, \
        .name = #function \
    };

#define nextcall(function) \
    ( \
      (fakechroot_##function##_fn_t)( \
          fakechroot_##function##_wrapper_decl.nextfunc ? \
          fakechroot_##function##_wrapper_decl.nextfunc : \
          fakechroot_loadfunc(&fakechroot_##function##_wrapper_decl) \
      ) \
    )

#ifndef __GLIBC__
extern char **environ;
#endif



/* Useful to exclude a list of directories or files */
static char *exclude_list[32];
static int exclude_length[32];
static int list_max = 0;
static int first = 0;
static char *home_path=NULL;


static int debug (const char *fmt, ...)
{
    va_list ap;
    int ret;
    char newfmt[2048];

    if (!getenv("FAKECHROOT_DEBUG"))
        return 0;

    sprintf(newfmt, PACKAGE ": %s\n", fmt);

    va_start(ap, fmt);
    ret = vfprintf(stderr, newfmt, ap);
    va_end(ap);

    return ret;
}


#ifndef HAVE_STRCHRNUL
/* strchrnul function taken from GNU C Library.
   Copyright (C) 1991,1993-1997,99,2000,2005 Free Software Foundation, Inc.
 */
/* Find the first occurrence of C in S or the final NUL byte.  */
static char * strchrnul (const char *s, int c_in)
{
    const unsigned char *char_ptr;
    const unsigned long int *longword_ptr;
    unsigned long int longword, magic_bits, charmask;
    unsigned char c;

    c = (unsigned char) c_in;

    /* Handle the first few characters by reading one character at a time.
       Do this until CHAR_PTR is aligned on a longword boundary.  */
    for (char_ptr = (unsigned char *)s; ((unsigned long int) char_ptr
                        & (sizeof(longword) - 1)) != 0; ++char_ptr)
        if (*char_ptr == c || *char_ptr == '\0')
            return (void *) char_ptr;

    /* All these elucidatory comments refer to 4-byte longwords,
       but the theory applies equally well to 8-byte longwords.  */

    longword_ptr = (unsigned long int *) char_ptr;

    /* Bits 31, 24, 16, and 8 of this number are zero.  Call these bits
       the "holes."  Note that there is a hole just to the left of
       each byte, with an extra at the end:

       bits:  01111110 11111110 11111110 11111111
       bytes: AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD

       The 1-bits make sure that carries propagate to the next 0-bit.
       The 0-bits provide holes for carries to fall into.  */
    switch (sizeof(longword)) {
    case 4:
        magic_bits = 0x7efefeffL;
        break;
    case 8:
        magic_bits = ((0x7efefefeL << 16) << 16) | 0xfefefeffL;
        break;
    default:
        abort();
    }

    /* Set up a longword, each of whose bytes is C.  */
    charmask = c | (c << 8);
    charmask |= charmask << 16;
    if (sizeof(longword) > 4)
        /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
        charmask |= (charmask << 16) << 16;
    if (sizeof(longword) > 8)
        abort();

    /* Instead of the traditional loop which tests each character,
       we will test a longword at a time.  The tricky part is testing
       if *any of the four* bytes in the longword in question are zero.  */
    for (;;) {
        /* We tentatively exit the loop if adding MAGIC_BITS to
           LONGWORD fails to change any of the hole bits of LONGWORD.

           1) Is this safe?  Will it catch all the zero bytes?
           Suppose there is a byte with all zeros.  Any carry bits
           propagating from its left will fall into the hole at its
           least significant bit and stop.  Since there will be no
           carry from its most significant bit, the LSB of the
           byte to the left will be unchanged, and the zero will be
           detected.

           2) Is this worthwhile?  Will it ignore everything except
           zero bytes?  Suppose every byte of LONGWORD has a bit set
           somewhere.  There will be a carry into bit 8.  If bit 8
           is set, this will carry into bit 16.  If bit 8 is clear,
           one of bits 9-15 must be set, so there will be a carry
           into bit 16.  Similarly, there will be a carry into bit
           24.  If one of bits 24-30 is set, there will be a carry
           into bit 31, so all of the hole bits will be changed.

           The one misfire occurs when bits 24-30 are clear and bit
           31 is set; in this case, the hole at bit 31 is not
           changed.  If we had access to the processor carry flag,
           we could close this loophole by putting the fourth hole
           at bit 32!

           So it ignores everything except 128's, when they're aligned
           properly.

           3) But wait!  Aren't we looking for C as well as zero?
           Good point.  So what we do is XOR LONGWORD with a longword,
           each of whose bytes is C.  This turns each byte that is C
           into a zero.  */

        longword = *longword_ptr++;

        /* Add MAGIC_BITS to LONGWORD.  */
        if ((((longword + magic_bits)

              /* Set those bits that were unchanged by the addition.  */
              ^ ~longword)

             /* Look at only the hole bits.  If any of the hole bits
                are unchanged, most likely one of the bytes was a
                zero.  */
             & ~magic_bits) != 0 ||
            /* That caught zeroes.  Now test for C.  */
            ((((longword ^ charmask) +
               magic_bits) ^ ~(longword ^ charmask))
             & ~magic_bits) != 0) {
            /* Which of the bytes was C or zero?
               If none of them were, it was a misfire; continue the search.  */

            const unsigned char *cp =
                (const unsigned char *) (longword_ptr - 1);

            if (*cp == c || *cp == '\0')
                return (char *) cp;
            if (*++cp == c || *cp == '\0')
                return (char *) cp;
            if (*++cp == c || *cp == '\0')
                return (char *) cp;
            if (*++cp == c || *cp == '\0')
                return (char *) cp;
            if (sizeof(longword) > 4) {
                if (*++cp == c || *cp == '\0')
                    return (char *) cp;
                if (*++cp == c || *cp == '\0')
                    return (char *) cp;
                if (*++cp == c || *cp == '\0')
                    return (char *) cp;
                if (*++cp == c || *cp == '\0')
                    return (char *) cp;
            }
        }
    }

    /* This should never happen.  */
    return NULL;
}
#endif

#ifdef HAVE___FXSTATAT
wrapper_proto(__fxstatat, int, (int, int, const char *, struct stat *, int));
#endif
#ifdef HAVE___FXSTATAT64
wrapper_proto(__fxstatat64, int, (int, int, const char *, struct stat64 *, int));
#endif
#ifdef HAVE___GETCWD_CHK
wrapper_proto(__getcwd_chk, char *, (char *, size_t, size_t));
#endif
#ifdef HAVE___GETWD_CHK
wrapper_proto(__getwd_chk, char *, (char *, size_t));
#endif
#ifdef HAVE___LXSTAT
wrapper_proto(__lxstat, int, (int, const char *, struct stat *));
#endif
#ifdef HAVE___LXSTAT64
wrapper_proto(__lxstat64, int, (int, const char *, struct stat64 *));
#endif
#ifdef HAVE___OPEN
wrapper_proto(__open, int, (const char *, int, ...));
#endif
#ifdef HAVE___OPEN_2
wrapper_proto(__open_2, int, (const char *, int));
#endif
#ifdef HAVE___OPEN64
wrapper_proto(__open64, int, (const char *, int, ...));
#endif
#ifdef HAVE___OPEN64_2
wrapper_proto(__open64_2, int, (const char *, int));
#endif
#ifdef HAVE___OPENAT_2
wrapper_proto(__openat_2, int, (int, const char *, int));
#endif
#ifdef HAVE___OPENAT64_2
wrapper_proto(__openat64_2, int, (int, const char *, int));
#endif
#ifdef HAVE___OPENDIR2
wrapper_proto(__opendir2, DIR *, (const char *, int));
#endif
#ifdef HAVE___REALPATH_CHK
wrapper_proto(__realpath_chk, char *, (const char *, char *, size_t));
#endif
#ifdef HAVE___READLINK_CHK
wrapper_proto(__readlink_chk, ssize_t, (const char *, char *, size_t, size_t));
#endif
#ifdef HAVE___READLINKAT_CHK
wrapper_proto(__readlinkat_chk, ssize_t, (int, const char *, char *, size_t, size_t));
#endif
#ifdef HAVE___STATFS
wrapper_proto(__statfs, int, (const char *, struct statfs *));
#endif
#ifdef HAVE___XMKNOD
wrapper_proto(__xmknod, int, (int, const char *, mode_t, dev_t *));
#endif
#ifdef HAVE___XSTAT
wrapper_proto(__xstat, int, (int, const char *, struct stat *));
#endif
#ifdef HAVE___XSTAT64
wrapper_proto(__xstat64, int, (int, const char *, struct stat64 *));
#endif
#ifdef HAVE__XFTW
wrapper_proto(_xftw, int, (int, const char *, int (*)(const char *, const struct stat *, int), int));
#endif
#ifdef HAVE__XFTW64
wrapper_proto(_xftw64, int, (int, const char *, int (*)(const char *, const struct stat64 *, int), int));
#endif
wrapper_proto(access, int, (const char *, int));
wrapper_proto(acct, int, (const char *));
#ifdef AF_UNIX
wrapper_proto(bind, int, (int, BIND_TYPE_ARG2(/**/), socklen_t));
#endif
#ifdef HAVE_BINDTEXTDOMAIN
wrapper_proto(bindtextdomain, char *, (const char *, const char *));
#endif
#ifdef HAVE_CANONICALIZE_FILE_NAME
wrapper_proto(canonicalize_file_name, char *, (const char *));
#endif
wrapper_proto(chdir, int, (const char *));
wrapper_proto(chmod, int, (const char *, mode_t));
wrapper_proto(chown, int, (const char *, uid_t, gid_t));
wrapper_proto(chroot, int, (const char *));
#ifdef AF_UNIX
wrapper_proto(connect, int, (int, CONNECT_TYPE_ARG2(/**/), socklen_t));
#endif
wrapper_proto(creat, int, (const char *, mode_t));
#ifdef HAVE_CREAT64
wrapper_proto(creat64, int, (const char *, mode_t));
#endif
#ifdef HAVE_DLMOPEN
wrapper_proto(dlmopen, void *, (Lmid_t, const char *, int));
#endif
wrapper_proto(dlopen, void *, (const char *, int));
#ifdef HAVE_EACCESS
wrapper_proto(eaccess, int, (const char *, int));
#endif
#ifdef HAVE_EUIDACCESS
wrapper_proto(euidaccess, int, (const char *, int));
#endif
wrapper_proto(execl, int, (const char *, const char *, ...));
wrapper_proto(execle, int, (const char *, const char *, ...));
wrapper_proto(execlp, int, (const char *, const char *, ...));
wrapper_proto(execv, int, (const char *, char * const []));
wrapper_proto(execve, int, (const char *, char * const [], char * const []));
wrapper_proto(execvp, int, (const char *, char * const []));
#ifdef HAVE_FCHMODAT
wrapper_proto(fchmodat, int, (int, const char *, mode_t, int));
#endif
#ifdef HAVE_FCHOWNAT
wrapper_proto(fchownat, int, (int, const char *, uid_t, gid_t, int));
#endif
wrapper_proto(fopen, FILE *, (const char *, const char *));
#ifdef HAVE_FOPEN64
wrapper_proto(fopen64, FILE *, (const char *, const char *));
#endif
wrapper_proto(freopen, FILE *, (const char *, const char *, FILE *));
#ifdef HAVE_FREOPEN64
wrapper_proto(freopen64, FILE *, (const char *, const char *, FILE *));
#endif
#ifdef HAVE_FTS_CHILDREN
#if !defined(HAVE___OPENDIR2)
wrapper_proto(fts_children, FTSENT *, (FTS *, int));
#endif
#endif
#ifdef HAVE_FTS_OPEN
#if !defined(HAVE___OPENDIR2)
wrapper_proto(fts_open, FTS *, (char * const *, int, int (*)(const FTSENT **, const FTSENT **)));
#endif
#endif
#ifdef HAVE_FTS_READ
#if !defined(HAVE___OPENDIR2)
wrapper_proto(fts_read, FTSENT *, (FTS *));
#endif
#endif
#ifdef HAVE_FTW
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
wrapper_proto(ftw, int, (const char *, int (*)(const char *, const struct stat *, int), int));
#endif
#endif
#ifdef HAVE_FTW64
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
wrapper_proto(ftw64, int, (const char *, int (*)(const char *, const struct stat64 *, int), int));
#endif
#endif
#ifdef HAVE_FUTIMESAT
wrapper_proto(futimesat, int, (int, const char *, const struct timeval [2]));
#endif
#ifdef HAVE_GET_CURRENT_DIR_NAME
wrapper_proto(get_current_dir_name, char *, (void));
#endif
wrapper_proto(getcwd, char *, (char *, size_t));
#ifdef AF_UNIX
wrapper_proto(getpeername, int, (int, GETPEERNAME_TYPE_ARG2(/**/), socklen_t *));
#endif
#ifdef AF_UNIX
wrapper_proto(getsockname, int, (int, GETSOCKNAME_TYPE_ARG2(/**/), socklen_t *));
#endif
#ifdef HAVE_GETWD
wrapper_proto(getwd, char *, (char *));
#endif
#ifdef HAVE_GETXATTR
wrapper_proto(getxattr, ssize_t, (const char *, const char *, void *, size_t));
#endif
wrapper_proto(glob, int, (const char *, int, int (*) (const char *, int), glob_t *));
#ifdef HAVE_GLOB64
wrapper_proto(glob64, int, (const char *, int, int (*) (const char *, int), glob64_t *));
#endif
#ifdef HAVE_GLOB_PATTERN_P
wrapper_proto(glob_pattern_p, int, (const char *, int));
#endif
#ifdef HAVE_INOTIFY_ADD_WATCH
wrapper_proto(inotify_add_watch, int, (int, const char *, uint32_t));
#endif
#ifdef HAVE_LCHMOD
wrapper_proto(lchmod, int, (const char *, mode_t));
#endif
wrapper_proto(lchown, int, (const char *, uid_t, gid_t));
#ifdef HAVE_LCKPWDF
wrapper_proto(lckpwdf, int, (void));
#endif
#ifdef HAVE_LGETXATTR
wrapper_proto(lgetxattr, ssize_t, (const char *, const char *, void *, size_t));
#endif
wrapper_proto(link, int, (const char *, const char *));
#ifdef HAVE_LINKAT
wrapper_proto(linkat, int, (int, const char *, int, const char *, int));
#endif
#ifdef HAVE_LISTXATTR
wrapper_proto(listxattr, ssize_t, (const char *, char *, size_t));
#endif
#ifdef HAVE_LLISTXATTR
wrapper_proto(llistxattr, ssize_t, (const char *, char *, size_t));
#endif
#ifdef HAVE_LREMOVEXATTR
wrapper_proto(lremovexattr, int, (const char *, const char *));
#endif
#ifdef HAVE_LSETXATTR
wrapper_proto(lsetxattr, int, (const char *, const char *, const void *, size_t, int));
#endif
wrapper_proto(lstat, int, (const char *, struct stat *));
#ifdef HAVE_LSTAT64
wrapper_proto(lstat64, int, (const char *, struct stat64 *));
#endif
#ifdef HAVE_LUTIMES
wrapper_proto(lutimes, int, (const char *, const struct timeval [2]));
#endif
wrapper_proto(mkdir, int, (const char *, mode_t));
#ifdef HAVE_MKDIRAT
wrapper_proto(mkdirat, int, (int, const char *, mode_t));
#endif
#ifdef HAVE_MKDTEMP
wrapper_proto(mkdtemp, char *, (char *));
#endif
wrapper_proto(mknod, int, (const char *, mode_t, dev_t));
#ifdef HAVE_MKNODAT
wrapper_proto(mknodat, int, (int, const char *, mode_t, dev_t));
#endif
wrapper_proto(mkfifo, int, (const char *, mode_t));
#ifdef HAVE_MKFIFOAT
wrapper_proto(mkfifoat, int, (int, const char *, mode_t));
#endif
wrapper_proto(mkstemp, int, (char *));
#ifdef HAVE_MKSTEMP64
wrapper_proto(mkstemp64, int, (char *));
#endif
wrapper_proto(mktemp, char *, (char *));
#ifdef HAVE_NFTW
wrapper_proto(nftw, int, (const char *, int (*)(const char *, const struct stat *, int, struct FTW *), int, int));
#endif
#ifdef HAVE_NFTW64
wrapper_proto(nftw64, int, (const char *, int (*)(const char *, const struct stat64 *, int, struct FTW *), int, int));
#endif
wrapper_proto(open, int, (const char *, int, ...));
#ifdef HAVE_OPEN64
wrapper_proto(open64, int, (const char *, int, ...));
#endif
#ifdef HAVE_OPENAT
wrapper_proto(openat, int, (int, const char *, int, ...));
#endif
#ifdef HAVE_OPENAT64
wrapper_proto(openat64, int, (int, const char *, int, ...));
#endif
#if !defined(HAVE___OPENDIR2)
wrapper_proto(opendir, DIR *, (const char *));
#endif
wrapper_proto(pathconf, long, (const char *, int));
#ifdef __GNUC__
wrapper_proto(popen, FILE *, (const char *, const char *));
#endif
wrapper_proto(readlink, READLINK_TYPE_RETURN, (const char *, char *, READLINK_TYPE_ARG3(/**/)));
#ifdef HAVE_READLINKAT
wrapper_proto(readlinkat, ssize_t, (int, const char *, char *, size_t));
#endif
wrapper_proto(realpath, char *, (const char *, char *));
wrapper_proto(remove, int, (const char *));
#ifdef HAVE_REMOVEXATTR
wrapper_proto(removexattr, int, (const char *, const char *));
#endif
wrapper_proto(rename, int, (const char *, const char *));
#ifdef HAVE_RENAMEAT
wrapper_proto(renameat, int, (int, const char *, int, const char *));
#endif
#ifdef HAVE_REVOKE
wrapper_proto(revoke, int, (const char *));
#endif
wrapper_proto(rmdir, int, (const char *));
#ifdef HAVE_SCANDIR
wrapper_proto(scandir, int, (const char *, struct dirent ***, SCANDIR_TYPE_ARG3(/**/), SCANDIR_TYPE_ARG4(/**/)));
#endif
#ifdef HAVE_SCANDIR64
wrapper_proto(scandir64, int, (const char *, struct dirent64 ***, SCANDIR64_TYPE_ARG3(/**/), SCANDIR64_TYPE_ARG4(/**/)));
#endif
#ifdef HAVE_SETXATTR
wrapper_proto(setxattr, int, (const char *, const char *, const void *, size_t, int));
#endif
wrapper_proto(stat, int, (const char *, struct stat *));
#ifdef HAVE_STAT64
wrapper_proto(stat64, int, (const char *, struct stat64 *));
#endif
#ifdef HAVE_STATFS
wrapper_proto(statfs, int, (const char *, struct statfs *));
#endif
#ifdef HAVE_STATFS64
wrapper_proto(statfs64, int, (const char *, struct statfs64 *));
#endif
#ifdef HAVE_STATVFS
#if !defined(__FreeBSD__) || defined(__GLIBC__)
wrapper_proto(statvfs, int, (const char *, struct statvfs *));
#endif
#endif
#ifdef HAVE_STATVFS64
wrapper_proto(statvfs64, int, (const char *, struct statvfs64 *));
#endif
wrapper_proto(symlink, int, (const char *, const char *));
#ifdef HAVE_SYMLINKAT
wrapper_proto(symlinkat, int, (const char *, int, const char *));
#endif
#ifdef __GNUC__
wrapper_proto(system, int, (const char *));
#endif
wrapper_proto(tempnam, char *, (const char *, const char *));
wrapper_proto(tmpnam, char *, (char *));
wrapper_proto(truncate, int, (const char *, off_t));
#ifdef HAVE_TRUNCATE64
wrapper_proto(truncate64, int, (const char *, off64_t));
#endif
wrapper_proto(unlink, int, (const char *));
#ifdef HAVE_UNLINKAT
wrapper_proto(unlinkat, int, (int, const char *, int));
#endif
#ifdef HAVE_ULCKPWDF
wrapper_proto(ulckpwdf, int, (void));
#endif
wrapper_proto(utime, int, (const char *, const struct utimbuf *));
#ifdef HAVE_UTIMENSAT
wrapper_proto(utimensat, int, (int, const char *, const struct timespec [2], int));
#endif
wrapper_proto(utimes, int, (const char *, const struct timeval [2]));


/* Bootstrap the library */
void fakechroot_init (void) __attribute__((constructor));
void fakechroot_init (void)
{
    int i,j;
    struct passwd* passwd = NULL;
    char *pointer;

    debug("fakechroot_init()");
    if (!first) {
        first = 1;

        /* We get a list of directories or files */
        pointer = getenv("FAKECHROOT_EXCLUDE_PATH");
        if (pointer) {
            for (i=0; list_max<32 ;) {
                for (j=i; pointer[j]!=':' && pointer[j]!='\0'; j++);
                exclude_list[list_max] = malloc(j-i+2);
                memset(exclude_list[list_max], '\0', j-i+2);
                strncpy(exclude_list[list_max], &(pointer[i]), j-i);
                exclude_length[list_max] = strlen(exclude_list[list_max]);
                list_max++;
                if (pointer[j] != ':') break;
                i=j+1;
            }
        }

        /* We get the home of the user */
        passwd = getpwuid(getuid());
        if (passwd && passwd->pw_dir) {
            home_path = malloc(strlen(passwd->pw_dir)+2);
            strcpy(home_path, passwd->pw_dir);
            strcat(home_path, "/");
        }
    }
}


/* Lazily load function */
static inline fakechroot_wrapperfn_t fakechroot_loadfunc (struct fakechroot_wrapper *w)
{
    char *msg;
    if (!(w->nextfunc = dlsym(RTLD_NEXT, w->name))) {;
        msg = dlerror();
        fprintf(stderr, "%s: %s: %s\n", PACKAGE, w->name, msg != NULL ? msg : "unresolved symbol");
        exit(EXIT_FAILURE);
    }
    return w->nextfunc;
}


/* Check if path is on exclude list */
static int fakechroot_localdir (const char *p_path)
{
    char *v_path = (char*)p_path;
    char *fakechroot_path, *fakechroot_ptr;
    char cwd_path[FAKECHROOT_PATH_MAX];
    int i, len;

    if (!p_path) return 0;

    if (!first) fakechroot_init();

    /* We need to expand ~ paths */
    if (home_path!=NULL && p_path[0]=='~') {
        strcpy(cwd_path, home_path);
        strcat(cwd_path, &(p_path[1]));
        v_path = cwd_path;
    }

    /* We need to expand relative paths */
    if (p_path[0] != '/') {
        nextcall(getcwd)(cwd_path, FAKECHROOT_PATH_MAX);
        v_path = cwd_path;
        narrow_chroot_path(v_path, fakechroot_path, fakechroot_ptr);
    }

    /* We try to find if we need direct access to a file */
    len = strlen(v_path);
    for (i=0; i<list_max; i++) {
        if (exclude_length[i]>len ||
            v_path[exclude_length[i]-1]!=(exclude_list[i])[exclude_length[i]-1] ||
            strncmp(exclude_list[i],v_path,exclude_length[i])!=0) continue;
        if (exclude_length[i]==len || v_path[exclude_length[i]]=='/') return 1;
    }

    return 0;
}


#ifdef HAVE___FXSTATAT
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
/* #include <sys/stat.h> */
int __fxstatat (int ver, int dirfd, const char *pathname, struct stat *buf, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__fxstatat(%d, %d, \"%s\", &buf, %d)", ver, dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(__fxstatat)(ver, dirfd, pathname, buf, flags);
}
#endif


#ifdef HAVE___FXSTATAT64
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
/* #include <sys/stat.h> */
int __fxstatat64 (int ver, int dirfd, const char *pathname, struct stat64 *buf, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__fxstatat64(%d, %d, \"%s\", &buf, %d)", ver, dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(__fxstatat64)(ver, dirfd, pathname, buf, flags);
}
#endif


#ifdef HAVE___GETCWD_CHK
/* #include <unistd.h> */
char * __getcwd_chk (char *buf, size_t size, size_t buflen)
{
    char *cwd;
    char *fakechroot_path, *fakechroot_ptr;

    debug("__getcwd_chk(&buf, %zd, %zd)", size, buflen);
    if ((cwd = nextcall(__getcwd_chk)(buf, size, buflen)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    return cwd;
}
#endif


#ifdef HAVE___GETWD_CHK
/* #include <unistd.h> */
char * __getwd_chk (char *buf, size_t buflen)
{
    char *cwd;
    char *fakechroot_path, *fakechroot_ptr;

    debug("__getwd_chk(&buf, %zd)", buflen);
    if ((cwd = nextcall(__getwd_chk)(buf, buflen)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    return cwd;
}
#endif


#ifdef HAVE___LXSTAT
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX], tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN status;
    const char* orig;

    debug("__lxstat(%d, \"%s\", &buf)", ver, filename);
    orig = filename;
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    retval = nextcall(__lxstat)(ver, filename, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((buf->st_mode & S_IFMT) == S_IFLNK)
        if ((status = readlink(orig, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = status;

    return retval;
}
#endif


#ifdef HAVE___LXSTAT64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat64 (int ver, const char *filename, struct stat64 *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX], tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN status;
    const char* orig;

    debug("__lxstat64(%d, \"%s\", &buf)", ver, filename);
    orig = filename;
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    retval = nextcall(__lxstat64)(ver, filename, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((buf->st_mode & S_IFMT) == S_IFLNK)
        if ((status = readlink(orig, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = status;

    return retval;
}
#endif


#ifdef HAVE___OPEN
/* Internal libc function */
int __open (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__open(\"%s\", %d, ...)", pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return nextcall(__open)(pathname, flags, mode);
}
#endif


#ifdef HAVE___OPEN_2
/* Internal libc function */
int __open_2 (const char *pathname, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__open_2(\"%s\", %d)", pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    return nextcall(__open_2)(pathname, flags);
}
#endif


#ifdef HAVE___OPEN64
/* Internal libc function */
int __open64 (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__open64(\"%s\", %d, ...)", pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return nextcall(__open64)(pathname, flags, mode);
}
#endif


#ifdef HAVE___OPEN64_2
/* Internal libc function */
int __open64_2 (const char *pathname, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__open64_2(\"%s\", %d)", pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    return nextcall(__open64_2)(pathname, flags);
}
#endif


#ifdef HAVE___OPENAT_2
/* Internal libc function */
int __openat_2 (int dirfd, const char *pathname, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__openat_2(%d, \"%s\", %d)", dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    return nextcall(__openat_2)(dirfd, pathname, flags);
}
#endif


#ifdef HAVE___OPENAT64_2
/* Internal libc function */
int __openat64_2 (int dirfd, const char *pathname, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__openat64_2(%d, \"%s\", %d)", dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    return nextcall(__openat64_2)(dirfd, pathname, flags);
}
#endif


#ifdef HAVE___OPENDIR2
/* Internal libc function */
/* #include <dirent.h> */
DIR *__opendir2 (const char *name, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__opendir2(\"%s\", %d)", name, flags);
    expand_chroot_path(name, fakechroot_path, fakechroot_buf);
    return nextcall(__opendir2)(name, flags);
}
#endif


#ifdef HAVE___READLINK_CHK
/* #include <unistd.h> */
ssize_t __readlink_chk (const char *path, char *buf, size_t bufsiz, size_t buflen)
{
    int status;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__readlink_chk(\"%s\", &buf, %zd, %zd)", path, bufsiz, buflen);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);

    if ((status = nextcall(__readlink_chk)(path, tmp, FAKECHROOT_PATH_MAX-1, buflen)) == -1) {
        return -1;
    }
    tmp[status] = '\0';

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + strlen(fakechroot_path);
            status -= strlen(fakechroot_path);
        }
        if (strlen(tmpptr) > bufsiz) {
            status = bufsiz;
        }
        strncpy(buf, tmpptr, status);
    } else {
        strncpy(buf, tmp, status);
    }
    return status;
}
#endif


#ifdef HAVE___READLINKAT_CHK
/* #include <unistd.h> */
ssize_t __readlinkat_chk (int dirfd, const char *path, char *buf, size_t bufsiz, size_t buflen)
{
    int status;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("__readlinkat_chk(%d, \"%s\", &buf, %zd, %zd)", dirfd, path, bufsiz, buflen);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);

    if ((status = nextcall(__readlinkat_chk)(dirfd, path, tmp, FAKECHROOT_PATH_MAX-1, buflen)) == -1) {
        return -1;
    }
    tmp[status] = '\0';

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + strlen(fakechroot_path);
            status -= strlen(fakechroot_path);
        }
        if (strlen(tmpptr) > bufsiz) {
            status = bufsiz;
        }
        strncpy(buf, tmpptr, status);
    } else {
        strncpy(buf, tmp, status);
    }
    return status;
}
#endif


#ifdef HAVE___REALPATH_CHK
/* #include <stdlib.h> */
/* #include <sys/cdefs.h> */

extern void __chk_fail (void) __attribute__ ((__noreturn__));

char * __realpath_chk (const char *path, char *resolved, size_t resolvedlen)
{
    debug("__realpath_chk(\"%s\", &buf, %zd)", path, resolvedlen);
    if (resolvedlen < FAKECHROOT_PATH_MAX)
        __chk_fail ();

    return realpath (path, resolved);
}
#endif


#ifdef HAVE___STATFS
/* #include <sys/vfs.h> */
int __statfs (const char *path, struct statfs *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__statfs(\"%s\", &buf)", path);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(__statfs)(path, buf);
}
#endif


#ifdef HAVE___XMKNOD
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xmknod (int ver, const char *path, mode_t mode, dev_t *dev)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__xmknod(%d, \"%s\", 0%od, &dev)", ver, path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(__xmknod)(ver, path, mode, dev);
}
#endif


#ifdef HAVE___XSTAT
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__xstat(%d, \"%s\", %d, &buf)", ver, filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(__xstat)(ver, filename, buf);
}
#endif


#ifdef HAVE___XSTAT64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xstat64 (int ver, const char *filename, struct stat64 *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("__xstat64(%d, \"%s\", %d, &buf)", ver, filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(__xstat64)(ver, filename, buf);
}
#endif


#ifdef HAVE__XFTW
/* include <ftw.h> */
static int (*_xftw_fn_saved)(const char *file, const struct stat *sb, int flag);

static int _xftw_fn_wrapper (const char *file, const struct stat *sb, int flag)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return _xftw_fn_saved(file, sb, flag);
}

int _xftw (int mode, const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("_xftw(%d, \"%s\", &fn, %d)", mode, dir, nopenfd);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    _xftw_fn_saved = fn;
    return nextcall(_xftw)(mode, dir, _xftw_fn_wrapper, nopenfd);
}
#endif


#ifdef HAVE__XFTW64
/* include <ftw.h> */
static int (*_xftw64_fn_saved)(const char *file, const struct stat *sb, int flag);

static int _xftw64_fn_wrapper (const char *file, const struct stat *sb, int flag)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return _xftw64_fn_saved(file, sb, flag);
}

int _xftw64 (int mode, const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag), int nopenfd)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("_xftw64(%d, \"%s\", &fn, %d)", mode, dir, nopenfd);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    _xftw64_fn_saved = fn;
    return nextcall(_xftw64)(mode, dir, _xftw64_fn_wrapper, nopenfd);
}
#endif


/* #include <unistd.h> */
int access (const char *pathname, int mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("access(\"%s\", %d)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(access)(pathname, mode);
}


/* #include <unistd.h> */
int acct (const char *filename)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("acct(\"%s\")", filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(acct)(filename);
}


#ifdef AF_UNIX
/* #include <sys/types.h> */
/* #include <sys/socket.h> */
/* #include <sys/un.h> */

#ifdef HAVE_BIND_TYPE_ARG2___CONST_SOCKADDR_ARG__
#define BIND_SOCKADDR_UN(addr) SOCKADDR_UN_UNION(addr)
#else
#define BIND_SOCKADDR_UN(addr) SOCKADDR_UN(addr)
#endif

int bind (int sockfd, BIND_TYPE_ARG2(addr), socklen_t addrlen)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    char *path;
    socklen_t newaddrlen;
    struct sockaddr_un newaddr_un;
    struct sockaddr_un *addr_un = BIND_SOCKADDR_UN(addr);
    char *af_unix_path;
    const int af_unix_path_max = sizeof(addr_un->sun_path);

    debug("bind(%d, &addr, &addrlen)", sockfd);
    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path && *(addr_un->sun_path)) {
        path = addr_un->sun_path;
        if ((af_unix_path = getenv("FAKECHROOT_AF_UNIX_PATH")) != NULL) {
            fakechroot_buf[af_unix_path_max] = 0;
            strncpy(fakechroot_buf, af_unix_path, af_unix_path_max - 1);
            strcat(fakechroot_buf, path);
            path = fakechroot_buf;
        }
        else {
            expand_chroot_path(path, fakechroot_path, fakechroot_buf);
        }

        if (strlen(path) >= sizeof(addr_un->sun_path)) {
            errno = ENAMETOOLONG;
            return -1;
        }
        memset(&newaddr_un, 0, sizeof(struct sockaddr_un));
        newaddr_un.sun_family = addr_un->sun_family;
        strncpy(newaddr_un.sun_path, path, sizeof(newaddr_un.sun_path) - 1);
        newaddrlen = SUN_LEN(&newaddr_un);
        return nextcall(bind)(sockfd, (struct sockaddr *)&newaddr_un, newaddrlen);
    }
    return nextcall(bind)(sockfd, addr, addrlen);
}
#endif


#ifdef HAVE_BINDTEXTDOMAIN
/* #include <libintl.h> */
char * bindtextdomain (const char *domainname, const char *dirname)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("bindtextdomain(\"%s\", \"%s\")", domainname, dirname);
    expand_chroot_path(dirname, fakechroot_path, fakechroot_buf);
    return nextcall(bindtextdomain)(domainname, dirname);
}
#endif


#ifdef HAVE_CANONICALIZE_FILE_NAME
/* #include <stdlib.h> */
char *canonicalize_file_name (const char *name)
{
    char *resolved = malloc(FAKECHROOT_PATH_MAX * 2);
    debug("canonicalize_file_name(\"%s\")", name);
#ifdef HAVE___REALPATH_CHK
    return __realpath_chk(name, resolved, FAKECHROOT_PATH_MAX * 2);
#else
    return realpath(name, resolved);
#endif
}
#endif


/* #include <unistd.h> */
int chdir (const char *path)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("chdir(\"%s\")", path);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(chdir)(path);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int chmod (const char *path, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("chmod(\"%s\", 0%od)", path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(chmod)(path, mode);
}


/* #include <sys/types.h> */
/* #include <unistd.h> */
int chown (const char *path, uid_t owner, gid_t group)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("chown(\"%s\", %d, %d)", path, owner, group);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(chown)(path, owner, group);
}


/* #include <unistd.h> */
int chroot (const char *path)
{
    char *ptr, *ld_library_path, *tmp, *fakechroot_path;
    int status, len;
    char dir[FAKECHROOT_PATH_MAX], cwd[FAKECHROOT_PATH_MAX];
#if !defined(HAVE_SETENV)
    char *envbuf;
#endif
    struct stat sb;

    debug("chroot(\"%s\")", path);
    if (path == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (!*path) {
        errno = ENOENT;
        return -1;
    }
    if (*path != '/') {
        if (nextcall(getcwd)(cwd, FAKECHROOT_PATH_MAX) == NULL) {
            errno = ENAMETOOLONG;
            return -1;
        }
        if (cwd == NULL) {
            errno = EFAULT;
            return -1;
        }
        if (strcmp(cwd, "/") == 0) {
            snprintf(dir, FAKECHROOT_PATH_MAX, "/%s", path);
        }
        else {
            snprintf(dir, FAKECHROOT_PATH_MAX, "%s/%s", cwd, path);
        }
    }
    else {
        fakechroot_path = getenv("FAKECHROOT_BASE");

        if (fakechroot_path != NULL) {
            snprintf(dir, FAKECHROOT_PATH_MAX, "%s%s", fakechroot_path, path);
        }
        else {
            snprintf(dir, FAKECHROOT_PATH_MAX, "%s", path);
        }
    }

#if defined(HAVE___XSTAT) && defined(_STAT_VER)
    if ((status = nextcall(__xstat)(_STAT_VER, dir, &sb)) != 0) {
        return status;
    }
#else
    if ((status = nextcall(stat)(dir, &sb)) != 0) {
        return status;
    }
#endif

    if ((sb.st_mode & S_IFMT) != S_IFDIR) {
        return ENOTDIR;
    }

    ptr = rindex(dir, 0);
    if (ptr > dir) {
        ptr--;
        while (*ptr == '/') {
            *ptr-- = 0;
        }
    }

    ptr = tmp = dir;
    for (ptr=tmp=dir; *ptr; ptr++) {
        if (*ptr == '/' &&
                *(ptr+1) && *(ptr+1) == '.' &&
                (!*(ptr+2) || (*(ptr+2) == '/'))
        ) {
            ptr++;
        } else {
            *(tmp++) = *ptr;
        }
    }
    *tmp = 0;

#if defined(HAVE_SETENV)
    setenv("FAKECHROOT_BASE", dir, 1);
#else
    envbuf = malloc(FAKECHROOT_PATH_MAX+16);
    snprintf(envbuf, FAKECHROOT_PATH_MAX+16, "FAKECHROOT_BASE=%s", dir);
    putenv(envbuf);
#endif
    fakechroot_path = getenv("FAKECHROOT_BASE");

    ld_library_path = getenv("LD_LIBRARY_PATH");
    if (ld_library_path == NULL) {
        ld_library_path = "";
    }

    if ((len = strlen(ld_library_path)+strlen(dir)*2+sizeof(":/usr/lib:/lib")) > FAKECHROOT_PATH_MAX) {
        return ENAMETOOLONG;
    }

    if ((tmp = malloc(len)) == NULL) {
        return ENOMEM;
    }

    snprintf(tmp, len, "%s:%s/usr/lib:%s/lib", ld_library_path, dir, dir);
#if defined(HAVE_SETENV)
    setenv("LD_LIBRARY_PATH", tmp, 1);
#else
    envbuf = malloc(FAKECHROOT_PATH_MAX+16);
    snprintf(envbuf, FAKECHROOT_PATH_MAX+16, "LD_LIBRARY_PATH=%s", tmp);
    putenv(envbuf);
#endif
    free(tmp);
    return 0;
}


#ifdef AF_UNIX
/* #include <sys/types.h> */
/* #include <sys/socket.h> */
/* #include <sys/un.h> */

#ifdef HAVE_CONNECT_TYPE_ARG2___CONST_SOCKADDR_ARG__
#define CONNECT_SOCKADDR_UN(addr) SOCKADDR_UN_UNION(addr)
#else
#define CONNECT_SOCKADDR_UN(addr) SOCKADDR_UN(addr)
#endif

int connect (int sockfd, CONNECT_TYPE_ARG2(addr), socklen_t addrlen)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    char *path;
    socklen_t newaddrlen;
    struct sockaddr_un newaddr_un;
    struct sockaddr_un *addr_un = CONNECT_SOCKADDR_UN(addr);
    char *af_unix_path;
    const int af_unix_path_max = sizeof(addr_un->sun_path);

    debug("connect(%d, &addr, %d)", sockfd, addrlen);
    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path && *(addr_un->sun_path)) {
        path = addr_un->sun_path;
        if ((af_unix_path = getenv("FAKECHROOT_AF_UNIX_PATH")) != NULL) {
            fakechroot_buf[af_unix_path_max] = 0;
            strncpy(fakechroot_buf, af_unix_path, af_unix_path_max - 1);
            strcat(fakechroot_buf, path);
            path = fakechroot_buf;
        }
        else {
            expand_chroot_path(path, fakechroot_path, fakechroot_buf);
        }

        if (strlen(path) >= sizeof(addr_un->sun_path)) {
            errno = ENAMETOOLONG;
            return -1;
        }
        memset(&newaddr_un, 0, sizeof(struct sockaddr_un));
        newaddr_un.sun_family = addr_un->sun_family;
        strncpy(newaddr_un.sun_path, path, sizeof(newaddr_un.sun_path) - 1);
        newaddrlen = SUN_LEN(&newaddr_un);
        return nextcall(connect)(sockfd, (struct sockaddr *)&newaddr_un, newaddrlen);
    }
    return nextcall(connect)(sockfd, addr, addrlen);
}
#endif


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int creat (const char *pathname, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("creat(\"%s\", 0%od)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(creat)(pathname, mode);
}


#ifdef HAVE_CREAT64
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int creat64 (const char *pathname, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("creat64(\"%s\", 0%od)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(creat64)(pathname, mode);
}
#endif


#ifdef HAVE_DLMOPEN
/* #include <dlfcn.h> */
void *dlmopen (Lmid_t nsid, const char *filename, int flag)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("dlmopen(&nsid, \"%s\", %d)", filename, flag);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(dlmopen)(nsid, filename, flag);
}
#endif


/* #include <dlfcn.h> */
void *dlopen (const char *filename, int flag)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("dlopen(\"%s\", %d)", filename, flag);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(dlopen)(filename, flag);
}


#ifdef HAVE_EACCESS
/* #define _GNU_SOURCE */
/* #include <unistd.h> */
int eaccess (const char *pathname, int mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("eaccess(\"%s\", %d)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(eaccess)(pathname, mode);
}
#endif


#ifdef HAVE_EUIDACCESS
/* #define _GNU_SOURCE */
/* #include <unistd.h> */
int euidaccess (const char *pathname, int mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("euidaccess(\"%s\", %d)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(euidaccess)(pathname, mode);
}
#endif


/* execl function reimplementation taken from GNU C Library.
   Copyright (C) 1991,92,94,97,98,99,2002,2005 Free Software Foundation, Inc.
 */
/* #include <unistd.h> */
int execl (const char *path, const char *arg, ...)
{
    size_t argv_max = 1024;
    const char **argv = alloca(argv_max * sizeof(const char *));
    unsigned int i;
    va_list args;

    debug("execl(\"%s\", \"%s\", ...)", path, arg);
    argv[0] = arg;

    va_start (args, arg);
    i = 0;
    while (argv[i++] != NULL) {
        if (i == argv_max) {
            const char **nptr = alloca((argv_max *= 2) * sizeof(const char *));

            if ((char *) argv + i == (char *) nptr)
                /* Stack grows up.  */
                argv_max += i;
            else
                /* We have a hole in the stack.  */
                argv = (const char **) memcpy(nptr, argv, i
                        * sizeof(const char *));
        }

        argv[i] = va_arg (args, const char *);
    }
    va_end (args);

    return execve(path, (char * const *) argv, environ);
}


/* execle function reimplementation taken from GNU C Library.
   Copyright (C) 1991,97,98,99,2002,2005 Free Software Foundation, Inc.
 */
/* #include <unistd.h> */
int execle (const char *path, const char *arg, ...)
{
    size_t argv_max = 1024;
    const char **argv = alloca(argv_max * sizeof(const char *));
    const char * const *envp;
    unsigned int i;
    va_list args;

    debug("execle(\"%s\", \"%s\", ...)", path, arg);
    argv[0] = arg;

    va_start (args, arg);
    i = 0;
    while (argv[i++] != NULL) {
        if (i == argv_max) {
            const char **nptr = alloca((argv_max *= 2) * sizeof(const char *));

            if ((char *) argv + i == (char *) nptr)
                /* Stack grows up.  */
                argv_max += i;
            else
                /* We have a hole in the stack.  */
                argv = (const char **) memcpy(nptr, argv, i
                        * sizeof(const char *));
        }

        argv[i] = va_arg (args, const char *);
    }

    envp = va_arg (args, const char * const *);
    va_end (args);

    return execve(path, (char * const *) argv, (char * const *) envp);
}


/* execlp function reimplementation taken from GNU C Library.
   Copyright (C) 1991,93,96,97,98,99,2002,2005 Free Software Foundation, Inc.
 */
/* #include <unistd.h> */
int execlp (const char *file, const char *arg, ...)
{
    size_t argv_max = 1024;
    const char **argv = alloca(argv_max * sizeof(const char *));
    unsigned int i;
    va_list args;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("execlp(\"%s\", \"%s\", ...)", file, arg);
    argv[0] = arg;

    va_start (args, arg);
    i = 0;
    while (argv[i++] != NULL) {
        if (i == argv_max) {
            const char **nptr = alloca((argv_max *= 2) * sizeof(const char *));

            if ((char *) argv + i == (char *) nptr)
                /* Stack grows up.  */
                argv_max += i;
            else
                /* We have a hole in the stack.  */
                argv = (const char **) memcpy(nptr, argv, i
                        * sizeof(const char *));
        }

        argv[i] = va_arg (args, const char *);
    }
    va_end (args);

    expand_chroot_path(file, fakechroot_path, fakechroot_buf);
    return execvp(file, (char * const *) argv);
}


/* #include <unistd.h> */
int execv (const char *path, char * const argv [])
{
    debug("execv(\"%s\", {\"%s\", ...})", path, argv[0]);
    return execve (path, argv, environ);
}

/* Parse the FAKECHROOT_CMD_SUBST environment variable (the first
 * parameter) and if there is a match with filename, return the
 * substitution in cmd_subst.  Returns non-zero if there was a match.
 *
 * FAKECHROOT_CMD_SUBST=cmd=subst:cmd=subst:...
 */
static int try_cmd_subst (char *env, const char *filename, char *cmd_subst)
{
    int len = strlen (filename), len2;
    char *p;

    if (env == NULL) return 0;

    do {
        p = strchrnul (env, ':');

        if (strncmp (env, filename, len) == 0 && env[len] == '=') {
            len2 = p - &env[len+1];
            if (len2 >= FAKECHROOT_PATH_MAX)
                len2 = FAKECHROOT_PATH_MAX - 1;
            strncpy (cmd_subst, &env[len+1], len2);
            cmd_subst[len2] = '\0';
            return 1;
        }

        env = p;
    } while (*env++ != '\0');

    return 0;
}

/* #include <unistd.h> */
int execve (const char *filename, char * const argv [], char * const envp[])
{
    int file;
    char hashbang[FAKECHROOT_PATH_MAX];
    size_t argv_max = 1024;
    const char **newargv = alloca (argv_max * sizeof (const char *));
    char **newenvp, **ep;
    char *env;
    char tmp[FAKECHROOT_PATH_MAX], newfilename[FAKECHROOT_PATH_MAX], argv0[FAKECHROOT_PATH_MAX];
    char *ptr;
    unsigned int i, j, n, len, r, newenvppos;
    size_t sizeenvp;
    char c;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    char *envkey[] = { "FAKECHROOT", "FAKECHROOT_BASE",
                       "FAKECHROOT_VERSION", "FAKECHROOT_EXCLUDE_PATH",
                       "FAKECHROOT_CMD_SUBST",
                       "LD_LIBRARY_PATH", "LD_PRELOAD" };
    const int nr_envkey = sizeof envkey / sizeof envkey[0];

    debug("execve(\"%s\", {\"%s\", ...}, {\"%s\", ...})", filename, argv[0], envp[0]);

    /* Scan envp and check its size */
    sizeenvp = 0;
    for (ep = (char **)envp; *ep != NULL; ++ep) {
        sizeenvp++;
    }

    /* Copy envp to newenvp */
    newenvp = malloc( (sizeenvp + 1) * sizeof (char *) );
    if (newenvp == NULL) {
        errno = ENOMEM;
        return -1;
    }
    for (ep = (char **) envp, newenvppos = 0; *ep != NULL; ++ep) {
        for (j = 0; j < nr_envkey; j++) {
            len = strlen (envkey[j]);
            if (strncmp (*ep, envkey[j], len) == 0 && (*ep)[len] == '=')
                goto skip;
        }
        newenvp[newenvppos] = *ep;
        newenvppos++;
    skip: ;
    }
    newenvp[newenvppos] = NULL;

    strncpy(argv0, filename, FAKECHROOT_PATH_MAX);

    r = try_cmd_subst (getenv ("FAKECHROOT_CMD_SUBST"), filename, tmp);
    if (r) {
        filename = tmp;

        /* FAKECHROOT_CMD_SUBST escapes the chroot.  newenvp here does
         * not contain LD_PRELOAD and the other special environment
         * variables.
         */
        return nextcall(execve)(filename, argv, newenvp);
    }

    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    strcpy(tmp, filename);
    filename = tmp;

    if ((file = nextcall(open)(filename, O_RDONLY)) == -1) {
        errno = ENOENT;
        return -1;
    }

    i = read(file, hashbang, FAKECHROOT_PATH_MAX-2);
    close(file);
    if (i == -1) {
        errno = ENOENT;
        return -1;
    }

    /* Add our variables to newenvp */
    newenvp = realloc( newenvp, (newenvppos + nr_envkey + 1) * sizeof(char *) );
    if (newenvp == NULL) {
        errno = ENOMEM;
        return -1;
    }
    for (j = 0; j < nr_envkey; j++) {
        env = getenv(envkey[j]);
        if (env != NULL) {
            newenvp[newenvppos] = malloc(strlen(envkey[j]) + strlen(env) + 3);
            strcpy(newenvp[newenvppos], envkey[j]);
            strcat(newenvp[newenvppos], "=");
            strcat(newenvp[newenvppos], env);
            newenvppos++;
        }
    }
    newenvp[newenvppos] = NULL;

    /* No hashbang in argv */
    if (hashbang[0] != '#' || hashbang[1] != '!')
        return nextcall(execve)(filename, argv, newenvp);

    /* For hashbang we must fix argv[0] */
    hashbang[i] = hashbang[i+1] = 0;
    for (i = j = 2; (hashbang[i] == ' ' || hashbang[i] == '\t') && i < FAKECHROOT_PATH_MAX; i++, j++);
    for (n = 0; i < FAKECHROOT_PATH_MAX; i++) {
        c = hashbang[i];
        if (hashbang[i] == 0 || hashbang[i] == ' ' || hashbang[i] == '\t' || hashbang[i] == '\n') {
            hashbang[i] = 0;
            if (i > j) {
                if (n == 0) {
                    ptr = &hashbang[j];
                    expand_chroot_path(ptr, fakechroot_path, fakechroot_buf);
                    strcpy(newfilename, ptr);
                }
                newargv[n++] = &hashbang[j];
            }
            j = i + 1;
        }
        if (c == '\n' || c == 0)
            break;
    }

    newargv[n++] = argv0;

    for (i = 1; argv[i] != NULL && i<argv_max; ) {
        newargv[n++] = argv[i++];
    }

    newargv[n] = 0;

    return nextcall(execve)(newfilename, (char * const *)newargv, newenvp);
}


/* execvp function reimplementation taken from GNU C Library.
   Copyright (C) 1991,92, 1995-99, 2002, 2004, 2005, 2007, 2009
   Free Software Foundation, Inc.
 */
/* #include <unistd.h> */
int execvp (const char *file, char * const argv[])
{
    debug("execvp(\"%s\", {\"%s\", ...})", file, argv[0]);
    if (*file == '\0') {
        /* We check the simple case first. */
        errno = ENOENT;
        return -1;
    }

    if (strchr(file, '/') != NULL) {
        /* Don't search when it contains a slash.  */
        return execve(file, argv, environ);
    } else {
        int got_eacces = 0;
        char *path, *p, *name;
        size_t len;
        size_t pathlen;

        path = getenv("PATH");
        if (path == NULL) {
            /* There is no `PATH' in the environment.
             The default search path is the current directory
             followed by the path `confstr' returns for `_CS_PATH'.  */
            len = confstr(_CS_PATH, (char *) NULL, 0);
            path = (char *) alloca(1 + len);
            path[0] = ':';
            (void) confstr(_CS_PATH, path + 1, len);
        }

        len = strlen(file) + 1;
        pathlen = strlen(path);
        name = alloca(pathlen + len + 1);
        /* Copy the file name at the top.  */
        name = (char *) memcpy(name + pathlen + 1, file, len);
        /* And add the slash.  */
        *--name = '/';

        p = path;
        do {
            char *startp;

            path = p;
            p = strchrnul(path, ':');

            if (p == path)
                /* Two adjacent colons, or a colon at the beginning or the end
                 of `PATH' means to search the current directory.  */
                startp = name + 1;
            else
                startp = (char *) memcpy(name - (p - path), path, p - path);

            /* Try to execute this name.  If it works, execv will not return.  */
            execve(startp, argv, environ);

            switch (errno) {
            case EACCES:
                /* Record the we got a `Permission denied' error.  If we end
                 up finding no executable we can use, we want to diagnose
                 that we did find one but were denied access.  */
                got_eacces = 1;
            case ENOENT:
            case ESTALE:
            case ENOTDIR:
                /* Those errors indicate the file is missing or not executable
                 by us, in which case we want to just try the next path
                 directory.  */
                break;

            default:
                /* Some other error means we found an executable file, but
                 something went wrong executing it; return the error to our
                 caller.  */
                return -1;
            }
        } while (*p++ != '\0');

        /* We tried every element and none of them worked.  */
        if (got_eacces)
            /* At least one failure was due to permissions, so report that
             error.  */
            errno = EACCES;
    }

    /* Return the error from the last attempt (probably ENOENT).  */
    return -1;
}


#ifdef HAVE_FCHMODAT
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
/* #include <sys/stat.h> */
int fchmodat (int dirfd, const char *path, mode_t mode, int flag)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("fchmodat(%d, \"%s\", 0%od, %d)", dirfd, path, mode, flag);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(fchmodat)(dirfd, path, mode, flag);
}
#endif


/* #define _ATFILE_SOURCE */
#ifdef HAVE_FCHOWNAT
/* #include <fcntl.h> */
/* #include <unistd.h> */
int fchownat (int dirfd, const char *path, uid_t owner, gid_t group, int flag)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("fchownat(%d, \"%s\", %d, %d, %d)", dirfd, path, owner, group, flag);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(fchownat)(dirfd, path, owner, group, flag);
}
#endif


/* #include <stdio.h> */
FILE *fopen (const char *path, const char *mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("fopen(\"%s\", \"%s\")", path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(fopen)(path, mode);
}


#ifdef HAVE_FOPEN64
/* #include <stdio.h> */
FILE *fopen64 (const char *path, const char *mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("fopen64(\"%s\", \"%s\")", path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(fopen64)(path, mode);
}
#endif


/* #include <stdio.h> */
FILE *freopen (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("freopen(\"%s\", \"%s\", &stream)", path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(freopen)(path, mode, stream);
}


#ifdef HAVE_FREOPEN64
/* #include <stdio.h> */
FILE *freopen64 (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("freopen64(\"%s\", \"%s\", &stream)", path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(freopen64)(path, mode, stream);
}
#endif


#ifdef HAVE_FTS_CHILDREN
#if !defined(HAVE___OPENDIR2)
/* #include <fts.h> */
FTSENT * fts_children (FTS *ftsp, int options)
{
    FTSENT *entry, *newentry;
    char *fakechroot_path, *fakechroot_ptr;

    debug("fts_children(&ftsp, %d)", options);
    if ((entry = nextcall(fts_read)(ftsp)) == NULL)
        return NULL;

    /* Memory leak. We can't free this resource. */
    if ((newentry = malloc(sizeof(FTSENT))) == NULL)
        return NULL;

    memmove(newentry, entry, sizeof(FTSENT));

    if ((newentry->fts_path = malloc(entry->fts_pathlen + 1)) == NULL)
        return NULL;

    memmove(newentry->fts_path, entry->fts_path, entry->fts_pathlen);
    narrow_chroot_path(newentry->fts_path, fakechroot_path, fakechroot_ptr);

    return newentry;
}
#endif
#endif


#ifdef HAVE_FTS_OPEN
#if !defined(HAVE___OPENDIR2)
FTS * fts_open (char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **))
{
    char *fakechroot_path, *fakechroot_buf;
    char *path;
    char * const *p;
    char **new_path_argv;
    char **np;
    int n;

    debug("fts_open({\"%s\", ...}, %d, &compar)", path_argv[0], options);
    for (n=0, p=path_argv; *p; n++, p++);
    if ((new_path_argv = malloc(n*(sizeof(char *)))) == NULL) {
        return NULL;
    }

    for (n=0, p=path_argv, np=new_path_argv; *p; n++, p++, np++) {
        path = *p;
        expand_chroot_path_malloc(path, fakechroot_path, fakechroot_buf);
        *np = path;
    }

    return nextcall(fts_open)(new_path_argv, options, compar);
}
#endif
#endif


#ifdef HAVE_FTS_READ
#if !defined(HAVE___OPENDIR2)
FTSENT * fts_read (FTS *ftsp)
{
    FTSENT *entry, *newentry;
    char *fakechroot_path, *fakechroot_ptr;

    debug("fts_read(&ftsp)");
    if ((entry = nextcall(fts_read)(ftsp)) == NULL)
        return NULL;

    /* Memory leak. We can't free this resource. */
    if ((newentry = malloc(sizeof(FTSENT))) == NULL)
        return NULL;

    memmove(newentry, entry, sizeof(FTSENT));

    if ((newentry->fts_path = malloc(entry->fts_pathlen + 1)) == NULL)
        return NULL;

    memmove(newentry->fts_path, entry->fts_path, entry->fts_pathlen);
    narrow_chroot_path(newentry->fts_path, fakechroot_path, fakechroot_ptr);

    return newentry;
}
#endif
#endif


#ifdef HAVE_FTW
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
/* include <ftw.h> */
static int (*ftw_fn_saved)(const char *file, const struct stat *sb, int flag);

static int ftw_fn_wrapper (const char *file, const struct stat *sb, int flag)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return ftw_fn_saved(file, sb, flag);
}

int ftw (const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("ftw(\"%s\", &fn, %d)", dir, nopenfd);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    ftw_fn_saved = fn;
    return nextcall(ftw)(dir, &ftw_fn_wrapper, nopenfd);
}
#endif
#endif


#ifdef HAVE_FTW64
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW64)
/* include <ftw.h> */
static int (*ftw64_fn_saved)(const char *file, const struct stat64 *sb, int flag);

static int ftw64_fn_wrapper (const char *file, const struct stat64 *sb, int flag)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return ftw64_fn_saved(file, sb, flag);
}

int ftw64 (const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag), int nopenfd)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("ftw64(\"%s\", &fn, %d)", dir, nopenfd);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    ftw64_fn_saved = fn;
    return nextcall(ftw64)(dir, &ftw64_fn_wrapper, nopenfd);
}
#endif
#endif


#ifdef HAVE_FUTIMESAT
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
int futimesat (int fd, const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("futimesat(%d, \"%s\", &tv)", fd, filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(futimesat)(fd, filename, tv);
}
#endif


#ifdef HAVE_GET_CURRENT_DIR_NAME
/* #include <unistd.h> */
char * get_current_dir_name (void)
{
    char *cwd, *oldptr, *newptr;
    char *fakechroot_path, *fakechroot_ptr;

    debug("get_current_dir_name()");
    if ((cwd = nextcall(get_current_dir_name)()) == NULL) {
        return NULL;
    }
    oldptr = cwd;
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    if (cwd == NULL) {
        return NULL;
    }
    if ((newptr = malloc(strlen(cwd)+1)) == NULL) {
        free(oldptr);
        return NULL;
    }
    strcpy(newptr, cwd);
    free(oldptr);
    return newptr;
}
#endif


/* #include <unistd.h> */
char * getcwd (char *buf, size_t size)
{
    char *cwd;
    char *fakechroot_path, *fakechroot_ptr;

    debug("getcwd(&buf, %zd)", size);
    if ((cwd = nextcall(getcwd)(buf, size)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    return cwd;
}


#ifdef AF_UNIX
/* #include <sys/socket.h> */
/* #include <sys/un.h> */

#ifdef HAVE_GETPEERNAME_TYPE_ARG2___SOCKADDR_ARG__
#define GETPEERNAME_SOCKADDR_UN(addr) SOCKADDR_UN_UNION(addr)
#else
#define GETPEERNAME_SOCKADDR_UN(addr) SOCKADDR_UN(addr)
#endif

int getpeername (int s, GETPEERNAME_TYPE_ARG2(addr), socklen_t *addrlen)
{
    int status;
    socklen_t newaddrlen;
    struct sockaddr_un newaddr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("getpeername(%d, &addr, &addrlen)", s);
    newaddrlen = sizeof(struct sockaddr_un);
    memset(&newaddr, 0, newaddrlen);
    status = nextcall(getpeername)(s, (struct sockaddr *)&newaddr, &newaddrlen);
    if (status != 0) {
        return status;
    }
    if (newaddr.sun_family == AF_UNIX && newaddr.sun_path && *(newaddr.sun_path)) {
        strncpy(fakechroot_buf, newaddr.sun_path, FAKECHROOT_PATH_MAX);
        narrow_chroot_path(fakechroot_buf, fakechroot_path, fakechroot_ptr);
        strncpy(newaddr.sun_path, fakechroot_buf, UNIX_PATH_MAX);
    }

    memcpy(GETPEERNAME_SOCKADDR_UN(addr), &newaddr, *addrlen < sizeof(struct sockaddr_un) ? *addrlen : sizeof(struct sockaddr_un));
    *addrlen = SUN_LEN(&newaddr);
    return status;
}
#endif


#ifdef AF_UNIX
/* #include <sys/socket.h> */
/* #include <sys/un.h> */

#ifdef HAVE_GETSOCKNAME_TYPE_ARG2___SOCKADDR_ARG__
#define GETSOCKNAME_SOCKADDR_UN(addr) SOCKADDR_UN_UNION(addr)
#else
#define GETSOCKNAME_SOCKADDR_UN(addr) SOCKADDR_UN(addr)
#endif

int getsockname (int s, GETSOCKNAME_TYPE_ARG2(addr), socklen_t *addrlen)
{
    int status;
    socklen_t newaddrlen;
    struct sockaddr_un newaddr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("getsockname(%d, &addr, &addrlen)", s);
    newaddrlen = sizeof(struct sockaddr_un);
    memset(&newaddr, 0, newaddrlen);
    status = nextcall(getsockname)(s, (struct sockaddr *)&newaddr, &newaddrlen);
    if (status != 0) {
        return status;
    }
    if (newaddr.sun_family == AF_UNIX && newaddr.sun_path && *(newaddr.sun_path)) {
        strncpy(fakechroot_buf, newaddr.sun_path, FAKECHROOT_PATH_MAX);
        narrow_chroot_path(fakechroot_buf, fakechroot_path, fakechroot_ptr);
        strncpy(newaddr.sun_path, fakechroot_buf, UNIX_PATH_MAX);
    }

    memcpy(GETSOCKNAME_SOCKADDR_UN(addr), &newaddr, *addrlen < sizeof(struct sockaddr_un) ? *addrlen : sizeof(struct sockaddr_un));
    *addrlen = SUN_LEN(&newaddr);
    return status;
}
#endif


#ifdef HAVE_GETWD
/* #include <unistd.h> */
char * getwd (char *buf)
{
    char *cwd;
    char *fakechroot_path, *fakechroot_ptr;

    debug("getwd(&buf)");
    if ((cwd = nextcall(getwd)(buf)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    return cwd;
}
#endif


#ifdef HAVE_GETXATTR
/* #include <sys/xattr.h> */
ssize_t getxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("getxattr(\"%s\", \"%s\", &value, %zd)", path, name, size);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(getxattr)(path, name, value, size);
}
#endif


/* #include <glob.h> */
int glob (const char *pattern, int flags, int (*errfunc) (const char *, int), glob_t *pglob)
{
    int rc,i;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("glob(\"%s\", %d, &errfunc, &pglob)", pattern, flags);
    expand_chroot_path(pattern, fakechroot_path, fakechroot_buf);

    rc = nextcall(glob)(pattern, flags, errfunc, pglob);
    if (rc < 0)
        return rc;

    for(i = 0; i < pglob->gl_pathc; i++) {
        strcpy(tmp,pglob->gl_pathv[i]);
        fakechroot_path = getenv("FAKECHROOT_BASE");
        if (fakechroot_path != NULL) {
            fakechroot_ptr = strstr(tmp, fakechroot_path);
            if (fakechroot_ptr != tmp) {
                tmpptr = tmp;
            } else {
                tmpptr = tmp + strlen(fakechroot_path);
            }
            strcpy(pglob->gl_pathv[i], tmpptr);
        }
    }
    return rc;
}


#ifdef HAVE_GLOB64
/* #include <glob.h> */
int glob64 (const char *pattern, int flags, int (*errfunc) (const char *, int), glob64_t *pglob)
{
    int rc,i;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("glob64(\"%s\", %d, &errfunc, &pglob)", pattern, flags);
    expand_chroot_path(pattern, fakechroot_path, fakechroot_buf);

    rc = nextcall(glob64)(pattern, flags, errfunc, pglob);
    if (rc < 0)
        return rc;

    for(i = 0; i < pglob->gl_pathc; i++) {
        strcpy(tmp,pglob->gl_pathv[i]);
        fakechroot_path = getenv("FAKECHROOT_BASE");
        if (fakechroot_path != NULL) {
            fakechroot_ptr = strstr(tmp, fakechroot_path);
            if (fakechroot_ptr != tmp) {
                tmpptr = tmp;
            } else {
                tmpptr = tmp + strlen(fakechroot_path);
            }
            strcpy(pglob->gl_pathv[i], tmpptr);
        }
    }
    return rc;
}
#endif


#ifdef HAVE_GLOB_PATTERN_P
/* #include <glob.h> */
int glob_pattern_p (const char *pattern, int quote)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("glob_pattern_p(\"%s\", %d)", pattern, quote);
    expand_chroot_path(pattern, fakechroot_path, fakechroot_buf);
    return nextcall(glob_pattern_p)(pattern, quote);
}
#endif


#ifdef HAVE_INOTIFY_ADD_WATCH
/* #include <sys/inotify.h> */
int inotify_add_watch (int fd, const char *pathname, uint32_t mask)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("inotify_add_watch(%d, \"%s\", %d)", fd, pathname, mask);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(inotify_add_watch)(fd, pathname, mask);
}
#endif


#ifdef HAVE_LCHMOD
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int lchmod (const char *path, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("lchmod(\"%s\", 0%od)", path, mode);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(lchmod)(path, mode);
}
#endif


/* #include <sys/types.h> */
/* #include <unistd.h> */
int lchown (const char *path, uid_t owner, gid_t group)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("lchown(\"%s\", %d, %d)", path, owner, group);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(lchown)(path, owner, group);
}


#ifdef HAVE_LCKPWDF
/* #include <shadow.h> */
int lckpwdf (void)
{
    debug("lckpwdf()");
    return 0;
}
#endif


#ifdef HAVE_LGETXATTR
/* #include <sys/xattr.h> */
ssize_t lgetxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("lgetxattr(\"%s\", \"%s\", &value, %zd)", path, name, size);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(lgetxattr)(path, name, value, size);
}
#endif


/* #include <unistd.h> */
int link (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("link(\"%s\", \"%s\")", oldpath, newpath);
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_buf);
    return nextcall(link)(oldpath, newpath);
}


#ifdef HAVE_LINKAT
/* #include <unistd.h> */
int linkat (int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("linkat(%d, \"%s\", %d, \"%s\", %d)", olddirfd, oldpath, newdirfd, newpath, flags);
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_buf);
    return nextcall(linkat)(olddirfd, oldpath, newdirfd, newpath, flags);
}
#endif


#ifdef HAVE_LISTXATTR
/* #include <sys/xattr.h> */
ssize_t listxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("listxattr(\"%s\", &list, %zd)", path, list);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(listxattr)(path, list, size);
}
#endif


#ifdef HAVE_LLISTXATTR
/* #include <sys/xattr.h> */
ssize_t llistxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("llistxattr(\"%s\", &list, %zd)", path, list);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(llistxattr)(path, list, size);
}
#endif


#ifdef HAVE_LREMOVEXATTR
/* #include <sys/xattr.h> */
int lremovexattr (const char *path, const char *name)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("lremovexattr(\"%s\", \"%s\")", path, name);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(lremovexattr)(path, name);
}
#endif


#ifdef HAVE_LSETXATTR
/* #include <sys/xattr.h> */
int lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("lsetxattr(\"%s\", \"%s\", &value, %zd, %d)", path, name, size, flags);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(lsetxattr)(path, name, value, size, flags);
}
#endif


/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat (const char *file_name, struct stat *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX], tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN status;
    const char *orig;

    debug("lstat(\"%s\", &buf)", file_name);
    orig = file_name;
    expand_chroot_path(file_name, fakechroot_path, fakechroot_buf);
    retval = nextcall(lstat)(file_name, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((buf->st_mode & S_IFMT) == S_IFLNK)
        if ((status = readlink(orig, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = status;
    return retval;
}


#ifdef HAVE_LSTAT64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX], tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN status;
    const char *orig;

    debug("lstat64(\"%s\", &buf)", file_name);
    orig = file_name;
    expand_chroot_path(file_name, fakechroot_path, fakechroot_buf);
    retval = nextcall(lstat64)(file_name, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((buf->st_mode & S_IFMT) == S_IFLNK)
        if ((status = readlink(orig, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = status;
    return retval;
}
#endif


#ifdef HAVE_LUTIMES
/* #include <sys/time.h> */
int lutimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("lutimes(\"%s\", &tv)", filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(lutimes)(filename, tv);
}
#endif


/* #include <sys/stat.h> */
/* #include <sys/types.h> */
int mkdir (const char *pathname, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("mkdir(\"%s\", 0%od)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(mkdir)(pathname, mode);
}


#ifdef HAVE_MKDIRAT
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
/* #include <sys/stat.h> */
int mkdirat (int dirfd, const char *pathname, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("mkdirat(%d, \"%s\", 0%od)", dirfd, pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(mkdirat)(dirfd, pathname, mode);
}
#endif


#ifdef HAVE_MKDTEMP
/* #include <stdlib.h> */
char *mkdtemp (char *template)
{
    char tmp[FAKECHROOT_PATH_MAX], *oldtemplate, *ptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("mkdtemp(\"%s\")", template);
    oldtemplate = template;

    expand_chroot_path(template, fakechroot_path, fakechroot_buf);

    if (nextcall(mkdtemp)(template) == NULL) {
        return NULL;
    }
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr, fakechroot_path, fakechroot_ptr);
    if (ptr == NULL) {
        return NULL;
    }
    strcpy(oldtemplate, ptr);
    return oldtemplate;
}
#endif


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int mkfifo (const char *pathname, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("mkfifo(\"%s\", 0%od)", pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(mkfifo)(pathname, mode);
}


#ifdef HAVE_MKFIFOAT
/* #include <sys/stat.h> */
int mkfifoat (int dirfd, const char *pathname, mode_t mode)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("mkfifoat(%d, \"%s\", 0%od)", dirfd, pathname, mode);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(mkfifoat)(dirfd, pathname, mode);
}
#endif


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
/* #include <unistd.h> */
int mknod (const char *pathname, mode_t mode, dev_t dev)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("mknod(\"%s\", 0%od, %ld)", pathname, mode, dev);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(mknod)(pathname, mode, dev);
}


#ifdef HAVE_MKNODAT
/* #include <fcntl.h> */
/* #include <sys/stat.h> */
int mknodat (int dirfd, const char *pathname, mode_t mode, dev_t dev)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("mknodat(%d, \"%s\", 0%od, %ld)", dirfd, pathname, mode, dev);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(mknodat)(dirfd, pathname, mode, dev);
}
#endif


/* #include <stdlib.h> */
int mkstemp (char *template)
{
    char tmp[FAKECHROOT_PATH_MAX], *oldtemplate, *ptr;
    int fd;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("mkstemp(\"%s\")", template);
    oldtemplate = template;

    expand_chroot_path(template, fakechroot_path, fakechroot_buf);

    if ((fd = nextcall(mkstemp)(template)) == -1) {
        return -1;
    }
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr, fakechroot_path, fakechroot_ptr);
    if (ptr != NULL) {
        strcpy(oldtemplate, ptr);
    }
    return fd;
}


#ifdef HAVE_MKSTEMP64
/* #include <stdlib.h> */
int mkstemp64 (char *template)
{
    char tmp[FAKECHROOT_PATH_MAX], *oldtemplate, *ptr;
    int fd;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("mkstemp64(\"%s\")", template);
    oldtemplate = template;

    expand_chroot_path(template, fakechroot_path, fakechroot_buf);

    if ((fd = nextcall(mkstemp64)(template)) == -1) {
        return -1;
    }
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr, fakechroot_path, fakechroot_ptr);
    if (ptr != NULL) {
        strcpy(oldtemplate, ptr);
    }
    return fd;
}
#endif


/* #include <stdlib.h> */
char *mktemp (char *template)
{
    char tmp[FAKECHROOT_PATH_MAX], *ptr, *ptr2;
    char *fakechroot_path, *fakechroot_ptr, *fakechroot_buf;
    int localdir = 0;

    debug("mktemp(\"%s\")", template);
    tmp[FAKECHROOT_PATH_MAX-1] = '\0';
    strncpy(tmp, template, FAKECHROOT_PATH_MAX-2);
    ptr = tmp;

    if (!fakechroot_localdir(ptr)) {
        localdir = 1;
        expand_chroot_path_malloc(ptr, fakechroot_path, fakechroot_buf);
    }

    if (nextcall(mktemp)(ptr) == NULL) {
        if (!localdir) free(ptr);
        return NULL;
    }

    ptr2 = ptr;
    narrow_chroot_path(ptr2, fakechroot_path, fakechroot_ptr);

    strncpy(template, ptr2, strlen(template));
    if (!localdir) free(ptr);
    return template;
}


#ifdef HAVE_NFTW
/* #include <ftw.h> */
static int (*nftw_fn_saved)(const char *file, const struct stat *sb, int flag, struct FTW *s);

static int nftw_fn_wrapper (const char *file, const struct stat *sb, int flag, struct FTW *s)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return nftw_fn_saved(file, sb, flag, s);
}

int nftw (const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag, struct FTW *s), int nopenfd, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("ntfw(\"%s\", &fn, %d, %d)", dir, nopenfd, flags);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    nftw_fn_saved = fn;
    return nextcall(nftw)(dir, nftw_fn_wrapper, nopenfd, flags);
}
#endif


#ifdef HAVE_NFTW64
/* #include <ftw.h> */
static int (*nftw64_fn_saved)(const char *file, const struct stat64 *sb, int flag, struct FTW *s);

static int nftw64_fn_wrapper (const char *file, const struct stat64 *sb, int flag, struct FTW *s)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return nftw64_fn_saved(file, sb, flag, s);
}

int nftw64 (const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag, struct FTW *s), int nopenfd, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("ntfw64(\"%s\", &fn, %d, %d)", dir, nopenfd, flags);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    nftw64_fn_saved = fn;
    return nextcall(nftw64)(dir, nftw64_fn_wrapper, nopenfd, flags);
}
#endif


#ifdef USE_ALIAS
extern int __REDIRECT (__open_alias, (const char *pathname, int flags, ...), open);
#else
#define __open_alias open
#endif
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int __open_alias (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("open(\"%s\", %d, ...)", pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return nextcall(open)(pathname, flags, mode);
}


#ifdef HAVE_OPEN64
#ifdef USE_ALIAS
extern int __REDIRECT (__open64_alias, (const char *pathname, int flags, ...), open64);
#else
#define __open64_alias open64
#endif
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int __open64_alias (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("open64(\"%s\", %d, ...)", pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return nextcall(open64)(pathname, flags, mode);
}
#endif


#ifdef HAVE_OPENAT
#ifdef USE_ALIAS
extern int __REDIRECT (__openat_alias, (int dirfd, const char *pathname, int flags, ...), openat);
#else
#define __openat_alias openat
#endif
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
int __openat_alias (int dirfd, const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("openat(%d, \"%s\", %d, ...)", dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return nextcall(openat)(dirfd, pathname, flags, mode);
}
#endif


#ifdef HAVE_OPENAT64
#ifdef USE_ALIAS
extern int __REDIRECT (__openat64_alias, (int dirfd, const char *pathname, int flags, ...), openat64);
#else
#define __openat64_alias openat64
#endif
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
int __openat64_alias (int dirfd, const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("openat64(%d, \"%s\", %d, ...)", dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return nextcall(openat64)(dirfd, pathname, flags, mode);
}
#endif


#if !defined(HAVE___OPENDIR2)
/* #include <sys/types.h> */
/* #include <dirent.h> */
DIR *opendir (const char *name)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("opendir(\"%s\")", name);
    expand_chroot_path(name, fakechroot_path, fakechroot_buf);
    return nextcall(opendir)(name);
}
#endif


/* #include <unistd.h> */
long pathconf (const char *path, int name)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("pathconf(\"%s\", %d)", path, name);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(pathconf)(path, name);
}


#ifdef __GNUC__
/*
   popen function reimplementation taken from uClibc.
   Copyright (C) 2004       Manuel Novoa III    <mjn3@codepoet.org>
   Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 */
/* #include <stdio.h> */
struct popen_list_item {
    struct popen_list_item *next;
    FILE *f;
    pid_t pid;
};

static struct popen_list_item *popen_list /* = NULL (bss initialized) */;

FILE *popen (const char *command, const char *modes)
{
    FILE *fp;
    struct popen_list_item *pi;
    struct popen_list_item *po;
    int pipe_fd[2];
    int parent_fd;
    int child_fd;
    int child_writing;                  /* Doubles as the desired child fildes. */
    pid_t pid;

    debug("popen(\"%s\", \"%s\")", command, modes);
    child_writing = 0;                  /* Assume child is writing. */
    if (modes[0] != 'w') {              /* Parent not writing... */
        ++child_writing;                /* so child must be writing. */
        if (modes[0] != 'r') {  /* Oops!  Parent not reading either! */
            __set_errno(EINVAL);
            goto RET_NULL;
        }
    }

    if (!(pi = malloc(sizeof(struct popen_list_item)))) {
        goto RET_NULL;
    }

    if (pipe(pipe_fd)) {
        goto FREE_PI;
    }

    child_fd = pipe_fd[child_writing];
    parent_fd = pipe_fd[1-child_writing];

    if (!(fp = fdopen(parent_fd, modes))) {
        close(parent_fd);
        close(child_fd);
        goto FREE_PI;
    }

    if ((pid = vfork()) == 0) { /* Child of vfork... */
        close(parent_fd);
        if (child_fd != child_writing) {
            dup2(child_fd, child_writing);
            close(child_fd);
        }

        /* SUSv3 requires that any previously popen()'d streams in the
         * parent shall be closed in the child. */
        for (po = popen_list ; po ; po = po->next) {
            fclose(po->f);
        }

        execl("/bin/sh", "sh", "-c", command, (char *)0);

        /* SUSv3 mandates an exit code of 127 for the child if the
         * command interpreter can not be invoked. */
        _exit(127);
    }

    /* We need to close the child filedes whether vfork failed or
     * it succeeded and we're in the parent. */
    close(child_fd);

    if (pid > 0) {                              /* Parent of vfork... */
        pi->pid = pid;
        pi->f = fp;
        pi->next = popen_list;
        popen_list = pi;

        return fp;
    }

    /* If we get here, vfork failed. */
    fclose(fp);                                 /* Will close parent_fd. */

FREE_PI:
    free(pi);

RET_NULL:
    return NULL;
}
#endif


/* #include <unistd.h> */
READLINK_TYPE_RETURN readlink (const char *path, char *buf, READLINK_TYPE_ARG3(bufsiz))
{
    int status;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("readlink(\"%s\", &buf, %d)", path, bufsiz);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);

    if ((status = nextcall(readlink)(path, tmp, FAKECHROOT_PATH_MAX-1)) == -1) {
        return -1;
    }
    tmp[status] = '\0';

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + strlen(fakechroot_path);
            status -= strlen(fakechroot_path);
        }
        if (strlen(tmpptr) > bufsiz) {
            status = bufsiz;
        }
        strncpy(buf, tmpptr, status);
    } else {
        strncpy(buf, tmp, status);
    }
    return status;
}


#ifdef HAVE_READLINKAT
/* #include <unistd.h> */
ssize_t readlinkat (int dirfd, const char *path, char *buf, size_t bufsiz)
{
    int status;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("readlinkat(%d, \"%s\", &buf, %zd)", dirfd, path, bufsiz);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);

    if ((status = nextcall(readlinkat)(dirfd, path, tmp, FAKECHROOT_PATH_MAX-1)) == -1) {
        return -1;
    }
    tmp[status] = '\0';

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + strlen(fakechroot_path);
            status -= strlen(fakechroot_path);
        }
        if (strlen(tmpptr) > bufsiz) {
            status = bufsiz;
        }
        strncpy(buf, tmpptr, status);
    } else {
        strncpy(buf, tmp, status);
    }
    return status;
}
#endif


/*
   realpath function reimplementation taken from Gnulib.
   Copyright (C) 1996-2010 Free Software Foundation, Inc.
*/
/* #include <stdlib.h> */

#ifndef MAXSYMLINKS
# ifdef SYMLOOP_MAX
#  define MAXSYMLINKS SYMLOOP_MAX
# else
#  define MAXSYMLINKS 20
# endif
#endif

#ifndef DOUBLE_SLASH_IS_DISTINCT_ROOT
# define DOUBLE_SLASH_IS_DISTINCT_ROOT 0
#endif

char *realpath (const char *name, char *resolved)
{
    char *rpath, *dest, *extra_buf = NULL;
    const char *start, *end, *rpath_limit;
    long int path_max;
    int num_links = 0;

    debug("realpath(\"%s\", &resolved)", name);
    if (name == NULL) {
        /* As per Single Unix Specification V2 we must return an error if
           either parameter is a null pointer.  We extend this to allow
           the RESOLVED parameter to be NULL in case the we are expected to
           allocate the room for the return value.  */
        __set_errno (EINVAL);
        return NULL;
    }

    if (name[0] == '\0') {
        /* As per Single Unix Specification V2 we must return an error if
           the name argument points to an empty string.  */
        __set_errno (ENOENT);
        return NULL;
    }

    path_max = FAKECHROOT_PATH_MAX;

    if (resolved == NULL) {
        rpath = malloc (path_max);
        if (rpath == NULL) {
            /* It's easier to set errno to ENOMEM than to rely on the
               'malloc-posix' gnulib module.  */
            errno = ENOMEM;
            return NULL;
        }
    } else
        rpath = resolved;
    rpath_limit = rpath + path_max;

    if (name[0] != '/') {
        if (!getcwd (rpath, path_max)) {
            rpath[0] = '\0';
            goto error;
        }
        dest = strchr (rpath, '\0');
    } else {
        rpath[0] = '/';
        dest = rpath + 1;
        if (DOUBLE_SLASH_IS_DISTINCT_ROOT && name[1] == '/')
            *dest++ = '/';
    }

    for (start = end = name; *start; start = end) {
#ifdef HAVE___LXSTAT64
        struct stat64 st;
#else
        struct stat st;
#endif
        int n;

        /* Skip sequence of multiple path-separators.  */
        while (*start == '/')
            ++start;

        /* Find end of path component.  */
        for (end = start; *end && *end != '/'; ++end)
            /* Nothing.  */;

        if (end - start == 0)
            break;
        else if (end - start == 1 && start[0] == '.')
            /* nothing */;
        else if (end - start == 2 && start[0] == '.' && start[1] == '.') {
            /* Back up to previous component, ignore if at root already.  */
            if (dest > rpath + 1)
                while ((--dest)[-1] != '/');
            if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rpath + 1
                    && *dest == '/')
                dest++;
        } else {
            size_t new_size;

            if (dest[-1] != '/')
                *dest++ = '/';

            if (dest + (end - start) >= rpath_limit) {
                ptrdiff_t dest_offset = dest - rpath;
                char *new_rpath;

                if (resolved) {
                    __set_errno (ENAMETOOLONG);
                    if (dest > rpath + 1)
                        dest--;
                    *dest = '\0';
                    goto error;
                }
                new_size = rpath_limit - rpath;
                if (end - start + 1 > path_max)
                    new_size += end - start + 1;
                else
                    new_size += path_max;
                new_rpath = (char *) realloc (rpath, new_size);
                if (new_rpath == NULL) {
                    /* It's easier to set errno to ENOMEM than to rely on the
                       'realloc-posix' gnulib module.  */
                    errno = ENOMEM;
                    goto error;
                }
                rpath = new_rpath;
                rpath_limit = rpath + new_size;

                dest = rpath + dest_offset;
            }

            memcpy (dest, start, end - start);
            dest += end - start;
            *dest = '\0';

#ifdef HAVE___LXSTAT64
            if (__lxstat64 (_STAT_VER, rpath, &st) < 0)
                goto error;
#else
            if (lstat (rpath, &st) < 0)
                goto error;
#endif

            if (S_ISLNK (st.st_mode)) {
                char *buf;
                size_t len;

                if (++num_links > MAXSYMLINKS) {
                    __set_errno (ELOOP);
                    goto error;
                }

                buf = malloc (path_max);
                if (!buf) {
                    errno = ENOMEM;
                    goto error;
                }

                n = readlink (rpath, buf, path_max - 1);
                if (n < 0) {
                    int saved_errno = errno;
                    free (buf);
                    errno = saved_errno;
                    goto error;
                }
                buf[n] = '\0';

                if (!extra_buf) {
                    extra_buf = malloc (path_max);
                    if (!extra_buf) {
                        free (buf);
                        errno = ENOMEM;
                        goto error;
                    }
                }

                len = strlen (end);
                if ((long int) (n + len) >= path_max) {
                    free (buf);
                    __set_errno (ENAMETOOLONG);
                    goto error;
                }

                /* Careful here, end may be a pointer into extra_buf... */
                memmove (&extra_buf[n], end, len + 1);
                name = end = memcpy (extra_buf, buf, n);

                if (buf[0] == '/') {
                    dest = rpath + 1;     /* It's an absolute symlink */
                    if (DOUBLE_SLASH_IS_DISTINCT_ROOT && buf[1] == '/')
                        *dest++ = '/';
                } else {
                    /* Back up to previous component, ignore if at root
                       already: */
                    if (dest > rpath + 1)
                        while ((--dest)[-1] != '/');
                    if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rpath + 1
                            && *dest == '/')
                        dest++;
                }
            } else if (!S_ISDIR (st.st_mode) && *end != '\0') {
                __set_errno (ENOTDIR);
                goto error;
            }
        }
    }
    if (dest > rpath + 1 && dest[-1] == '/')
        --dest;
    if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rpath + 1 && *dest == '/')
        dest++;
    *dest = '\0';

    if (extra_buf)
        free (extra_buf);

    return rpath;

error: {
        int saved_errno = errno;
        if (extra_buf)
            free (extra_buf);
        if (resolved == NULL)
            free (rpath);
        errno = saved_errno;
    }
    return NULL;
}


/* #include <stdio.h> */
int remove (const char *pathname)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("remove(\"%s\")", pathname);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(remove)(pathname);
}


#ifdef HAVE_REMOVEXATTR
/* #include <sys/xattr.h> */
int removexattr (const char *path, const char *name)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("removexattr(\"%s\", \"%s\")", path, name);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(removexattr)(path, name);
}
#endif


/* #include <stdio.h> */
int rename (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("rename(\"%s\", \"%s\")", oldpath, newpath);
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_buf);
    return nextcall(rename)(oldpath, newpath);
}


#ifdef HAVE_RENAMEAT
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
/* #include <stdio.h> */
int renameat (int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("renameat(%d, \"%s\", %d, \"%s\")", olddirfd, oldpath, newdirfd, newpath);
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_buf);
    return nextcall(renameat)(olddirfd, oldpath, newdirfd, newpath);
}
#endif


#ifdef HAVE_REVOKE
/* #include <unistd.h> */
int revoke (const char *file)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("revoke(\"%s\")", file);
    expand_chroot_path(file, fakechroot_path, fakechroot_buf);
    return nextcall(revoke)(file);
}
#endif


/* #include <unistd.h> */
int rmdir (const char *pathname)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("rmdir(\"%s\")", pathname);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(rmdir)(pathname);
}


#ifdef HAVE_SCANDIR
/* #include <dirent.h> */
int scandir (const char *dir, struct dirent ***namelist, SCANDIR_TYPE_ARG3(filter), SCANDIR_TYPE_ARG4(compar))
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("scandir(\"%s\", &namelist, &filter, &compar)", dir);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    return nextcall(scandir)(dir, namelist, filter, compar);
}
#endif


#ifdef HAVE_SCANDIR64
/* #include <dirent.h> */
int scandir64 (const char *dir, struct dirent64 ***namelist, SCANDIR64_TYPE_ARG3(filter), SCANDIR64_TYPE_ARG4(compar))
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("scandir64(\"%s\", &namelist, &filter, &compar)", dir);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    return nextcall(scandir64)(dir, namelist, filter, compar);
}
#endif


#ifdef HAVE_SETXATTR
/* #include <sys/xattr.h> */
int setxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("setxattr(\"%s\", \"%s\", &value, %zd, %d)", path, name, size, flags);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(setxattr)(path, name, value, size, flags);
}
#endif


/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat (const char *file_name, struct stat *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("stat(\"%s\", &buf)", file_name);
    expand_chroot_path(file_name, fakechroot_path, fakechroot_buf);
    return nextcall(stat)(file_name, buf);
}


#ifdef HAVE_STAT64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("stat64(\"%s\", &buf)", file_name);
    expand_chroot_path(file_name, fakechroot_path, fakechroot_buf);
    return nextcall(stat64)(file_name, buf);
}
#endif


#ifdef HAVE_STATFS
/* #include <sys/vfs.h> */
/* or */
/* #include <sys/param.h> */
/* #include <sys/mount.h> */
int statfs (const char *path, struct statfs *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("statfs(\"%s\", &buf)", path);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(statfs)(path, buf);
}
#endif


#ifdef HAVE_STATFS64
/* #include <sys/vfs.h> */
int statfs64 (const char *path, struct statfs64 *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("statfs64(\"%s\", &buf)", path);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(statfs64)(path, buf);
}
#endif


#ifdef HAVE_STATVFS
#if !defined(__FreeBSD__) || defined(__GLIBC__)
/* #include <sys/statvfs.h> */
int statvfs (const char *path, struct statvfs *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("statvfs(\"%s\", &buf)", path);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(statvfs)(path, buf);
}
#endif
#endif


#ifdef HAVE_STATVFS64
/* #include <sys/statvfs.h> */
int statvfs64 (const char *path, struct statvfs64 *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("statvfs64(\"%s\", &buf)", path);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(statvfs64)(path, buf);
}
#endif


/* #include <unistd.h> */
int symlink (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("symlink(\"%s\", \"%s\")", oldpath, newpath);
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_buf);
    return nextcall(symlink)(oldpath, newpath);
}


#ifdef HAVE_SYMLINKAT
/* #include <stdio.h> */
int symlinkat (const char *oldpath, int newdirfd, const char *newpath)
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("symlinkat(\"%s\", %d, \"%s\")", oldpath, newdirfd, newpath);
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_buf);
    return nextcall(symlinkat)(oldpath, newdirfd, newpath);
}
#endif


#ifdef __GNUC__
/* #include <sys/types.h> */
/* #include <signal.h> */
/* #include <stdlib.h> */
/* #include <unistd.h> */
/* #include <sys/wait.h> */
int system (const char *command)
{
    int pid, pstat;
    sigset_t mask, omask;
    struct sigaction new_action_ign, old_action_int, old_action_quit;

    debug("system(\"%s\")", command);
    if (command == 0)
        return 1;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &omask);

    if ((pid = vfork()) < 0) {
        sigprocmask(SIG_SETMASK, &omask, NULL);
        return -1;
    }
    if (pid == 0) {
        sigprocmask(SIG_SETMASK, &omask, NULL);

        execl("/bin/sh", "sh", "-c", command, (char *) 0);
        _exit(127);
    }

    new_action_ign.sa_handler = SIG_IGN;
    sigemptyset (&new_action_ign.sa_mask);
    new_action_ign.sa_flags = 0;

    sigaction(SIGINT, &new_action_ign, &old_action_int);
    sigaction(SIGQUIT, &new_action_ign, &old_action_quit);

    pid = waitpid(pid, (int *)&pstat, 0);

    sigprocmask(SIG_SETMASK, &omask, NULL);

    sigaction(SIGINT, &old_action_int, NULL);
    sigaction(SIGQUIT, &old_action_quit, NULL);

    return (pid == -1 ? -1 : pstat);
}
#endif


/* #include <stdio.h> */
char *tempnam (const char *dir, const char *pfx)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("tempnam(\"%s\", \"%s\")", dir, pfx);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    return nextcall(tempnam)(dir, pfx);
}


/* #include <stdio.h> */
char *tmpnam (char *s)
{
    char *ptr;
    char *fakechroot_path, *fakechroot_buf;

    debug("tmpnam(&s)");
    if (s != NULL) {
        return nextcall(tmpnam)(s);
    }

    ptr = nextcall(tmpnam)(NULL);
    expand_chroot_path_malloc(ptr, fakechroot_path, fakechroot_buf);
    return ptr;
}


/* #include <unistd.h> */
/* #include <sys/types.h> */
int truncate (const char *path, off_t length)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("truncate(\"%s\", %d)", path, length);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(truncate)(path, length);
}


#ifdef HAVE_TRUNCATE64
/* #include <unistd.h> */
/* #include <sys/types.h> */
int truncate64 (const char *path, off64_t length)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("truncate64(\"%s\", %d)", path, length);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);
    return nextcall(truncate64)(path, length);
}
#endif


#ifdef HAVE_ULCKPWDF
/* #include <shadow.h> */
int ulckpwdf (void)
{
    debug("ulckpwdf()");
    return 0;
}
#endif


/* #include <unistd.h> */
int unlink (const char *pathname)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("unlink(\"%s\")", pathname);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(unlink)(pathname);
}


#ifdef HAVE_UNLINKAT
/* #define _ATFILE_SOURCE */
/* #include <fcntl.h> */
int unlinkat (int dirfd, const char *pathname, int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("unlinkat(%d, \"%s\", %d)", dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(unlinkat)(dirfd, pathname, flags);
}
#endif


/* #include <sys/types.h> */
/* #include <utime.h> */
int utime (const char *filename, const struct utimbuf *buf)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("utime(\"%s\", &buf)", filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(utime)(filename, buf);
}


#ifdef HAVE_UTIMENSAT
/* #include <sys/stat.h> */
int utimensat (int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("utimeat(%d, \"%s\", &buf, %d)", dirfd, pathname, flags);
    expand_chroot_path(pathname, fakechroot_path, fakechroot_buf);
    return nextcall(utimensat)(dirfd, pathname, times, flags);
}
#endif


/* #include <sys/time.h> */
int utimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("utimes(\"%s\", &tv)", filename);
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    return nextcall(utimes)(filename, tv);
}
