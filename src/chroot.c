/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>

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

#define _GNU_SOURCE
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "libfakechroot.h"

#include "__xstat64.h"
#include "getcwd.h"

wrapper(chroot, int, (const char * path))
{
    char *ptr, *ld_library_path, *tmp, *fakechroot_path;
    int status, len;
    char dir[FAKECHROOT_PATH_MAX], cwd[FAKECHROOT_PATH_MAX];
#if !defined(HAVE_SETENV)
    char *envbuf;
#endif
#if defined(HAVE___XSTAT64) && defined(_STAT_VER)
    struct stat64 sb;
#else
    struct stat sb;
#endif

    debug("chroot(\"%s\")", path);
    if (path == NULL) {
        __set_errno(EFAULT);
        return -1;
    }
    if (!*path) {
        __set_errno(ENOENT);
        return -1;
    }
    if (*path != '/') {
        if (nextcall(getcwd)(cwd, FAKECHROOT_PATH_MAX) == NULL) {
            __set_errno(ENAMETOOLONG);
            return -1;
        }
        if (cwd == NULL) {
            __set_errno(EFAULT);
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

#if defined(HAVE___XSTAT64) && defined(_STAT_VER)
    if ((status = nextcall(__xstat64)(_STAT_VER, dir, &sb)) != 0) {
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
