/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2021 Piotr Roszatycki <dexter@debian.org>

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

#ifdef HAVE_FSTATAT64

#define _ATFILE_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <limits.h>
#include "libfakechroot.h"

wrapper(fstatat64, int, (int dirfd, const char *pathname, struct stat64 *buf, int flags))
{
    char fakechroot_abspath[FAKECHROOT_PATH_MAX];
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("fstatat64(%d, \"%s\", &buf, %d)", dirfd, pathname, flags);
    expand_chroot_path_at(dirfd, pathname);
    return nextcall(fstatat64)(dirfd, pathname, buf, flags);
}

#else
typedef int empty_translation_unit;
#endif
