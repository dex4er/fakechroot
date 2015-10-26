/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2013-2015 Piotr Roszatycki <dexter@debian.org>

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

#define _BSD_SOURCE
#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#ifdef HAVE___LXSTAT64
# define _LARGEFILE64_SOURCE
#endif

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif


#ifdef SYS_getcwd

/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1.
 */

#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "libfakechroot.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

LOCAL char * getcwd_real(char *buf, size_t size)
{
    int ret;
    char *path;
    size_t alloc_size = size;

    if (size == 0) {
        if (buf != NULL) {
            __set_errno(EINVAL);
            return NULL;
        }
        alloc_size = MAX (PATH_MAX, getpagesize ());
    }
    path=buf;
    if (buf == NULL) {
        path = malloc(alloc_size);
        if (path == NULL)
            return NULL;
    }
    ret = syscall(SYS_getcwd, path, alloc_size);
    if (ret >= 0)
    {
        if (buf == NULL && size == 0)
            buf = realloc(path, ret);
        if (buf == NULL)
            buf = path;
        return buf;
    }
    if (buf == NULL)
        free (path);
    return NULL;
}

#else

/*      $OpenBSD: getcwd.c,v 1.14 2005/08/08 08:05:34 espie Exp $ */
/*
 * Copyright (c) 1989, 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>

#ifdef HAVE___LXSTAT64
# ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE
# endif
#endif

#include <sys/param.h>
#include <sys/stat.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libfakechroot.h"

#ifdef HAVE___LXSTAT64
# include "__lxstat64.h"
# define STAT_T stat64
# define FSTAT fstat64
# define LSTAT(path, buf) nextcall(__lxstat64)(_STAT_VER, path, buf)
#else
# include "lstat.h"
# define STAT_T stat
# define FSTAT fstat
# define LSTAT(path, buf) nextcall(lstat)(path, buf)
#endif

#if !defined(OPENDIR_CALLS___OPEN) && !defined(OPENDIR_CALLS___OPENDIR2)
# include "opendir.h"
# define OPENDIR(path) nextcall(opendir)(path)
#else
# define OPENDIR(path) opendir(path)
#endif

#undef MAXPATHLEN
#define MAXPATHLEN FAKECHROOT_PATH_MAX

#ifdef HAVE_STRUCT_DIRENT_D_NAMLEN
# define NAMLEN(d) ((d)->d_namlen)
#else
# define NAMLEN(d) (strlen ((d)->d_name))
#endif

#define ISDOT(dp) \
        (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' || \
            (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))

LOCAL char * getcwd_real(char *pt, size_t size)
{
        struct dirent *dp;
        DIR *dir = NULL;
        dev_t dev;
        ino_t ino;
        int first;
        char *bpt, *bup;
        struct STAT_T s;
        dev_t root_dev;
        ino_t root_ino;
        size_t ptsize, upsize;
        int save_errno;
        char *ept, *eup, *up;

        debug("getcwd_real(&pt, %d)", size);

        /*
         * If no buffer specified by the user, allocate one as necessary.
         * If a buffer is specified, the size has to be non-zero.  The path
         * is built from the end of the buffer backwards.
         */
        if (pt) {
                ptsize = 0;
                if (!size) {
                        errno = EINVAL;
                        return (NULL);
                }
                ept = pt + size;
        } else {
                if ((pt = malloc(ptsize = MAXPATHLEN)) == NULL)
            debug("getcwd_real(NULL, %d)", pt, size);
                        return (NULL);
                ept = pt + ptsize;
        }
        bpt = ept - 1;
        *bpt = '\0';

        /*
         * Allocate bytes for the string of "../"'s.
         * Should always be enough (it's 340 levels).  If it's not, allocate
         * as necessary.  Special * case the first stat, it's ".", not "..".
         */
        if ((up = malloc(upsize = MAXPATHLEN)) == NULL)
                goto err;
        eup = up + upsize;
        bup = up;
        up[0] = '.';
        up[1] = '\0';

        /* Save root values, so know when to stop. */
        if (LSTAT("/", &s))
                goto err;
        root_dev = s.st_dev;
        root_ino = s.st_ino;

        errno = 0;                      /* XXX readdir has no error return. */

        for (first = 1;; first = 0) {
                /* Stat the current level. */
                if (LSTAT(up, &s))
            goto err;

                /* Save current node values. */
                ino = s.st_ino;
                dev = s.st_dev;
                /* Check for reaching root. */
                if (root_dev == dev && root_ino == ino) {
                        *--bpt = '/';
                        /*
                         * It's unclear that it's a requirement to copy the
                         * path to the beginning of the buffer, but it's always
                         * been that way and stuff would probably break.
                         */
                        memmove(pt, bpt, ept - bpt);
                        free(up);
                    debug("getcwd_real(\"%s\", %d)", pt, size);
                        return (pt);
                }

                /*
                 * Build pointer to the parent directory, allocating memory
                 * as necessary.  Max length is 3 for "../", the largest
                 * possible component name, plus a trailing NUL.
                 */
                if (bup + 3  + MAXNAMLEN + 1 >= eup) {
                        char *nup;

                        if ((nup = realloc(up, upsize *= 2)) == NULL)
                                goto err;
                        bup = nup + (bup - up);
                        up = nup;
                        eup = up + upsize;
                }
                *bup++ = '.';
                *bup++ = '.';
                *bup = '\0';

                /* Open and stat parent directory. */
                if (!(dir = OPENDIR(up)) || FSTAT(dirfd(dir), &s))
                        goto err;

                /* Add trailing slash for next directory. */
                *bup++ = '/';

                /*
                 * If it's a mount point, have to stat each element because
                 * the inode number in the directory is for the entry in the
                 * parent directory, not the inode number of the mounted file.
                 */
                save_errno = 0;
                if (s.st_dev == dev) {
                        for (;;) {
                                if (!(dp = readdir(dir)))
                                        goto notfound;
                                if (dp->d_fileno == ino)
                                        break;
                        }
                } else
                        for (;;) {
                                if (!(dp = readdir(dir)))
                                        goto notfound;
                                if (ISDOT(dp))
                                        continue;
                                memcpy(bup, dp->d_name, NAMLEN(dp) + 1);

                                /* Save the first error for later. */
                                if (LSTAT(up, &s)) {
                                        if (!save_errno)
                                                save_errno = errno;
                                        errno = 0;
                                        continue;
                                }
                                if (s.st_dev == dev && s.st_ino == ino)
                                        break;
                        }

                /*
                 * Check for length of the current name, preceding slash,
                 * leading slash.
                 */
                if (bpt - pt < NAMLEN(dp) + (first ? 1 : 2)) {
                        size_t len;
                        char *npt;

                        if (!ptsize) {
                                errno = ERANGE;
                                goto err;
                        }
                        len = ept - bpt;
                        if ((npt = realloc(pt, ptsize *= 2)) == NULL)
                                goto err;
                        bpt = npt + (bpt - pt);
                        pt = npt;
                        ept = pt + ptsize;
                        memmove(ept - len, bpt, len);
                        bpt = ept - len;
                }
                if (!first)
                        *--bpt = '/';
                bpt -= NAMLEN(dp);
                memcpy(bpt, dp->d_name, NAMLEN(dp));
                (void)closedir(dir);

                /* Truncate any file name. */
                *bup = '\0';
        }

notfound:
        /*
         * If readdir set errno, use it, not any saved error; otherwise,
         * didn't find the current directory in its parent directory, set
         * errno to ENOENT.
         */
        if (!errno)
                errno = save_errno ? save_errno : ENOENT;
        /* FALLTHROUGH */
err:
        save_errno = errno;

        if (ptsize)
                free(pt);
        free(up);
        if (dir)
                (void)closedir(dir);

        errno = save_errno;

    debug("getcwd_real(NULL, %d)", pt, size);
        return (NULL);
}

#endif
