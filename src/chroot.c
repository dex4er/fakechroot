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
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "setenv.h"
#include "libfakechroot.h"

#ifdef HAVE___XSTAT64
# include "__xstat64.h"
#else
# include "stat.h"
#endif
#include "getcwd.h"

wrapper(chroot, int, (const char * path))
{
    char *ptr, *ld_library_path, *separator, *tmp, *fakechroot_path;
    int status, len;
    char dir[FAKECHROOT_PATH_MAX], cwd[FAKECHROOT_PATH_MAX];
#ifdef HAVE___XSTAT64
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

#ifdef HAVE___XSTAT64
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

    setenv("FAKECHROOT_BASE", dir, 1);
    fakechroot_path = getenv("FAKECHROOT_BASE");

    ld_library_path = getenv("LD_LIBRARY_PATH");
    if (ld_library_path != NULL && strlen(ld_library_path) > 0) {
        separator = ":";
    }
    else {
        ld_library_path = "";
        separator = "";
    }

    if ((len = strlen(ld_library_path)+strlen(separator)+strlen(dir)*2+sizeof("/usr/lib:/lib")) > FAKECHROOT_PATH_MAX) {
        return ENAMETOOLONG;
    }

    if ((tmp = malloc(len)) == NULL) {
        return ENOMEM;
    }

    snprintf(tmp, len, "%s%s%s/usr/lib:%s/lib", ld_library_path, separator, dir, dir);
    setenv("LD_LIBRARY_PATH", tmp, 1);
    free(tmp);
    return 0;
}
