/*
    libfakechroot -- fake chroot environment
    (c) 2003-2007 Piotr Roszatycki <dexter@debian.org>, LGPL

    klik2 support -- give direct access to a list of directories
    (c) 2006-2007 Lionel Tricon <lionel.tricon@free.fr>, LGPL

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


#include <config.h>

#define _GNU_SOURCE
#define __BSD_VISIBLE

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
#include <pwd.h>
#ifdef HAVE_FTS_H
#include <fts.h>
#endif
#ifdef HAVE_FTW_H
#include <ftw.h>
#endif
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#if defined(PATH_MAX)
#define FAKECHROOT_MAXPATH PATH_MAX
#elif defined(_POSIX_PATH_MAX)
#define FAKECHROOT_MAXPATH _POSIX_PATH_MAX
#elif defined(MAXPATHLEN)
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
    if (!fakechroot_localdir(path)) { \
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
    if (!fakechroot_localdir(path)) { \
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

#define nextsym(function, name) \
    { \
        char *msg; \
	if (next_##function == NULL) { \
	    *(void **)(&next_##function) = dlsym(RTLD_NEXT, name); \
	    if ((msg = dlerror()) != NULL) { \
 		fprintf (stderr, "%s: dlsym(%s): %s\n", PACKAGE, name, msg); \
	    } \
	} \
    }


#ifndef __GLIBC__
extern char **environ;
#endif

/* Useful to exclude a list of directories or files */
static char *exclude_list[32];
static int exclude_length[32];
static int list_max = 0;
static int first = 0;
static char *home_path=NULL;

#ifndef HAVE_STRCHRNUL
/* Find the first occurrence of C in S or the final NUL byte.  */
static char *strchrnul (const char *s, int c_in)
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
static int     (*next___lxstat) (int ver, const char *filename, struct stat *buf) = NULL;
#endif
#ifdef HAVE___LXSTAT64
static int     (*next___lxstat64) (int ver, const char *filename, struct stat64 *buf) = NULL;
#endif
#ifdef HAVE___OPEN
static int     (*next___open) (const char *pathname, int flags, ...) = NULL;
#endif
#ifdef HAVE___OPEN64
static int     (*next___open64) (const char *pathname, int flags, ...) = NULL;
#endif
#ifdef HAVE___OPENDIR2
static DIR *   (*next___opendir2) (const char *name, int flags) = NULL;
#endif
#ifdef HAVE___XMKNOD
static int     (*next___xmknod) (int ver, const char *path, mode_t mode, dev_t *dev) = NULL;
#endif
#ifdef HAVE___XSTAT
static int     (*next___xstat) (int ver, const char *filename, struct stat *buf) = NULL;
#endif
#ifdef HAVE___XSTAT64
static int     (*next___xstat64) (int ver, const char *filename, struct stat64 *buf) = NULL;
#endif
#ifdef HAVE__XFTW
static int     (*next__xftw) (int mode, const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd) = NULL;
#endif
#ifdef HAVE__XFTW64
static int     (*next__xftw64) (int mode, const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag), int nopenfd) = NULL;
#endif
static int     (*next_access) (const char *pathname, int mode) = NULL;
static int     (*next_acct) (const char *filename) = NULL;
#ifdef HAVE_CANONICALIZE_FILE_NAME
static char *  (*next_canonicalize_file_name) (const char *name) = NULL;
#endif
static int     (*next_chdir) (const char *path) = NULL;
static int     (*next_chmod) (const char *path, mode_t mode) = NULL;
static int     (*next_chown) (const char *path, uid_t owner, gid_t group) = NULL;
/* static int     (*next_chroot) (const char *path) = NULL; */
static int     (*next_creat) (const char *pathname, mode_t mode) = NULL;
static int     (*next_creat64) (const char *pathname, mode_t mode) = NULL;
#ifdef HAVE_DLMOPEN
static void *  (*next_dlmopen) (Lmid_t nsid, const char *filename, int flag) = NULL;
#endif
static void *  (*next_dlopen) (const char *filename, int flag) = NULL;
#ifdef HAVE_EUIDACCESS
static int     (*next_euidaccess) (const char *pathname, int mode) = NULL;
#endif
/* static int     (*next_execl) (const char *path, const char *arg, ...) = NULL; */
/* static int     (*next_execle) (const char *path, const char *arg, ...) = NULL; */
/* static int     (*next_execlp) (const char *file, const char *arg, ...) = NULL; */
/* static int     (*next_execv) (const char *path, char *const argv []) = NULL; */
static int     (*next_execve) (const char *filename, char *const argv [], char *const envp[]) = NULL;
static int     (*next_execvp) (const char *file, char *const argv []) = NULL;
static FILE *  (*next_fopen) (const char *path, const char *mode) = NULL;
static FILE *  (*next_fopen64) (const char *path, const char *mode) = NULL;
static FILE *  (*next_freopen) (const char *path, const char *mode, FILE *stream) = NULL;
static FILE *  (*next_freopen64) (const char *path, const char *mode, FILE *stream) = NULL;
#ifdef HAVE_FTS_OPEN
#if !defined(HAVE___OPENDIR2)
static FTS *   (*next_fts_open) (char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **)) = NULL;
#endif
#endif
#ifdef HAVE_FTW
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
static int     (*next_ftw) (const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd) = NULL;
#endif
#endif
#ifdef HAVE_FTW64
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
static int     (*next_ftw64) (const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag), int nopenfd) = NULL;
#endif
#endif
#ifdef HAVE_GET_CURRENT_DIR_NAME
static char *  (*next_get_current_dir_name) (void) = NULL;
#endif
static char *  (*next_getcwd) (char *buf, size_t size) = NULL;
static char *  (*next_getwd) (char *buf) = NULL;
#ifdef HAVE_GETXATTR
static ssize_t (*next_getxattr) (const char *path, const char *name, void *value, size_t size) = NULL;
#endif
static int     (*next_glob) (const char *pattern, int flags, int (*errfunc) (const char *, int), glob_t *pglob) = NULL;
#ifdef HAVE_GLOB64
static int     (*next_glob64) (const char *pattern, int flags, int (*errfunc) (const char *, int), glob64_t *pglob) = NULL;
#endif
#ifdef HAVE_GLOB_PATTERN_P
static int     (*next_glob_pattern_p) (const char *pattern, int quote) = NULL;
#endif
#ifdef HAVE_LCHMOD
static int     (*next_lchmod) (const char *path, mode_t mode) = NULL;
#endif
static int     (*next_lchown) (const char *path, uid_t owner, gid_t group) = NULL;
#ifdef HAVE_LCKPWDF
/* static int     (*next_lckpwdf) (void) = NULL; */
#endif
#ifdef HAVE_LGETXATTR
static ssize_t (*next_lgetxattr) (const char *path, const char *name, void *value, size_t size) = NULL;
#endif
static int     (*next_link) (const char *oldpath, const char *newpath) = NULL;
#ifdef HAVE_LISTXATTR
static ssize_t (*next_listxattr) (const char *path, char *list, size_t size) = NULL;
#endif
#ifdef HAVE_LLISTXATTR
static ssize_t (*next_llistxattr) (const char *path, char *list, size_t size) = NULL;
#endif
#ifdef HAVE_LREMOVEXATTR
static int     (*next_lremovexattr) (const char *path, const char *name) = NULL;
#endif
#ifdef HAVE_LSETXATTR
static int     (*next_lsetxattr) (const char *path, const char *name, const void *value, size_t size, int flags) = NULL;
#endif
#if !defined(HAVE___LXSTAT)
static int     (*next_lstat) (const char *file_name, struct stat *buf) = NULL;
#endif
#ifdef HAVE_LSTAT64
#if !defined(HAVE___LXSTAT64)
static int     (*next_lstat64) (const char *file_name, struct stat64 *buf) = NULL;
#endif
#endif
#ifdef HAVE_LUTIMES
static int     (*next_lutimes) (const char *filename, const struct timeval tv[2]) = NULL;
#endif
static int     (*next_mkdir) (const char *pathname, mode_t mode) = NULL;
#ifdef HAVE_MKDTEMP
static char *  (*next_mkdtemp) (char *template) = NULL;
#endif
static int     (*next_mknod) (const char *pathname, mode_t mode, dev_t dev) = NULL;
static int     (*next_mkfifo) (const char *pathname, mode_t mode) = NULL;
static int     (*next_mkstemp) (char *template) = NULL;
static int     (*next_mkstemp64) (char *template) = NULL;
static char *  (*next_mktemp) (char *template) = NULL;
#ifdef HAVE_NFTW
static int     (*next_nftw) (const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag, struct FTW *s), int nopenfd, int flags) = NULL;
#endif
#ifdef HAVE_NFTW64
static int     (*next_nftw64) (const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag, struct FTW *s), int nopenfd, int flags) = NULL;
#endif
static int     (*next_open) (const char *pathname, int flags, ...) = NULL;
static int     (*next_open64) (const char *pathname, int flags, ...) = NULL;
#if !defined(HAVE___OPENDIR2)
static DIR *   (*next_opendir) (const char *name) = NULL;
#endif
static long    (*next_pathconf) (const char *path, int name) = NULL;
static int     (*next_readlink) (const char *path, char *buf, READLINK_TYPE_ARG3) = NULL;
static char *  (*next_realpath) (const char *name, char *resolved) = NULL;
static int     (*next_remove) (const char *pathname) = NULL;
#ifdef HAVE_REMOVEXATTR
static int     (*next_removexattr) (const char *path, const char *name) = NULL;
#endif
static int     (*next_rename) (const char *oldpath, const char *newpath) = NULL;
#ifdef HAVE_REVOKE
static int     (*next_revoke) (const char *file) = NULL;
#endif
static int     (*next_rmdir) (const char *pathname) = NULL;
#ifdef HAVE_SCANDIR
static int     (*next_scandir) (const char *dir, struct dirent ***namelist, SCANDIR_TYPE_ARG3, int(*compar)(const void *, const void *)) = NULL;
#endif
#ifdef HAVE_SCANDIR64
static int     (*next_scandir64) (const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *)) = NULL;
#endif
#ifdef HAVE_SETXATTR
static int     (*next_setxattr) (const char *path, const char *name, const void *value, size_t size, int flags) = NULL;
#endif
#if !defined(HAVE___XSTAT)
static int     (*next_stat) (const char *file_name, struct stat *buf) = NULL;
#endif
#ifdef HAVE_STAT64
#if !defined(HAVE___XSTAT64)
static int     (*next_stat64) (const char *file_name, struct stat64 *buf) = NULL;
#endif
#endif
static int     (*next_symlink) (const char *oldpath, const char *newpath) = NULL;
static char *  (*next_tempnam) (const char *dir, const char *pfx) = NULL;
static char *  (*next_tmpnam) (char *s) = NULL;
static int     (*next_truncate) (const char *path, off_t length) = NULL;
#ifdef HAVE_TRUNCATE64
static int     (*next_truncate64) (const char *path, off64_t length) = NULL;
#endif
static int     (*next_unlink) (const char *pathname) = NULL;
#ifdef HAVE_ULCKPWDF
/* static int     (*next_ulckpwdf) (void) = NULL; */
#endif
static int     (*next_utime) (const char *filename, const struct utimbuf *buf) = NULL;
static int     (*next_utimes) (const char *filename, const struct timeval tv[2]) = NULL;


/* Bootstrap the library */
void fakechroot_init (void) __attribute((constructor));
void fakechroot_init (void)
{
    int i,j;
    struct passwd* passwd = NULL;
    char *pointer;

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
            home_path = malloc(strlen(passwd->pw_dir)+1);
            strcpy(home_path, passwd->pw_dir);
            strcat(home_path, "/");
        }
    }

#ifdef HAVE___LXSTAT
    nextsym(__lxstat, "__lxstat");
#endif
#ifdef HAVE___LXSTAT64
    nextsym(__lxstat64, "__lxstat64");
#endif
#ifdef HAVE___OPEN
    nextsym(__open, "__open");
#endif
#ifdef HAVE___OPEN64
    nextsym(__open64, "__open64");
#endif
#ifdef HAVE___OPENDIR2
    nextsym(__opendir2, "__opendir2");
#endif
#ifdef HAVE___XMKNOD
    nextsym(__xmknod, "__xmknod");
#endif
#ifdef HAVE___XSTAT
    nextsym(__xstat, "__xstat");
#endif
#ifdef HAVE___XSTAT64
    nextsym(__xstat64, "__xstat64");
#endif
    nextsym(access, "access");
    nextsym(acct, "acct");
#ifdef HAVE_CANONICALIZE_FILE_NAME
    nextsym(canonicalize_file_name, "canonicalize_file_name");
#endif
    nextsym(chdir, "chdir");
    nextsym(chmod, "chmod");
    nextsym(chown, "chown");
/*    nextsym(chroot, "chroot"); */
    nextsym(creat, "creat");
    nextsym(creat64, "creat64");
#ifdef HAVE_DLMOPEN
    nextsym(dlmopen, "dlmopen");
#endif
    nextsym(dlopen, "dlopen");
#ifdef HAVE_EUIDACCESS
    nextsym(euidaccess, "euidaccess");
#endif
/*    nextsym(execl, "execl"); */
/*    nextsym(execle, "execle"); */
/*    nextsym(execlp, "execlp"); */
/*    nextsym(execv, "execv"); */
    nextsym(execve, "execve");
    nextsym(execvp, "execvp");
    nextsym(fopen, "fopen");
    nextsym(fopen64, "fopen64");
    nextsym(freopen, "freopen");
    nextsym(freopen64, "freopen64");
#ifdef HAVE_FTS_OPEN
#if !defined(HAVE___OPENDIR2)
    nextsym(fts_open, "fts_open");
#endif
#endif
#ifdef HAVE_FTW
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
    nextsym(ftw, "ftw");
#endif
#endif
#ifdef HAVE_FTW64
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
    nextsym(ftw64, "ftw64");
#endif
#endif
#ifdef HAVE_GET_CURRENT_DIR_NAME
    nextsym(get_current_dir_name, "get_current_dir_name");
#endif
    nextsym(getcwd, "getcwd");
    nextsym(getwd, "getwd");
#ifdef HAVE_GETXATTR
    nextsym(getxattr, "getxattr");
#endif
    nextsym(glob, "glob");
#ifdef HAVE_GLOB64
    nextsym(glob64, "glob64");
#endif
#ifdef HAVE_GLOB_PATTERN_P
    nextsym(glob_pattern_p, "glob_pattern_p");
#endif
#ifdef HAVE_LCHMOD
    nextsym(lchmod, "lchmod");
#endif
    nextsym(lchown, "lchown");
#ifdef HAVE_LCKPWDF
/*    nextsym(lckpwdf, "lckpwdf"); */
#endif
#ifdef HAVE_LGETXATTR
    nextsym(lgetxattr, "lgetxattr");
#endif
    nextsym(link, "link");
#ifdef HAVE_LISTXATTR
    nextsym(listxattr, "listxattr");
#endif
#ifdef HAVE_LLISTXATTR
    nextsym(llistxattr, "llistxattr");
#endif
#ifdef HAVE_LREMOVEXATTR
    nextsym(lremovexattr, "lremovexattr");
#endif
#ifdef HAVE_LSETXATTR
    nextsym(lsetxattr, "lsetxattr");
#endif
#if !defined(HAVE___LXSTAT)
    nextsym(lstat, "lstat");
#endif
#ifdef HAVE_LSTAT64
#if !defined(HAVE___LXSTAT64)
    nextsym(lstat64, "lstat64");
#endif
#endif
#ifdef HAVE_LUTIMES
    nextsym(lutimes, "lutimes");
#endif
    nextsym(mkdir, "mkdir");
#ifdef HAVE_MKDTEMP
    nextsym(mkdtemp, "mkdtemp");
#endif
    nextsym(mknod, "mknod");
    nextsym(mkfifo, "mkfifo");
    nextsym(mkstemp, "mkstemp");
    nextsym(mkstemp64, "mkstemp64");
    nextsym(mktemp, "mktemp");
#ifdef HAVE_NFTW
    nextsym(nftw, "nftw");
#endif
#ifdef HAVE_NFTW64
    nextsym(nftw64, "nftw64");
#endif
    nextsym(open, "open");
    nextsym(open64, "open64");
#if !defined(HAVE___OPENDIR2)
    nextsym(opendir, "opendir");
#endif
    nextsym(pathconf, "pathconf");
    nextsym(readlink, "readlink");
    nextsym(realpath, "realpath");
    nextsym(remove, "remove");
#ifdef HAVE_REMOVEXATTR
    nextsym(removexattr, "removexattr");
#endif
    nextsym(rename, "rename");
#ifdef HAVE_REVOKE
    nextsym(revoke, "revoke");
#endif
    nextsym(rmdir, "rmdir");
#ifdef HAVE_SCANDIR
    nextsym(scandir, "scandir");
#endif
#ifdef HAVE_SCANDIR64
    nextsym(scandir64, "scandir64");
#endif
#ifdef HAVE_SETXATTR
    nextsym(setxattr, "setxattr");
#endif
#if !defined(HAVE___XSTAT)
    nextsym(stat, "stat");
#endif
#ifdef HAVE_STAT64
#if !defined(HAVE___XSTAT64)
    nextsym(stat64, "stat64");
#endif
#endif
    nextsym(symlink, "symlink");
    nextsym(tempnam, "tempnam");
    nextsym(tmpnam, "tmpnam");
    nextsym(truncate, "truncate");
#ifdef HAVE_TRUNCATE64
    nextsym(truncate64, "truncate64");
#endif
    nextsym(unlink, "unlink");
#ifdef HAVE_ULCKPWDF
/*    nextsym(ulckpwdf, "ulckpwdf"); */
#endif
    nextsym(utime, "utime");
    nextsym(utimes, "utimes");
}


/* Check if path is on exclude list */
static int fakechroot_localdir (const char *p_path)
{
    char *v_path = (char*)p_path;
    char *fakechroot_path, *fakechroot_ptr;
    char cwd_path[FAKECHROOT_MAXPATH];
    int i, len;

    if (!p_path) return 0;

    /* We need to expand ~ paths */
    if (home_path!=NULL && p_path[0]=='~') {
	strcpy(cwd_path, home_path);
	strcat(cwd_path, &(p_path[1]));
	v_path = cwd_path;
    }

    /* We need to expand relative paths */
    if (p_path[0] != '/') {
	if (next_getcwd == NULL) fakechroot_init();
	next_getcwd(cwd_path, FAKECHROOT_MAXPATH);
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


#ifdef HAVE___LXSTAT
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next___lxstat == NULL) fakechroot_init();
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
    if (next___lxstat64 == NULL) fakechroot_init();
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

    if (next___open == NULL) fakechroot_init();
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

    if (next___open64 == NULL) fakechroot_init();
    return next___open64(pathname, flags, mode);
}
#endif


#ifdef HAVE___OPENDIR2
/* Internal libc function */
/* #include <dirent.h> */
DIR *__opendir2 (const char *name, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next___opendir2 == NULL) fakechroot_init();
    return next___opendir2(name, flags);
}
#endif


#ifdef HAVE___XMKNOD
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xmknod (int ver, const char *path, mode_t mode, dev_t *dev)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next___xmknod == NULL) fakechroot_init();
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
    if (next___xstat == NULL) fakechroot_init();
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
    if (next___xstat64 == NULL) fakechroot_init();
    return next___xstat64(ver, filename, buf);
}
#endif


#ifdef HAVE__XFTW
/* include <ftw.h> */
int _xftw (int mode, const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next__xftw == NULL) fakechroot_init();
    return next__xftw(mode, dir, fn, nopenfd);
}
#endif


#ifdef HAVE__XFTW64
/* include <ftw.h> */
int _xftw64 (int mode, const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag), int nopenfd)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next__xftw64 == NULL) fakechroot_init();
    return next__xftw64(mode, dir, fn, nopenfd);
}
#endif


/* #include <unistd.h> */
int access (const char *pathname, int mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_access == NULL) fakechroot_init();
    return next_access(pathname, mode);
}


/* #include <unistd.h> */
int acct (const char *filename)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_acct == NULL) fakechroot_init();
    return next_acct(filename);
}


#ifdef HAVE_CANONICALIZE_FILE_NAME
/* #include <stdlib.h> */
char *canonicalize_file_name (const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_canonicalize_file_name == NULL) fakechroot_init();
    return next_canonicalize_file_name(name);
}
#endif


/* #include <unistd.h> */
int chdir (const char *path)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_chdir == NULL) fakechroot_init();
    return next_chdir(path);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int chmod (const char *path, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_chmod == NULL) fakechroot_init();
    return next_chmod(path, mode);
}


/* #include <sys/types.h> */
/* #include <unistd.h> */
int chown (const char *path, uid_t owner, gid_t group)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_chown == NULL) fakechroot_init();
    return next_chown(path, owner, group);
}


/* #include <unistd.h> */
int chroot (const char *path)
{
    char *ptr, *ld_library_path, *tmp, *fakechroot_path;
    int status, len;
    char dir[FAKECHROOT_MAXPATH];
#if !defined(HAVE_SETENV)
    char *envbuf;
#endif

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        return EFAULT;
    }

    if ((status = chdir(path)) != 0) {
        return status;
    }
    
    if (getcwd(dir, FAKECHROOT_MAXPATH) == NULL) {
	return EFAULT;
    }

    ptr = rindex(dir, 0);
    if (ptr > dir) {
        ptr--;
        while (*ptr == '/') {
            *ptr-- = 0;
        }
    }

#if defined(HAVE_SETENV)
    setenv("FAKECHROOT_BASE", dir, 1);
#else
    envbuf = malloc(FAKECHROOT_MAXPATH+16);
    snprintf(envbuf, FAKECHROOT_MAXPATH+16, "FAKECHROOT_BASE=%s", dir);
    putenv(envbuf);
#endif
    fakechroot_path = getenv("FAKECHROOT_BASE");

    ld_library_path = getenv("LD_LIBRARY_PATH");
    if (ld_library_path == NULL) {
	ld_library_path = "";
    }

    if ((len = strlen(ld_library_path)+strlen(dir)*2+sizeof(":/usr/lib:/lib")) > FAKECHROOT_MAXPATH) {
        return ENAMETOOLONG;
    }

    if ((tmp = malloc(len)) == NULL) {
        return ENOMEM;
    }

    snprintf(tmp, len, "%s:%s/usr/lib:%s/lib", ld_library_path, dir, dir);
#if defined(HAVE_SETENV)
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
    if (next_creat == NULL) fakechroot_init();
    return next_creat(pathname, mode);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int creat64 (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_creat64 == NULL) fakechroot_init();
    return next_creat64(pathname, mode);
}


#ifdef HAVE_DLMOPEN
/* #include <dlfcn.h> */
void *dlmopen (Lmid_t nsid, const char *filename, int flag)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_dlmopen == NULL) fakechroot_init();
    return next_dlmopen(nsid, filename, flag);
}
#endif


/* #include <dlfcn.h> */
void *dlopen (const char *filename, int flag)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_dlopen == NULL) fakechroot_init();
    return next_dlopen(filename, flag);
}


#ifdef HAVE_EUIDACCESS
/* #include <unistd.h> */
int euidaccess (const char *pathname, int mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_euidaccess == NULL) fakechroot_init();
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
  if (next_execvp == NULL) fakechroot_init();
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

    if (next_execve == NULL) fakechroot_init();

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
    if (next_fopen == NULL) fakechroot_init();
    return next_fopen(path, mode);
}


/* #include <stdio.h> */
FILE *fopen64 (const char *path, const char *mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_fopen64 == NULL) fakechroot_init();
    return next_fopen64(path, mode);
}


/* #include <stdio.h> */
FILE *freopen (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_freopen == NULL) fakechroot_init();
    return next_freopen(path, mode, stream);
}


/* #include <stdio.h> */
FILE *freopen64 (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_freopen64 == NULL) fakechroot_init();
    return next_freopen64(path, mode, stream);
}


#ifdef HAVE_FTS_OPEN
#if !defined(HAVE___OPENDIR2)
/* #include <fts.h> */
FTS * fts_open (char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **)) {
    char *fakechroot_path, *fakechroot_ptr, *fakechroot_buf;
    char *path;
    char * const *p;
    char **new_path_argv;
    char **np;
    int n;

    for (n=0, p=path_argv; *p; n++, p++);
    if ((new_path_argv = malloc(n*(sizeof(char *)))) == NULL) {
	return NULL;
    }

    for (n=0, p=path_argv, np=new_path_argv; *p; n++, p++, np++) {
	path = *p;
	expand_chroot_path_malloc(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
	*np = path;
    }
    
    if (next_fts_open == NULL) fakechroot_init();
    return next_fts_open(new_path_argv, options, compar);
}
#endif
#endif


#ifdef HAVE_FTW
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW)
/* include <ftw.h> */
int ftw (const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_ftw == NULL) fakechroot_init();
    return next_ftw(dir, fn, nopenfd);
}
#endif
#endif


#ifdef HAVE_FTW64
#if !defined(HAVE___OPENDIR2) && !defined(HAVE__XFTW64)
/* include <ftw.h> */
int ftw64 (const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag), int nopenfd)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_ftw64 == NULL) fakechroot_init();
    return next_ftw64(dir, fn, nopenfd);
}
#endif
#endif


#ifdef HAVE_GET_CURRENT_DIR_NAME
/* #include <unistd.h> */
char * get_current_dir_name (void) {
    char *cwd, *oldptr, *newptr;
    char *fakechroot_path, *fakechroot_ptr;

    if (next_get_current_dir_name == NULL) fakechroot_init();

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

    if (next_getcwd == NULL) fakechroot_init();

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

    if (next_getwd == NULL) fakechroot_init();

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
    if (next_getxattr == NULL) fakechroot_init();
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

    if (next_glob == NULL) fakechroot_init();

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

    if (next_glob64 == NULL) fakechroot_init();

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
    if (next_glob_pattern_p == NULL) fakechroot_init();
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
    if (next_lchmod == NULL) fakechroot_init();
    return next_lchmod(path, mode);
}
#endif


/* #include <sys/types.h> */
/* #include <unistd.h> */
int lchown (const char *path, uid_t owner, gid_t group)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lchown == NULL) fakechroot_init();
    return next_lchown(path, owner, group);
}


#ifdef HAVE_LCKPWDF
/* #include <shadow.h> */
int lckpwdf (void)
{
    return 0;
}
#endif


#ifdef HAVE_LGETXATTR
/* #include <sys/xattr.h> */
ssize_t lgetxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lgetxattr == NULL) fakechroot_init();
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
    if (next_link == NULL) fakechroot_init();
    return next_link(oldpath, newpath);
}


#ifdef HAVE_LISTXATTR
/* #include <sys/xattr.h> */
ssize_t listxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_listxattr == NULL) fakechroot_init();
    return next_listxattr(path, list, size);
}
#endif


#ifdef HAVE_LLISTXATTR
/* #include <sys/xattr.h> */
ssize_t llistxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_llistxattr == NULL) fakechroot_init();
    return next_llistxattr(path, list, size);
}
#endif


#ifdef HAVE_LREMOVEXATTR
/* #include <sys/xattr.h> */
int lremovexattr (const char *path, const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lremovexattr == NULL) fakechroot_init();
    return next_lremovexattr(path, name);
}
#endif


#ifdef HAVE_LSETXATTR
/* #include <sys/xattr.h> */
int lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lsetxattr == NULL) fakechroot_init();
    return next_lsetxattr(path, name, value, size, flags);
}
#endif


#if !defined(HAVE___LXSTAT)
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat (const char *file_name, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lstat == NULL) fakechroot_init();
    return next_lstat(file_name, buf);
}
#endif


#ifdef HAVE_LSTAT64
#if !defined(HAVE___LXSTAT64)
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lstat64 == NULL) fakechroot_init();
    return next_lstat64(file_name, buf);
}
#endif
#endif


#ifdef HAVE_LUTIMES
/* #include <sys/time.h> */
int lutimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_lutimes == NULL) fakechroot_init();
    return next_lutimes(filename, tv);
}
#endif


/* #include <sys/stat.h> */
/* #include <sys/types.h> */
int mkdir (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_mkdir == NULL) fakechroot_init();
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

    if (next_mkdtemp == NULL) fakechroot_init();

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
    if (next_mkfifo == NULL) fakechroot_init();
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
    if (next_mknod == NULL) fakechroot_init();
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

    if (next_mkstemp == NULL) fakechroot_init();

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

    if (next_mkstemp64 == NULL) fakechroot_init();

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
    if (next_mktemp == NULL) fakechroot_init();
    return next_mktemp(template);
}


#ifdef HAVE_NFTW
/* #include <ftw.h> */
int nftw (const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag, struct FTW *s), int nopenfd, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_nftw == NULL) fakechroot_init();
    return next_nftw(dir, fn, nopenfd, flags);
}
#endif


#ifdef HAVE_NFTW64
/* #include <ftw.h> */
int nftw64 (const char *dir, int (*fn)(const char *file, const struct stat64 *sb, int flag, struct FTW *s), int nopenfd, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_nftw64 == NULL) fakechroot_init();
    return next_nftw64(dir, fn, nopenfd, flags);
}
#endif


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

    if (next_open == NULL) fakechroot_init();
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

    if (next_open64 == NULL) fakechroot_init();
    return next_open64(pathname, flags, mode);
}


#if !defined(HAVE___OPENDIR2)
/* #include <sys/types.h> */
/* #include <dirent.h> */
DIR *opendir (const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_opendir == NULL) fakechroot_init();
    return next_opendir(name);
}
#endif


/* #include <unistd.h> */
long pathconf (const char *path, int name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_pathconf == NULL) fakechroot_init();
    return next_pathconf(path, name);
}


/* #include <unistd.h> */
int readlink (const char *path, char *buf, READLINK_TYPE_ARG3)
{
    int status;
    char tmp[FAKECHROOT_MAXPATH], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];

    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);

    if (next_readlink == NULL) fakechroot_init();

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

    if (next_realpath == NULL) fakechroot_init();

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
    if (next_remove == NULL) fakechroot_init();
    return next_remove(pathname);
}


#ifdef HAVE_REMOVEXATTR
/* #include <sys/xattr.h> */
int removexattr (const char *path, const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_removexattr == NULL) fakechroot_init();
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
    if (next_rename == NULL) fakechroot_init();
    return next_rename(oldpath, newpath);
}


#ifdef HAVE_REVOKE
/* #include <unistd.h> */
int revoke (const char *file)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_revoke == NULL) fakechroot_init();
    return next_revoke(file);
}
#endif


/* #include <unistd.h> */
int rmdir (const char *pathname)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_rmdir == NULL) fakechroot_init();
    return next_rmdir(pathname);
}


#ifdef HAVE_SCANDIR
/* #include <dirent.h> */
int scandir (const char *dir, struct dirent ***namelist, SCANDIR_TYPE_ARG3, int(*compar)(const void *, const void *))
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_scandir == NULL) fakechroot_init();
    return next_scandir(dir, namelist, filter, compar);
}
#endif


#ifdef HAVE_SCANDIR64
/* #include <dirent.h> */
int scandir64 (const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *))
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_scandir64 == NULL) fakechroot_init();
    return next_scandir64(dir, namelist, filter, compar);
}
#endif


#ifdef HAVE_SETXATTR
/* #include <sys/xattr.h> */
int setxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_setxattr == NULL) fakechroot_init();
    return next_setxattr(path, name, value, size, flags);
}
#endif


#if !defined(HAVE___XSTAT)
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat (const char *file_name, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_stat == NULL) fakechroot_init();
    return next_stat(file_name, buf);
}
#endif


#ifdef HAVE_STAT64
#if !defined(HAVE___XSTAT64)
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_stat64 == NULL) fakechroot_init();
    return next_stat64(file_name, buf);
}
#endif
#endif


/* #include <unistd.h> */
int symlink (const char *oldpath, const char *newpath)
{
    char tmp[FAKECHROOT_MAXPATH];
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(oldpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_symlink == NULL) fakechroot_init();
    return next_symlink(oldpath, newpath);
}


/* #include <stdio.h> */
char *tempnam (const char *dir, const char *pfx)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_tempnam == NULL) fakechroot_init();
    return next_tempnam(dir, pfx);
}


/* #include <stdio.h> */
char *tmpnam (char *s)
{
    char *ptr;
    char *fakechroot_path, *fakechroot_ptr, *fakechroot_buf;

    if (next_tmpnam == NULL) fakechroot_init();

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
    if (next_truncate == NULL) fakechroot_init();
    return next_truncate(path, length);
}


#ifdef HAVE_TRUNCATE64
/* #include <unistd.h> */
/* #include <sys/types.h> */
int truncate64 (const char *path, off64_t length)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_truncate64 == NULL) fakechroot_init();
    return next_truncate64(path, length);
}
#endif


#ifdef HAVE_ULCKPWDF
/* #include <shadow.h> */
int ulckpwdf (void)
{
    return 0;
}
#endif


/* #include <unistd.h> */
int unlink (const char *pathname)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_unlink == NULL) fakechroot_init();
    return next_unlink(pathname);
}


/* #include <sys/types.h> */
/* #include <utime.h> */
int utime (const char *filename, const struct utimbuf *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_utime == NULL) fakechroot_init();
    return next_utime(filename, buf);
}


/* #include <sys/time.h> */
int utimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    if (next_utimes == NULL) fakechroot_init();
    return next_utimes(filename, tv);
}
