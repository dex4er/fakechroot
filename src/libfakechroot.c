/*
    libfakechroot -- fake chroot environment
    (c) 2003-2005 Piotr Roszatycki <dexter@debian.org>, GPL

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

/* $Id$ */


#include "config.h"

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <string.h>
#include <glob.h>
#include <utime.h>
#include <shadow.h>
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif


#if defined(HAVE_PATH_MAX)
#define FAKECHROOT_MAXPATH PATH_MAX
#elif defined(HAVE__POSIX_PATH_MAX)
#define FAKECHROOT_MAXPATH _POSIX_PATH_MAX
#elif defined(HAVE_MAXPATHLEN)
#define FAKECHROOT_MAXPATH MAXPATHLEN
#else
#define FAKECHROOT_MAXPATH 2048
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
                        (path) = ((path) + strlen(fakechroot_path)); \
                    } \
                } \
            } \
        } \
    }

#define expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf) \
    { \
        if ((path) != NULL && *((char *)(path)) == '/') { \
            fakechroot_path = getenv("FAKECHROOT_BASE"); \
            if (fakechroot_path != NULL) { \
                fakechroot_ptr = strstr((path), fakechroot_path); \
                if (fakechroot_ptr != (path)) { \
                    strcpy(fakechroot_buf, fakechroot_path); \
                    strcat(fakechroot_buf, (path)); \
                    (path) = fakechroot_buf; \
                } \
            } \
        } \
    }

#define expand_chroot_path_malloc(path, fakechroot_path, fakechroot_ptr, fakechroot_buf) \
    { \
        if ((path) != NULL && *((char *)(path)) == '/') { \
            fakechroot_path = getenv("FAKECHROOT_BASE"); \
            if (fakechroot_path != NULL) { \
                fakechroot_ptr = strstr((path), fakechroot_path); \
                if (fakechroot_ptr != (path)) { \
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


#ifndef __GLIBC__
extern char **environ;
#endif


#ifndef HAVE_STRCHRNUL
/* Find the first occurrence of C in S or the final NUL byte.  */
char *strchrnul (const char *s, int c_in)
{
    const unsigned char *char_ptr;
    const unsigned long int *longword_ptr;
    unsigned long int longword, magic_bits, charmask;
    unsigned char c;

    c = (unsigned char) c_in;

    /* Handle the first few characters by reading one character at a time.
       Do this until CHAR_PTR is aligned on a longword boundary.  */
    for (char_ptr = s; ((unsigned long int) char_ptr
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


#ifdef HAVE___LXSTAT
static int     (*next___lxstat) (int ver, const char *filename, struct stat *buf);
#endif
#ifdef HAVE___LXSTAT64
static int     (*next___lxstat64) (int ver, const char *filename, struct stat64 *buf);
#endif
#ifdef HAVE___OPEN
static int     (*next___open) (const char *pathname, int flags, ...);
#endif
#ifdef HAVE___OPEN64
static int     (*next___open64) (const char *pathname, int flags, ...);
#endif
#ifdef HAVE___XMKNOD
static int     (*next___xmknod) (int ver, const char *path, mode_t mode, dev_t *dev);
#endif
#ifdef HAVE___XSTAT
static int     (*next___xstat) (int ver, const char *filename, struct stat *buf);
#endif
#ifdef HAVE___XSTAT64
static int     (*next___xstat64) (int ver, const char *filename, struct stat64 *buf);
#endif
static int     (*next_access) (const char *pathname, int mode);
static int     (*next_acct) (const char *filename);
#ifdef HAVE_CANONICALIZE_FILE_NAME
static char *  (*next_canonicalize_file_name) (const char *name);
#endif
static int     (*next_chdir) (const char *path);
static int     (*next_chmod) (const char *path, mode_t mode);
static int     (*next_chown) (const char *path, uid_t owner, gid_t group);
static int     (*next_chroot) (const char *path);
static int     (*next_creat) (const char *pathname, mode_t mode);
static int     (*next_creat64) (const char *pathname, mode_t mode);
#ifdef HAVE_DLMOPEN
static void *  (*next_dlmopen) (Lmid_t nsid, const char *filename, int flag);
#endif
static void *  (*next_dlopen) (const char *filename, int flag);
#ifdef HAVE_EUIDACCESS
static int     (*next_euidaccess) (const char *pathname, int mode);
#endif
static int     (*next_execl) (const char *path, const char *arg, ...);
static int     (*next_execle) (const char *path, const char *arg, ...);
static int     (*next_execlp) (const char *file, const char *arg, ...);
static int     (*next_execv) (const char *path, char *const argv []);
static int     (*next_execve) (const char *filename, char *const argv [], char *const envp[]);
static int     (*next_execvp) (const char *file, char *const argv []);
static FILE *  (*next_fopen) (const char *path, const char *mode);
static FILE *  (*next_fopen64) (const char *path, const char *mode);
static FILE *  (*next_freopen) (const char *path, const char *mode, FILE *stream);
static FILE *  (*next_freopen64) (const char *path, const char *mode, FILE *stream);
#ifdef HAVE_GET_CURRENT_DIR_NAME
static char *  (*next_get_current_dir_name) (void);
#endif
static char *  (*next_getcwd) (char *buf, size_t size);
static char *  (*next_getwd) (char *buf);
#ifdef HAVE_GETXATTR
static ssize_t (*next_getxattr) (const char *path, const char *name, void *value, size_t size);
#endif
static int     (*next_glob) (const char *pattern, int flags, int (*errfunc) (const char *, int), glob_t *pglob);
#ifdef HAVE_GLOB64
static int     (*next_glob64) (const char *pattern, int flags, int (*errfunc) (const char *, int), glob64_t *pglob);
#endif
#ifdef HAVE_GLOB_PATTERN_P
static int     (*next_glob_pattern_p) (const char *pattern, int quote);
#endif
#ifdef HAVE_LCHMOD
static int     (*next_lchmod) (const char *path, mode_t mode);
#endif
static int     (*next_lchown) (const char *path, uid_t owner, gid_t group);
static int     (*next_lckpwdf) (void);
#ifdef HAVE_LGETXATTR
static ssize_t (*next_lgetxattr) (const char *path, const char *name, void *value, size_t size);
#endif
static int     (*next_link) (const char *oldpath, const char *newpath);
#ifdef HAVE_LISTXATTR
static ssize_t (*next_listxattr) (const char *path, char *list, size_t size);
#endif
#ifdef HAVE_LLISTXATTR
static ssize_t (*next_llistxattr) (const char *path, char *list, size_t size);
#endif
#ifdef HAVE_LREMOVEXATTR
static int     (*next_lremovexattr) (const char *path, const char *name);
#endif
#ifdef HAVE_LSETXATTR
static int     (*next_lsetxattr) (const char *path, const char *name, const void *value, size_t size, int flags);
#endif
#ifndef __GLIBC__
static int     (*next_lstat) (const char *file_name, struct stat *buf);
static int     (*next_lstat64) (const char *file_name, struct stat64 *buf);
#endif
#ifdef HAVE_LUTIMES
static int     (*next_lutimes) (const char *filename, const struct timeval tv[2]);
#endif
static int     (*next_mkdir) (const char *pathname, mode_t mode);
#ifdef HAVE_MKDTEMP
static char *  (*next_mkdtemp) (char *template);
#endif
static int     (*next_mknod) (const char *pathname, mode_t mode, dev_t dev);
static int     (*next_mkfifo) (const char *pathname, mode_t mode);
static int     (*next_mkstemp) (char *template);
static int     (*next_mkstemp64) (char *template);
static char *  (*next_mktemp) (char *template);
static int     (*next_open) (const char *pathname, int flags, ...);
static int     (*next_open64) (const char *pathname, int flags, ...);
static DIR *   (*next_opendir) (const char *name);
static long    (*next_pathconf) (const char *path, int name);
static int     (*next_readlink) (const char *path, char *buf, size_t bufsiz);
static char *  (*next_realpath) (const char *name, char *resolved);
static int     (*next_remove) (const char *pathname);
#ifdef HAVE_REMOVEXATTR
static int     (*next_removexattr) (const char *path, const char *name);
#endif
static int     (*next_rename) (const char *oldpath, const char *newpath);
#ifdef HAVE_REVOKE
static int     (*next_revoke) (const char *file);
#endif
static int     (*next_rmdir) (const char *pathname);
#ifdef HAVE_SCANDIR
static int     (*next_scandir) (const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *));
#endif
#ifdef HAVE_SCANDIR64
static int     (*next_scandir64) (const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *));
#endif
#ifdef HAVE_SETXATTR
static int     (*next_setxattr) (const char *path, const char *name, const void *value, size_t size, int flags);
#endif
#ifndef __GLIBC__
static int     (*next_stat) (const char *file_name, struct stat *buf);
static int     (*next_stat64) (const char *file_name, struct stat64 *buf);
#endif
static int     (*next_symlink) (const char *oldpath, const char *newpath);
static char *  (*next_tempnam) (const char *dir, const char *pfx);
static char *  (*next_tmpnam) (char *s);
static int     (*next_truncate) (const char *path, off_t length);
static int     (*next_truncate64) (const char *path, off64_t length);
static int     (*next_unlink) (const char *pathname);
static int     (*next_ulckpwdf) (void);
static int     (*next_utime) (const char *filename, const struct utimbuf *buf);
static int     (*next_utimes) (const char *filename, const struct timeval tv[2]);


void fakechroot_init (void) __attribute((constructor));
void fakechroot_init (void)
{
#ifdef HAVE___LXSTAT
    *(void **)(&next___lxstat)                = dlsym(RTLD_NEXT, "__lxstat");
#endif
#ifdef HAVE___LXSTAT64
    *(void **)(&next___lxstat64)              = dlsym(RTLD_NEXT, "__lxstat64");
#endif
#ifdef HAVE___OPEN
    *(void **)(&next___open)                  = dlsym(RTLD_NEXT, "__open");
#endif
#ifdef HAVE___OPEN64
    *(void **)(&next___open64)                = dlsym(RTLD_NEXT, "__open64");
#endif
#ifdef HAVE___XMKNOD
    *(void **)(&next___xmknod)                = dlsym(RTLD_NEXT, "__xmknod");
#endif
#ifdef HAVE___XSTAT
    *(void **)(&next___xstat)                 = dlsym(RTLD_NEXT, "__xstat");
#endif
#ifdef HAVE___XSTAT64
    *(void **)(&next___xstat64)               = dlsym(RTLD_NEXT, "__xstat64");
#endif
    *(void **)(&next_access)                  = dlsym(RTLD_NEXT, "access");
    *(void **)(&next_acct)                    = dlsym(RTLD_NEXT, "acct");
#ifdef HAVE_CANONICALIZE_FILE_NAME
    *(void **)(&next_canonicalize_file_name)  = dlsym(RTLD_NEXT, "canonicalize_file_name");
#endif
    *(void **)(&next_chdir)                   = dlsym(RTLD_NEXT, "chdir");
    *(void **)(&next_chmod)                   = dlsym(RTLD_NEXT, "chmod");
    *(void **)(&next_chown)                   = dlsym(RTLD_NEXT, "chown");
    *(void **)(&next_chroot)                  = dlsym(RTLD_NEXT, "chroot");
    *(void **)(&next_creat)                   = dlsym(RTLD_NEXT, "creat");
    *(void **)(&next_creat64)                 = dlsym(RTLD_NEXT, "creat64");
#ifdef HAVE_DLMOPEN
    *(void **)(&next_dlmopen)                 = dlsym(RTLD_NEXT, "dlmopen");
#endif
    *(void **)(&next_dlopen)                  = dlsym(RTLD_NEXT, "dlopen");
#ifdef HAVE_EUIDACCESS
    *(void **)(&next_euidaccess)              = dlsym(RTLD_NEXT, "euidaccess");
#endif
    *(void **)(&next_execl)                   = dlsym(RTLD_NEXT, "execl");
    *(void **)(&next_execle)                  = dlsym(RTLD_NEXT, "execle");
    *(void **)(&next_execlp)                  = dlsym(RTLD_NEXT, "execlp");
    *(void **)(&next_execv)                   = dlsym(RTLD_NEXT, "execv");
    *(void **)(&next_execve)                  = dlsym(RTLD_NEXT, "execve");
    *(void **)(&next_execvp)                  = dlsym(RTLD_NEXT, "execvp");
    *(void **)(&next_fopen)                   = dlsym(RTLD_NEXT, "fopen");
    *(void **)(&next_fopen64)                 = dlsym(RTLD_NEXT, "fopen64");
    *(void **)(&next_freopen)                 = dlsym(RTLD_NEXT, "freopen");
    *(void **)(&next_freopen64)               = dlsym(RTLD_NEXT, "freopen64");
#ifdef HAVE_GET_CURRENT_DIR_NAME
    *(void **)(&next_get_current_dir_name)    = dlsym(RTLD_NEXT, "get_current_dir_name");
#endif
    *(void **)(&next_getcwd)                  = dlsym(RTLD_NEXT, "getcwd");
    *(void **)(&next_getwd)                   = dlsym(RTLD_NEXT, "getwd");
#ifdef HAVE_GETXATTR
    *(void **)(&next_getxattr)                = dlsym(RTLD_NEXT, "getxattr");
#endif
    *(void **)(&next_glob)                    = dlsym(RTLD_NEXT, "glob");
#ifdef HAVE_GLOB64
    *(void **)(&next_glob64)                  = dlsym(RTLD_NEXT, "glob64");
#endif
#ifdef HAVE_GLOB_PATTERN_P
    *(void **)(&next_glob_pattern_p)          = dlsym(RTLD_NEXT, "glob_pattern_p");
#endif
#ifdef HAVE_LCHMOD
    *(void **)(&next_lchmod)                  = dlsym(RTLD_NEXT, "lchmod");
#endif
    *(void **)(&next_lchown)                  = dlsym(RTLD_NEXT, "lchown");
    *(void **)(&next_lckpwdf)                 = dlsym(RTLD_NEXT, "lckpwdf");
#ifdef HAVE_LGETXATTR
    *(void **)(&next_lgetxattr)               = dlsym(RTLD_NEXT, "lgetxattr");
#endif
    *(void **)(&next_link)                    = dlsym(RTLD_NEXT, "link");
#ifdef HAVE_LISTXATTR
    *(void **)(&next_listxattr)               = dlsym(RTLD_NEXT, "listxattr");
#endif
#ifdef HAVE_LLISTXATTR
    *(void **)(&next_llistxattr)              = dlsym(RTLD_NEXT, "llistxattr");
#endif
#ifdef HAVE_LREMOVEXATTR
    *(void **)(&next_lremovexattr)            = dlsym(RTLD_NEXT, "lremovexattr");
#endif
#ifdef HAVE_LSETXATTR
    *(void **)(&next_lsetxattr)               = dlsym(RTLD_NEXT, "lsetxattr");
#endif
#ifndef __GLIBC__
    *(void **)(&next_lstat)                   = dlsym(RTLD_NEXT, "lstat");
    *(void **)(&next_lstat64)                 = dlsym(RTLD_NEXT, "lstat64");
#endif
#ifdef HAVE_LUTIMES
    *(void **)(&next_lutimes)                 = dlsym(RTLD_NEXT, "lutimes");
#endif
    *(void **)(&next_mkdir)                   = dlsym(RTLD_NEXT, "mkdir");
#ifdef HAVE_MKDTEMP
    *(void **)(&next_mkdtemp)                 = dlsym(RTLD_NEXT, "mkdtemp");
#endif
    *(void **)(&next_mknod)                   = dlsym(RTLD_NEXT, "mknod");
    *(void **)(&next_mkfifo)                  = dlsym(RTLD_NEXT, "mkfifo");
    *(void **)(&next_mkstemp)                 = dlsym(RTLD_NEXT, "mkstemp");
    *(void **)(&next_mkstemp64)               = dlsym(RTLD_NEXT, "mkstemp64");
    *(void **)(&next_mktemp)                  = dlsym(RTLD_NEXT, "mktemp");
    *(void **)(&next_open)                    = dlsym(RTLD_NEXT, "open");
    *(void **)(&next_open64)                  = dlsym(RTLD_NEXT, "open64");
    *(void **)(&next_opendir)                 = dlsym(RTLD_NEXT, "opendir");
    *(void **)(&next_pathconf)                = dlsym(RTLD_NEXT, "pathconf");
    *(void **)(&next_readlink)                = dlsym(RTLD_NEXT, "readlink");
    *(void **)(&next_realpath)                = dlsym(RTLD_NEXT, "realpath");
    *(void **)(&next_remove)                  = dlsym(RTLD_NEXT, "remove");
#ifdef HAVE_REMOVEXATTR
    *(void **)(&next_removexattr)             = dlsym(RTLD_NEXT, "removexattr");
#endif
    *(void **)(&next_rename)                  = dlsym(RTLD_NEXT, "rename");
#ifdef HAVE_REVOKE
    *(void **)(&next_revoke)                  = dlsym(RTLD_NEXT, "revoke");
#endif
    *(void **)(&next_rmdir)                   = dlsym(RTLD_NEXT, "rmdir");
#ifdef HAVE_SCANDIR
    *(void **)(&next_scandir)                 = dlsym(RTLD_NEXT, "scandir");
#endif
#ifdef HAVE_SCANDIR64
    *(void **)(&next_scandir64)               = dlsym(RTLD_NEXT, "scandir64");
#endif
#ifdef HAVE_SETXATTR
    *(void **)(&next_setxattr)                = dlsym(RTLD_NEXT, "setxattr");
#endif
#ifndef __GLIBC__
    *(void **)(&next_stat)                    = dlsym(RTLD_NEXT, "stat");
    *(void **)(&next_stat64)                  = dlsym(RTLD_NEXT, "stat64");
#endif
    *(void **)(&next_symlink)                 = dlsym(RTLD_NEXT, "symlink");
    *(void **)(&next_tempnam)                 = dlsym(RTLD_NEXT, "tempnam");
    *(void **)(&next_tmpnam)                  = dlsym(RTLD_NEXT, "tmpnam");
    *(void **)(&next_truncate)                = dlsym(RTLD_NEXT, "truncate");
    *(void **)(&next_truncate64)              = dlsym(RTLD_NEXT, "truncate64");
    *(void **)(&next_unlink)                  = dlsym(RTLD_NEXT, "unlink");
    *(void **)(&next_ulckpwdf)                = dlsym(RTLD_NEXT, "ulckpwdf");
    *(void **)(&next_utime)                   = dlsym(RTLD_NEXT, "utime");
    *(void **)(&next_utimes)                  = dlsym(RTLD_NEXT, "utimes");
}


#ifdef HAVE___LXSTAT
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___lxstat(ver, filename, buf);
}
#endif


#ifdef HAVE___LXSTAT64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat64 (int ver, const char *filename, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___lxstat64(ver, filename, buf);
}
#endif


#ifdef HAVE___OPEN
/* Internal libc function */
int __open (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next___open(pathname, flags, mode);
}
#endif


#ifdef HAVE___OPEN64
/* Internal libc function */
int __open64 (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next___open64(pathname, flags, mode);
}
#endif


#ifdef HAVE___XMKNOD
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xmknod (int ver, const char *path, mode_t mode, dev_t *dev)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___xmknod(ver, path, mode, dev);
}
#endif


#ifdef HAVE___XSTAT
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___xstat(ver, filename, buf);
}
#endif


#ifdef HAVE___XSTAT64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xstat64 (int ver, const char *filename, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___xstat64(ver, filename, buf);
}
#endif


/* #include <unistd.h> */
int access (const char *pathname, int mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_access(pathname, mode);
}


/* #include <unistd.h> */
int acct (const char *filename)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_acct(filename);
}


#ifdef HAVE_CANONICALIZE_FILE_NAME
/* #include <stdlib.h> */
char *canonicalize_file_name (const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_canonicalize_file_name(name);
}
#endif


/* #include <unistd.h> */
int chdir (const char *path)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_chdir(path);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int chmod (const char *path, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_chmod(path, mode);
}


/* #include <sys/types.h> */
/* #include <unistd.h> */
int chown (const char *path, uid_t owner, gid_t group)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_chown(path, owner, group);
}


/* #include <unistd.h> */
int chroot (const char *path)
{
    char *ptr, *ld_library_path, *tmp, *fakechroot_path;
    int status, len;
#ifndef HAVE_SETENV
    char *envbuf;
#endif

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        return EFAULT;
    }

    if ((status = chdir(path)) != 0) {
        return status;
    }

    ptr = rindex(path, 0);
    if (ptr > path) {
        ptr--;
        while (*ptr == '/') {
            *ptr-- = 0;
        }
    }

#ifdef HAVE_SETENV
    setenv("FAKECHROOT_BASE", path, 1);
#else
    envbuf = malloc(FAKECHROOT_MAXPATH+16);
    snprintf(envbuf, FAKECHROOT_MAXPATH+16, "FAKECHROOT_BASE=%s", path);
    putenv(envbuf);
#endif
    fakechroot_path = getenv("FAKECHROOT_BASE");

    ld_library_path = getenv("LD_LIBRARY_PATH");
    if (ld_library_path == NULL) {
	ld_library_path = "";
    }

    if ((len = strlen(ld_library_path)+strlen(path)*2+sizeof(":/usr/lib:/lib")) > FAKECHROOT_MAXPATH) {
        return ENAMETOOLONG;
    }

    if ((tmp = malloc(len)) == NULL) {
        return ENOMEM;
    }

    snprintf(tmp, len, "%s:%s/usr/lib:%s/lib", ld_library_path, path, path);
#ifdef HAVE_SETENV
    setenv("LD_LIBRARY_PATH", tmp, 1);
#else
    envbuf = malloc(FAKECHROOT_MAXPATH+16);
    snprintf(envbuf, FAKECHROOT_MAXPATH+16, "LD_LIBRARY_PATH=%s", tmp);
    putenv(envbuf);
#endif
    free(tmp);
    return 0;
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int creat (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_creat(pathname, mode);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int creat64 (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_creat64(pathname, mode);
}


#ifdef HAVE_DLMOPEN
/* #include <dlfcn.h> */
void *dlmopen (Lmid_t nsid, const char *filename, int flag)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_dlmopen(nsid, filename, flag);
}
#endif


/* #include <dlfcn.h> */
void *dlopen (const char *filename, int flag)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_dlopen(filename, flag);
}


#ifdef HAVE_EUIDACCESS
/* #include <unistd.h> */
int euidaccess (const char *pathname, int mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_euidaccess(pathname, mode);
}
#endif


/* #include <unistd.h> */
int execl (const char *path, const char *arg, ...)
{
  size_t argv_max = 1024;
  const char **argv = alloca (argv_max * sizeof (const char *));
  unsigned int i;
  va_list args;

  argv[0] = arg;

  va_start (args, arg);
  i = 0;
  while (argv[i++] != NULL)
    {
      if (i == argv_max)
        {
          const char **nptr = alloca ((argv_max *= 2) * sizeof (const char *));

            if ((char *) argv + i == (char *) nptr)
            /* Stack grows up.  */
            argv_max += i;
          else
            /* We have a hole in the stack.  */
            argv = (const char **) memcpy (nptr, argv,
                                           i * sizeof (const char *));
        }

      argv[i] = va_arg (args, const char *);
    }
  va_end (args);

  return execve (path, (char *const *) argv, environ);
}


/* #include <unistd.h> */
int execle (const char *path, const char *arg, ...)
{
  size_t argv_max = 1024;
  const char **argv = alloca (argv_max * sizeof (const char *));
  const char *const *envp;
  unsigned int i;
  va_list args;
  argv[0] = arg;

  va_start (args, arg);
  i = 0;
  while (argv[i++] != NULL)
    {
      if (i == argv_max)
        {
          const char **nptr = alloca ((argv_max *= 2) * sizeof (const char *));

            if ((char *) argv + i == (char *) nptr)
            /* Stack grows up.  */
            argv_max += i;
          else
            /* We have a hole in the stack.  */
            argv = (const char **) memcpy (nptr, argv,
                                           i * sizeof (const char *));
        }

      argv[i] = va_arg (args, const char *);
    }

  envp = va_arg (args, const char *const *);
  va_end (args);

  return execve (path, (char *const *) argv, (char *const *) envp);
}


/* #include <unistd.h> */
int execlp (const char *file, const char *arg, ...)
{
  size_t argv_max = 1024;
  const char **argv = alloca (argv_max * sizeof (const char *));
  unsigned int i;
  va_list args;
  char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

  argv[0] = arg;

  va_start (args, arg);
  i = 0;
  while (argv[i++] != NULL)
    {
      if (i == argv_max)
        {
          const char **nptr = alloca ((argv_max *= 2) * sizeof (const char *));

            if ((char *) argv + i == (char *) nptr)
            /* Stack grows up.  */
            argv_max += i;
          else
            /* We have a hole in the stack.  */
            argv = (const char **) memcpy (nptr, argv,
                                           i * sizeof (const char *));
        }

      argv[i] = va_arg (args, const char *);
    }
  va_end (args);

  expand_chroot_path(file, fakechroot_path, fakechroot_ptr, fakechroot_buf);
  return next_execvp (file, (char *const *) argv);
}


/* #include <unistd.h> */
int execv (const char *path, char *const argv [])
{
    return execve (path, argv, environ);
}


/* #include <unistd.h> */
int execve (const char *filename, char *const argv [], char *const envp[])
{
    int file;
    char hashbang[FAKECHROOT_MAXPATH];
    size_t argv_max = 1024;
    const char **newargv = alloca (argv_max * sizeof (const char *));
    char tmp[FAKECHROOT_MAXPATH], newfilename[FAKECHROOT_MAXPATH], argv0[FAKECHROOT_MAXPATH];
    char *ptr;
    unsigned int i, j, n;
    char c;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    strcpy(tmp, filename);
    filename = tmp;

    if ((file = open(filename, O_RDONLY)) == -1) {
        errno = ENOENT;
        return -1;
    }

    i = read(file, hashbang, FAKECHROOT_MAXPATH-2);
    close(file);
    if (i == -1) {
        errno = ENOENT;
        return -1;
    }

    if (hashbang[0] != '#' || hashbang[1] != '!')
        return next_execve(filename, argv, envp);

    hashbang[i] = hashbang[i+1] = 0;
    for (i = j = 2; (hashbang[i] == ' ' || hashbang[i] == '\t') && i < FAKECHROOT_MAXPATH; i++, j++);
    for (n = 0; i < FAKECHROOT_MAXPATH; i++) {
        c = hashbang[i];
        if (hashbang[i] == 0 || hashbang[i] == ' ' || hashbang[i] == '\t' || hashbang[i] == '\n') {
            hashbang[i] = 0;
            if (i > j) {
                if (n == 0) {
                    ptr = &hashbang[j];
                    expand_chroot_path(ptr, fakechroot_path, fakechroot_ptr, fakechroot_buf);
                    strcpy(newfilename, ptr);
                    strcpy(argv0, &hashbang[j]);
                    newargv[n++] = argv0;
                } else {
                    newargv[n++] = &hashbang[j];
                }
            }
            j = i + 1;
        }
        if (c == '\n' || c == 0)
            break;
    }

    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    newargv[n++] = filename;

    for (i = 1; argv[i] != NULL && i<argv_max; ) {
        newargv[n++] = argv[i++];
    }

    newargv[n] = 0;

    return next_execve(newfilename, (char *const *)newargv, envp);
}


/* #include <unistd.h> */
int execvp (const char *file, char *const argv [])
{
  if (*file == '\0')
    {
      /* We check the simple case first. */
      errno = ENOENT;
      return -1;
    }

  if (strchr (file, '/') != NULL)
    {
      /* Don't search when it contains a slash.  */
      return execve (file, argv, environ);
    }
  else
    {
      int got_eacces = 0;
      char *path, *p, *name;
      size_t len;
      size_t pathlen;

      path = getenv ("PATH");
      if (path == NULL)
        {
          /* There is no `PATH' in the environment.
             The default search path is the current directory
             followed by the path `confstr' returns for `_CS_PATH'.  */
          len = confstr (_CS_PATH, (char *) NULL, 0);
          path = (char *) alloca (1 + len);
          path[0] = ':';
          (void) confstr (_CS_PATH, path + 1, len);
        }

      len = strlen (file) + 1;
      pathlen = strlen (path);
      name = alloca (pathlen + len + 1);
      /* Copy the file name at the top.  */
      name = (char *) memcpy (name + pathlen + 1, file, len);
      /* And add the slash.  */
      *--name = '/';

      p = path;
      do
        {
          char *startp;

          path = p;
          p = strchrnul (path, ':');

          if (p == path)
            /* Two adjacent colons, or a colon at the beginning or the end
               of `PATH' means to search the current directory.  */
            startp = name + 1;
          else
            startp = (char *) memcpy (name - (p - path), path, p - path);

          /* Try to execute this name.  If it works, execv will not return.  */
          execve (startp, argv, environ);

          switch (errno)
            {
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
        }
      while (*p++ != '\0');

      /* We tried every element and none of them worked.  */
      if (got_eacces)
        /* At least one failure was due to permissions, so report that
           error.  */
        errno = EACCES;
    }

  /* Return the error from the last attempt (probably ENOENT).  */
  return -1;
}


/* #include <stdio.h> */
FILE *fopen (const char *path, const char *mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_fopen(path, mode);
}


/* #include <stdio.h> */
FILE *fopen64 (const char *path, const char *mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_fopen64(path, mode);
}


/* #include <stdio.h> */
FILE *freopen (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_freopen(path, mode, stream);
}


/* #include <stdio.h> */
FILE *freopen64 (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_freopen64(path, mode, stream);
}


#ifdef HAVE_GET_CURRENT_DIR_NAME
/* #include <unistd.h> */
char * get_current_dir_name (void) {
    char *cwd, *oldptr, *newptr;
    char *fakechroot_path, *fakechroot_ptr;

    if ((cwd = next_get_current_dir_name()) == NULL) {
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

    if ((cwd = next_getcwd(buf, size)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    return cwd;
}


/* #include <unistd.h> */
char * getwd (char *buf)
{
    char *cwd;
    char *fakechroot_path, *fakechroot_ptr;

    if ((cwd = next_getwd(buf)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd, fakechroot_path, fakechroot_ptr);
    return cwd;
}


#ifdef HAVE_GETXATTR
/* #include <sys/xattr.h> */
ssize_t getxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_getxattr(path, name, value, size);
}
#endif


/* #include <glob.h> */
int glob (const char *pattern, int flags, int (*errfunc) (const char *, int), glob_t *pglob)
{
    int rc,i;
    char tmp[FAKECHROOT_MAXPATH], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    expand_chroot_path(pattern, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    rc = next_glob(pattern, flags, errfunc, pglob);
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
    char tmp[FAKECHROOT_MAXPATH], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    expand_chroot_path(pattern, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    rc = next_glob64(pattern, flags, errfunc, pglob);
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
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pattern, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_glob_pattern_p(pattern, quote);
}
#endif


#ifdef HAVE_LCHMOD
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int lchmod (const char *path, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lchmod(path, mode);
}
#endif


/* #include <sys/types.h> */
/* #include <unistd.h> */
int lchown (const char *path, uid_t owner, gid_t group)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lchown(path, owner, group);
}


/* #include <shadow.h> */
int lckpwdf (void)
{
    return 0;
}


#ifdef HAVE_LGETXATTR
/* #include <sys/xattr.h> */
ssize_t lgetxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lgetxattr(path, name, value, size);
}
#endif


/* #include <unistd.h> */
int link (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_MAXPATH];
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_link(oldpath, newpath);
}


#ifdef HAVE_LISTXATTR
/* #include <sys/xattr.h> */
ssize_t listxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_listxattr(path, list, size);
}
#endif


#ifdef HAVE_LLISTXATTR
/* #include <sys/xattr.h> */
ssize_t llistxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_llistxattr(path, list, size);
}
#endif


#ifdef HAVE_LREMOVEXATTR
/* #include <sys/xattr.h> */
int lremovexattr (const char *path, const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lremovexattr(path, name);
}
#endif


#ifdef HAVE_LSETXATTR
/* #include <sys/xattr.h> */
int lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lsetxattr(path, name, value, size, flags);
}
#endif


#ifndef __GLIBC__
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat (const char *file_name, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lstat(file_name, buf);
}
#endif


#ifndef __GLIBC__
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lstat64(file_name, buf);
}
#endif


#ifdef HAVE_LUTIMES
/* #include <sys/time.h> */
int lutimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lutimes(filename, tv);
}
#endif


/* #include <sys/stat.h> */
/* #include <sys/types.h> */
int mkdir (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_mkdir(pathname, mode);
}


#ifdef HAVE_MKDTEMP
/* #include <stdlib.h> */
char *mkdtemp (char *template)
{
    char tmp[FAKECHROOT_MAXPATH], *oldtemplate, *ptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    oldtemplate = template;

    expand_chroot_path(template, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_mkdtemp(template) == NULL) {
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
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_mkfifo(pathname, mode);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
/* #include <unistd.h> */
int mknod (const char *pathname, mode_t mode, dev_t dev)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_mknod(pathname, mode, dev);
}


/* #include <stdlib.h> */
int mkstemp (char *template)
{
    char tmp[FAKECHROOT_MAXPATH], *oldtemplate, *ptr;
    int fd;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    oldtemplate = template;

    expand_chroot_path(template, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if ((fd = next_mkstemp(template)) == -1) {
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


/* #include <stdlib.h> */
int mkstemp64 (char *template)
{
    char tmp[FAKECHROOT_MAXPATH], *oldtemplate, *ptr;
    int fd;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    oldtemplate = template;

    expand_chroot_path(template, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if ((fd = next_mkstemp64(template)) == -1) {
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


/* #include <stdlib.h> */
char *mktemp (char *template)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(template, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_mktemp(template);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int open (const char *pathname, int flags, ...) {
    int mode = 0;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next_open(pathname, flags, mode);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int open64 (const char *pathname, int flags, ...)
{
    int mode = 0;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next_open64(pathname, flags, mode);
}


/* #include <sys/types.h> */
/* #include <dirent.h> */
DIR *opendir (const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_opendir(name);
}


/* #include <unistd.h> */
long pathconf (const char *path, int name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_pathconf(path, name);
}


/* #include <unistd.h> */
int readlink (const char *path, char *buf, size_t bufsiz)
{
    int status;
    char tmp[FAKECHROOT_MAXPATH], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);

    if ((status = next_readlink(path, tmp, bufsiz)) == -1) {
        return status;
    }
    /* TODO: shouldn't end with \000 */
    tmp[status] = '\0';

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + strlen(fakechroot_path);
        }
        strcpy(buf, tmpptr);
    } else {
        strcpy(buf, tmp);
    }
    return strlen(buf);
}


/* #include <stdlib.h> */
char *realpath (const char *name, char *resolved)
{
    char *ptr;
    char *fakechroot_path, *fakechroot_ptr;
    if ((ptr = next_realpath(name, resolved)) != NULL) {
        narrow_chroot_path(ptr, fakechroot_path, fakechroot_ptr);
    }
    return ptr;
}


/* #include <stdio.h> */
int remove (const char *pathname)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_remove(pathname);
}


#ifdef HAVE_REMOVEXATTR
/* #include <sys/xattr.h> */
int removexattr (const char *path, const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_removexattr(path, name);
}
#endif


/* #include <stdio.h> */
int rename (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_MAXPATH];
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_rename(oldpath, newpath);
}


#ifdef HAVE_REVOKE
/* #include <unistd.h> */
int revoke (const char *file)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_rmdir(file);
}
#endif


/* #include <unistd.h> */
int rmdir (const char *pathname)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_rmdir(pathname);
}


#ifdef HAVE_SCANDIR
/* #include <dirent.h> */
int scandir (const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *))
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_scandir(dir, namelist, filter, compar);
}
#endif


#ifdef HAVE_SCANDIR64
/* #include <dirent.h> */
int scandir64 (const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *))
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_scandir64(dir, namelist, filter, compar);
}
#endif


#ifdef HAVE_SETXATTR
/* #include <sys/xattr.h> */
int setxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_setxattr(path, name, value, size, flags);
}
#endif


#ifndef __GLIBC__
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat (const char *file_name, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_stat(file_name, buf);
}
#endif


#ifndef __GLIBC__
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_stat64(file_name, buf);
}
#endif


/* #include <unistd.h> */
int symlink (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_MAXPATH];
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_symlink(oldpath, newpath);
}


/* #include <stdio.h> */
char *tempnam (const char *dir, const char *pfx)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_tempnam(dir, pfx);
}


/* #include <stdio.h> */
char *tmpnam (char *s)
{
    char *ptr;
    char *fakechroot_path, *fakechroot_ptr, *fakechroot_buf;

    if (s != NULL) {
        return next_tmpnam(s);
    }

    ptr = next_tmpnam(NULL);
    expand_chroot_path_malloc(ptr, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return ptr;
}


/* #include <unistd.h> */
/* #include <sys/types.h> */
int truncate (const char *path, off_t length)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_truncate(path, length);
}


/* #include <unistd.h> */
/* #include <sys/types.h> */
int truncate64 (const char *path, off64_t length)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_truncate64(path, length);
}


/* #include <shadow.h> */
int ulckpwdf (void)
{
    return 0;
}


/* #include <unistd.h> */
int unlink (const char *pathname)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_unlink(pathname);
}


/* #include <sys/types.h> */
/* #include <utime.h> */
int utime (const char *filename, const struct utimbuf *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_utime(filename, buf);
}


/* #include <sys/time.h> */
int utimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_utimes(filename, tv);
}
