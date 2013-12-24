/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013 Piotr Roszatycki <dexter@debian.org>

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

#ifdef HAVE___LXSTAT64
# define _LARGEFILE64_SOURCE
#endif

#include <stddef.h>
#include <sys/stat.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "libfakechroot.h"
#include "readlink.h"

#ifdef HAVE___LXSTAT64
# define _LARGEFILE64_SOURCE
# include "__lxstat64.h"
# define STAT_T stat64
# define LSTAT_REL(rpath, st) __lxstat64_rel(_STAT_VER, rpath, st)
#else
# include "lstat.h"
# define STAT_T stat
# define LSTAT_REL(rpath, st) lstat_rel(rpath, st)
#endif


/*
   realpath function taken from Gnulib.
   Copyright (C) 1996-2010 Free Software Foundation, Inc.
*/

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

wrapper(realpath, char *, (const char * name, char * resolved))
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
        __set_errno(EINVAL);
        return NULL;
    }

    if (name[0] == '\0') {
        /* As per Single Unix Specification V2 we must return an error if
           the name argument points to an empty string.  */
        __set_errno(ENOENT);
        return NULL;
    }

    path_max = FAKECHROOT_PATH_MAX;

    if (resolved == NULL) {
        rpath = malloc(path_max);
        if (rpath == NULL) {
            /* It's easier to set errno to ENOMEM than to rely on the
               'malloc-posix' gnulib module.  */
            __set_errno(ENOMEM);
            return NULL;
        }
    } else
        rpath = resolved;
    rpath_limit = rpath + path_max;

    if (name[0] != '/') {
        if (!getcwd(rpath, path_max)) {
            rpath[0] = '\0';
            goto error;
        }
        dest = strchr(rpath, '\0');
    } else {
        rpath[0] = '/';
        dest = rpath + 1;
        if (DOUBLE_SLASH_IS_DISTINCT_ROOT && name[1] == '/')
            *dest++ = '/';
    }

    for (start = end = name; *start; start = end) {
        struct STAT_T st;
        int n;

        /* Skip sequence of multiple path-separators.  */
        while (*start == '/')
            ++start;

        /* Find end of path component.  */
        for (end = start; *end && *end != '/'; ++end)
            /* Nothing.  */;

        if (end - start == 0)
            break;
        else if (end - start == 1 && start[0] == '.') {
            /* nothing */
        } else if (end - start == 2 && start[0] == '.' && start[1] == '.') {
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
                    __set_errno(ENAMETOOLONG);
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
                new_rpath = (char *) realloc(rpath, new_size);
                if (new_rpath == NULL) {
                    /* It's easier to set errno to ENOMEM than to rely on the
                       'realloc-posix' gnulib module.  */
                    __set_errno(ENOMEM);
                    goto error;
                }
                rpath = new_rpath;
                rpath_limit = rpath + new_size;

                dest = rpath + dest_offset;
            }

            memcpy(dest, start, end - start);
            dest += end - start;
            *dest = '\0';

            if (LSTAT_REL (rpath, &st) < 0)
                goto error;

            if (S_ISLNK (st.st_mode)) {
                char *buf;
                size_t len;

                if (++num_links > MAXSYMLINKS) {
                    __set_errno(ELOOP);
                    goto error;
                }

                buf = alloca(path_max);
                if (!buf) {
                    __set_errno(ENOMEM);
                    goto error;
                }

                n = readlink(rpath, buf, path_max - 1);
                if (n < 0) {
                    int saved_errno = errno;
                    __set_errno(saved_errno);
                    goto error;
                }
                buf[n] = '\0';

                if (!extra_buf) {
                    extra_buf = alloca(path_max);
                    if (!extra_buf) {
                        __set_errno(ENOMEM);
                        goto error;
                    }
                }

                len = strlen(end);
                if ((long int) (n + len) >= path_max) {
                    __set_errno(ENAMETOOLONG);
                    goto error;
                }

                /* Careful here, end may be a pointer into extra_buf... */
                memmove(&extra_buf[n], end, len + 1);
                name = end = memcpy(extra_buf, buf, n);

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
                __set_errno(ENOTDIR);
                goto error;
            }
        }
    }
    if (dest > rpath + 1 && dest[-1] == '/')
        --dest;
    if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rpath + 1 && *dest == '/')
        dest++;
    *dest = '\0';

    return rpath;

error: {
        int saved_errno = errno;
        if (resolved == NULL)
            free(rpath);
        __set_errno(saved_errno);
    }
    return NULL;
}
