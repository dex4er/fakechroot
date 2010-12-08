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

#ifdef HAVE_NFTW64

#define _XOPEN_SOURCE 500
#define _LARGEFILE64_SOURCE
#include <stddef.h>
#include <ftw.h>
#include "libfakechroot.h"


static int (* nftw64_fn_saved)(const char * file, const struct stat64 * sb, int flag, struct FTW * s);

static int nftw64_fn_wrapper (const char * file, const struct stat64 * sb, int flag, struct FTW * s)
{
    char *fakechroot_path, *fakechroot_ptr;
    narrow_chroot_path(file, fakechroot_path, fakechroot_ptr);
    return nftw64_fn_saved(file, sb, flag, s);
}

wrapper(nftw64, int, (const char * dir, int (* fn)(const char * file, const struct stat64 * sb, int flag, struct FTW * s), int nopenfd, int flags))
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    debug("ntfw64(\"%s\", &fn, %d, %d)", dir, nopenfd, flags);
    expand_chroot_path(dir, fakechroot_path, fakechroot_buf);
    nftw64_fn_saved = fn;
    return nextcall(nftw64)(dir, nftw64_fn_wrapper, nopenfd, flags);
}

#endif
