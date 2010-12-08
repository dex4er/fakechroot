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
#define _BSD_SOURCE
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

#include "libfakechroot.h"


/* Useful to exclude a list of directories or files */
static char *exclude_list[32];
static int exclude_length[32];
static int list_max = 0;
static int first = 0;
static char *home_path=NULL;


LOCAL int fakechroot_debug (const char *fmt, ...)
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
// wrapper_proto(chdir, int, (const char *));
wrapper_proto(chmod, int, (const char *, mode_t));
wrapper_proto(chown, int, (const char *, uid_t, gid_t));
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


#include "getcwd.h"


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
LOCAL fakechroot_wrapperfn_t fakechroot_loadfunc (struct fakechroot_wrapper *w)
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
LOCAL int fakechroot_localdir (const char *p_path)
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
