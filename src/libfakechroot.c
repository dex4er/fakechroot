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
#include <sys/xattr.h>


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


static int     (*next___lxstat) (int ver, const char *filename, struct stat *buf);
#ifdef __USE_LARGEFILE64
static int     (*next___lxstat64) (int ver, const char *filename, struct stat64 *buf);
#endif
static int     (*next___open) (const char *pathname, int flags, ...);
#ifdef __USE_LARGEFILE64
static int     (*next___open64) (const char *pathname, int flags, ...);
#endif
static int     (*next___xmknod) (int ver, const char *path, mode_t mode, dev_t *dev);
static int     (*next___xstat) (int ver, const char *filename, struct stat *buf);
#ifdef __USE_LARGEFILE64
static int     (*next___xstat64) (int ver, const char *filename, struct stat64 *buf);
#endif
static int     (*next_access) (const char *pathname, int mode);
static int     (*next_acct) (const char *filename);
static char *  (*next_canonicalize_file_name) (const char *name);
static int     (*next_chdir) (const char *path);
static int     (*next_chmod) (const char *path, mode_t mode);
static int     (*next_chown) (const char *path, uid_t owner, gid_t group);
static int     (*next_chroot) (const char *path);
static int     (*next_creat) (const char *pathname, mode_t mode);
#ifdef __USE_LARGEFILE64
static int     (*next_creat64) (const char *pathname, mode_t mode);
#endif
static void *  (*next_dlmopen) (Lmid_t nsid, const char *filename, int flag);
static void *  (*next_dlopen) (const char *filename, int flag);
static int     (*next_euidaccess) (const char *pathname, int mode);
static int     (*next_execl) (const char *path, const char *arg, ...);
static int     (*next_execle) (const char *path, const char *arg, ...);
static int     (*next_execlp) (const char *file, const char *arg, ...);
static int     (*next_execv) (const char *path, char *const argv []);
static int     (*next_execve) (const char *filename, char *const argv [], char *const envp[]);
static int     (*next_execvp) (const char *file, char *const argv []);
static FILE *  (*next_fopen) (const char *path, const char *mode);
#ifdef __USE_LARGEFILE64
static FILE *  (*next_fopen64) (const char *path, const char *mode);
#endif
static FILE *  (*next_freopen) (const char *path, const char *mode, FILE *stream);
#ifdef __USE_LARGEFILE64
static FILE *  (*next_freopen64) (const char *path, const char *mode, FILE *stream);
#endif
static char *  (*next_get_current_dir_name) (void);
static char *  (*next_getcwd) (char *buf, size_t size);
static char *  (*next_getwd) (char *buf);
static ssize_t (*next_getxattr) (const char *path, const char *name, void *value, size_t size);
static int     (*next_glob) (const char *pattern, int flags, int (*errfunc) (const char *, int), glob_t *pglob);
#ifdef __USE_LARGEFILE64
static int     (*next_glob64) (const char *pattern, int flags, int (*errfunc) (const char *, int), glob64_t *pglob);
#endif
static int     (*next_glob_pattern_p) (const char *pattern, int quote);
static int     (*next_lchmod) (const char *path, mode_t mode);
static int     (*next_lchown) (const char *path, uid_t owner, gid_t group);
static int     (*next_lckpwdf) (void);
static ssize_t (*next_lgetxattr) (const char *path, const char *name, void *value, size_t size);
static int     (*next_link) (const char *oldpath, const char *newpath);
static ssize_t (*next_listxattr) (const char *path, char *list, size_t size);
static ssize_t (*next_llistxattr) (const char *path, char *list, size_t size);
static int     (*next_lremovexattr) (const char *path, const char *name);
static int     (*next_lsetxattr) (const char *path, const char *name, const void *value, size_t size, int flags);
#ifndef __GLIBC__
static int     (*next_lstat) (const char *file_name, struct stat *buf);
#ifdef __USE_LARGEFILE64
static int     (*next_lstat64) (const char *file_name, struct stat64 *buf);
#endif
#endif
static int     (*next_lutimes) (const char *filename, const struct timeval tv[2]);
static int     (*next_mkdir) (const char *pathname, mode_t mode);
static char *  (*next_mkdtemp) (char *template);
static int     (*next_mknod) (const char *pathname, mode_t mode, dev_t dev);
static int     (*next_mkfifo) (const char *pathname, mode_t mode);
static int     (*next_mkstemp) (char *template);
#ifdef __USE_LARGEFILE64
static int     (*next_mkstemp64) (char *template);
#endif
static char *  (*next_mktemp) (char *template);
static int     (*next_open) (const char *pathname, int flags, ...);
#ifdef __USE_LARGEFILE64
static int     (*next_open64) (const char *pathname, int flags, ...);
#endif
static DIR *   (*next_opendir) (const char *name);
static long    (*next_pathconf) (const char *path, int name);
static int     (*next_readlink) (const char *path, char *buf, size_t bufsiz);
static char *  (*next_realpath) (const char *name, char *resolved);
static int     (*next_remove) (const char *pathname);
static int     (*next_removexattr) (const char *path, const char *name);
static int     (*next_rename) (const char *oldpath, const char *newpath);
static int     (*next_revoke) (const char *file);
static int     (*next_rmdir) (const char *pathname);
static int     (*next_scandir) (const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *));
#ifdef __USE_LARGEFILE64
static int     (*next_scandir64) (const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *));
#endif
static int     (*next_setxattr) (const char *path, const char *name, const void *value, size_t size, int flags);
#ifndef __GLIBC__
static int     (*next_stat) (const char *file_name, struct stat *buf);
#ifdef __USE_LARGEFILE64
static int     (*next_stat64) (const char *file_name, struct stat64 *buf);
#endif
#endif
static int     (*next_symlink) (const char *oldpath, const char *newpath);
static char *  (*next_tempnam) (const char *dir, const char *pfx);
static char *  (*next_tmpnam) (char *s);
static int     (*next_truncate) (const char *path, off_t length);
#ifdef __USE_LARGEFILE64
static int     (*next_truncate64) (const char *path, off64_t length);
#endif
static int     (*next_unlink) (const char *pathname);
static int     (*next_ulckpwdf) (void);
static int     (*next_utime) (const char *filename, const struct utimbuf *buf);
static int     (*next_utimes) (const char *filename, const struct timeval tv[2]);


void fakechroot_init (void) __attribute((constructor));
void fakechroot_init (void)
{
    *(void **)(&next___lxstat)                = dlsym(RTLD_NEXT, "__lxstat");
#ifdef __USE_LARGEFILE64
    *(void **)(&next___lxstat64)              = dlsym(RTLD_NEXT, "__lxstat64");
#endif
    *(void **)(&next___open)                  = dlsym(RTLD_NEXT, "__open");
    *(void **)(&next___open64)                = dlsym(RTLD_NEXT, "__open64");
    *(void **)(&next___xmknod)                = dlsym(RTLD_NEXT, "__xmknod");
    *(void **)(&next___xstat)                 = dlsym(RTLD_NEXT, "__xstat");
#ifdef __USE_LARGEFILE64
    *(void **)(&next___xstat64)               = dlsym(RTLD_NEXT, "__xstat64");
#endif
    *(void **)(&next_access)                  = dlsym(RTLD_NEXT, "access");
    *(void **)(&next_acct)                    = dlsym(RTLD_NEXT, "acct");
    *(void **)(&next_canonicalize_file_name)  = dlsym(RTLD_NEXT, "canonicalize_file_name");
    *(void **)(&next_chdir)                   = dlsym(RTLD_NEXT, "chdir");
    *(void **)(&next_chmod)                   = dlsym(RTLD_NEXT, "chmod");
    *(void **)(&next_chown)                   = dlsym(RTLD_NEXT, "chown");
    *(void **)(&next_chroot)                  = dlsym(RTLD_NEXT, "chroot");
    *(void **)(&next_creat)                   = dlsym(RTLD_NEXT, "creat");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_creat64)                 = dlsym(RTLD_NEXT, "creat64");
#endif
    *(void **)(&next_dlmopen)                 = dlsym(RTLD_NEXT, "dlmopen");
    *(void **)(&next_dlopen)                  = dlsym(RTLD_NEXT, "dlopen");
    *(void **)(&next_euidaccess)              = dlsym(RTLD_NEXT, "euidaccess");
    *(void **)(&next_execl)                   = dlsym(RTLD_NEXT, "execl");
    *(void **)(&next_execle)                  = dlsym(RTLD_NEXT, "execle");
    *(void **)(&next_execlp)                  = dlsym(RTLD_NEXT, "execlp");
    *(void **)(&next_execv)                   = dlsym(RTLD_NEXT, "execv");
    *(void **)(&next_execve)                  = dlsym(RTLD_NEXT, "execve");
    *(void **)(&next_execvp)                  = dlsym(RTLD_NEXT, "execvp");
    *(void **)(&next_fopen)                   = dlsym(RTLD_NEXT, "fopen");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_fopen64)                 = dlsym(RTLD_NEXT, "fopen64");
#endif
    *(void **)(&next_freopen)                 = dlsym(RTLD_NEXT, "freopen");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_freopen64)               = dlsym(RTLD_NEXT, "freopen64");
#endif
    *(void **)(&next_get_current_dir_name)    = dlsym(RTLD_NEXT, "get_current_dir_name");
    *(void **)(&next_getcwd)                  = dlsym(RTLD_NEXT, "getcwd");
    *(void **)(&next_getwd)                   = dlsym(RTLD_NEXT, "getwd");
    *(void **)(&next_getxattr)                = dlsym(RTLD_NEXT, "getxattr");
    *(void **)(&next_glob)                    = dlsym(RTLD_NEXT, "glob");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_glob64)                  = dlsym(RTLD_NEXT, "glob64");
#endif
    *(void **)(&next_glob_pattern_p)          = dlsym(RTLD_NEXT, "glob_pattern_p");
    *(void **)(&next_lchmod)                  = dlsym(RTLD_NEXT, "lchmod");
    *(void **)(&next_lchown)                  = dlsym(RTLD_NEXT, "lchown");
    *(void **)(&next_lckpwdf)                 = dlsym(RTLD_NEXT, "lckpwdf");
    *(void **)(&next_lgetxattr)               = dlsym(RTLD_NEXT, "lgetxattr");
    *(void **)(&next_listxattr)               = dlsym(RTLD_NEXT, "listxattr");
    *(void **)(&next_link)                    = dlsym(RTLD_NEXT, "link");
    *(void **)(&next_llistxattr)              = dlsym(RTLD_NEXT, "llistxattr");
    *(void **)(&next_lremovexattr)            = dlsym(RTLD_NEXT, "lremovexattr");
    *(void **)(&next_lsetxattr)               = dlsym(RTLD_NEXT, "lsetxattr");
#ifndef __GLIBC__
    *(void **)(&next_lstat)                   = dlsym(RTLD_NEXT, "lstat");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_lstat64)                 = dlsym(RTLD_NEXT, "lstat64");
#endif
#endif
    *(void **)(&next_lutimes)                 = dlsym(RTLD_NEXT, "lutimes");
    *(void **)(&next_mkdir)                   = dlsym(RTLD_NEXT, "mkdir");
    *(void **)(&next_mkdtemp)                 = dlsym(RTLD_NEXT, "mkdtemp");
    *(void **)(&next_mknod)                   = dlsym(RTLD_NEXT, "mknod");
    *(void **)(&next_mkfifo)                  = dlsym(RTLD_NEXT, "mkfifo");
    *(void **)(&next_mkstemp)                 = dlsym(RTLD_NEXT, "mkstemp");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_mkstemp64)               = dlsym(RTLD_NEXT, "mkstemp64");
#endif
    *(void **)(&next_mktemp)                  = dlsym(RTLD_NEXT, "mktemp");
    *(void **)(&next_open)                    = dlsym(RTLD_NEXT, "open");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_open64)                  = dlsym(RTLD_NEXT, "open64");
#endif
    *(void **)(&next_opendir)                 = dlsym(RTLD_NEXT, "opendir");
    *(void **)(&next_pathconf)                = dlsym(RTLD_NEXT, "pathconf");
    *(void **)(&next_readlink)                = dlsym(RTLD_NEXT, "readlink");
    *(void **)(&next_realpath)                = dlsym(RTLD_NEXT, "realpath");
    *(void **)(&next_remove)                  = dlsym(RTLD_NEXT, "remove");
    *(void **)(&next_removexattr)             = dlsym(RTLD_NEXT, "removexattr");
    *(void **)(&next_rename)                  = dlsym(RTLD_NEXT, "rename");
    *(void **)(&next_revoke)                  = dlsym(RTLD_NEXT, "revoke");
    *(void **)(&next_rmdir)                   = dlsym(RTLD_NEXT, "rmdir");
    *(void **)(&next_scandir)                 = dlsym(RTLD_NEXT, "scandir");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_scandir64)               = dlsym(RTLD_NEXT, "scandir64");
#endif
    *(void **)(&next_setxattr)                = dlsym(RTLD_NEXT, "setxattr");
#ifndef __GLIBC__
    *(void **)(&next_stat)                    = dlsym(RTLD_NEXT, "stat");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_stat64)                  = dlsym(RTLD_NEXT, "stat64");
#endif
#endif
    *(void **)(&next_symlink)                 = dlsym(RTLD_NEXT, "symlink");
    *(void **)(&next_tempnam)                 = dlsym(RTLD_NEXT, "tempnam");
    *(void **)(&next_tmpnam)                  = dlsym(RTLD_NEXT, "tmpnam");
    *(void **)(&next_truncate)                = dlsym(RTLD_NEXT, "truncate");
#ifdef __USE_LARGEFILE64
    *(void **)(&next_truncate64)              = dlsym(RTLD_NEXT, "truncate64");
#endif
    *(void **)(&next_unlink)                  = dlsym(RTLD_NEXT, "unlink");
    *(void **)(&next_ulckpwdf)                = dlsym(RTLD_NEXT, "ulckpwdf");
    *(void **)(&next_utime)                   = dlsym(RTLD_NEXT, "utime");
    *(void **)(&next_utimes)                  = dlsym(RTLD_NEXT, "utimes");
}


/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___lxstat(ver, filename, buf);
}


#ifdef __USE_LARGEFILE64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __lxstat64 (int ver, const char *filename, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___lxstat64(ver, filename, buf);
}
#endif


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


#ifdef __USE_LARGEFILE64
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


/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xmknod (int ver, const char *path, mode_t mode, dev_t *dev)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___xmknod(ver, path, mode, dev);
}


/* #include <sys/stat.h> */
/* #include <unistd.h> */
int __xstat (int ver, const char *filename, struct stat *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next___xstat(ver, filename, buf);
}


#ifdef __USE_LARGEFILE64
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


/* #include <stdlib.h> */
char *canonicalize_file_name (const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_canonicalize_file_name(name);
}


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

    setenv("FAKECHROOT_BASE", path, 1);
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
    setenv("LD_LIBRARY_PATH", tmp, 1);
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


#ifdef __USE_LARGEFILE64
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
int creat64 (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_creat64(pathname, mode);
}
#endif


/* #include <dlfcn.h> */
void *dlmopen (Lmid_t nsid, const char *filename, int flag)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_dlmopen(nsid, filename, flag);
}


/* #include <dlfcn.h> */
void *dlopen (const char *filename, int flag)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_dlopen(filename, flag);
}


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


/* #include <unistd.h> */
int euidaccess (const char *pathname, int mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_euidaccess(pathname, mode);
}


/* #include <stdio.h> */
FILE *fopen (const char *path, const char *mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_fopen(path, mode);
}


#ifdef __USE_LARGEFILE64
/* #include <stdio.h> */
FILE *fopen64 (const char *path, const char *mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_fopen64(path, mode);
}
#endif


/* #include <stdio.h> */
FILE *freopen (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_freopen(path, mode, stream);
}


#ifdef __USE_LARGEFILE64
/* #include <stdio.h> */
FILE *freopen64 (const char *path, const char *mode, FILE *stream)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_freopen64(path, mode, stream);
}
#endif


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


/* #include <sys/xattr.h> */
ssize_t getxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_getxattr(path, name, value, size);
}


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


#ifdef __USE_LARGEFILE64
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


/* #include <glob.h> */
int glob_pattern_p (const char *pattern, int quote)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pattern, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_glob_pattern_p(pattern, quote);
}


/* #include <sys/types.h> */
/* #include <sys/stat.h> */
int lchmod (const char *path, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lchmod(path, mode);
}


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


/* #include <sys/xattr.h> */
ssize_t lgetxattr (const char *path, const char *name, void *value, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lgetxattr(path, name, value, size);
}


/* #include <sys/xattr.h> */
ssize_t listxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_listxattr(path, list, size);
}


/* #include <sys/xattr.h> */
ssize_t llistxattr (const char *path, char *list, size_t size)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_llistxattr(path, list, size);
}


/* #include <sys/xattr.h> */
int lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lsetxattr(path, name, value, size, flags);
}


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
#ifdef __USE_LARGEFILE64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int lstat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lstat64(file_name, buf);
}
#endif
#endif


/* #include <sys/time.h> */
int lutimes (const char *filename, const struct timeval tv[2])
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(filename, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_lutimes(filename, tv);
}


/* #include <sys/stat.h> */
/* #include <sys/types.h> */
int mkdir (const char *pathname, mode_t mode)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_mkdir(pathname, mode);
}


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


#ifdef __USE_LARGEFILE64
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
#endif


/* #include <stdlib.h> */
char *mktemp (char *template)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(template, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_mktemp(template);
}


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


#ifdef __USE_LARGEFILE64
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
#endif


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


/* #include <sys/xattr.h> */
int removexattr (const char *path, const char *name)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_removexattr(path, name);
}


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


/* #include <unistd.h> */
int revoke (const char *file)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_rmdir(file);
}


/* #include <unistd.h> */
int rmdir (const char *pathname)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(pathname, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_rmdir(pathname);
}


/* #include <dirent.h> */
int scandir (const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *))
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_scandir(dir, namelist, filter, compar);
}


#ifdef __USE_LARGEFILE64
/* #include <dirent.h> */
int scandir64 (const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *))
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(dir, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_scandir64(dir, namelist, filter, compar);
}
#endif


/* #include <sys/xattr.h> */
int setxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_setxattr(path, name, value, size, flags);
}


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
#ifdef __USE_LARGEFILE64
/* #include <sys/stat.h> */
/* #include <unistd.h> */
int stat64 (const char *file_name, struct stat64 *buf)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(file_name, fakechroot_path, fakechroot_ptr, fakechroot_buf);
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


#ifdef __USE_LARGEFILE64
/* #include <unistd.h> */
/* #include <sys/types.h> */
int truncate64 (const char *path, off64_t length)
{
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_MAXPATH];
    expand_chroot_path(path, fakechroot_path, fakechroot_ptr, fakechroot_buf);
    return next_truncate64(path, length);
}
#endif


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
