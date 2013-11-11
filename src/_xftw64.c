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

#ifdef HAVE__XFTW64

#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include "libfakechroot.h"


static int (* _xftw64_fn_saved)(const char * file, const struct stat * sb, int flag);

static int _xftw64_fn_wrapper (const char * file, const struct stat * sb, int flag)
{
    narrow_chroot_path(file);
    return _xftw64_fn_saved(file, sb, flag);
}

wrapper(_xftw64, int, (int mode, const char * dir, int (* fn)(const char * file, const struct stat64 * sb, int flag), int nopenfd))
{
    debug("_xftw64(%d, \"%s\", &fn, %d)", mode, dir, nopenfd);
    expand_chroot_path(dir);
    _xftw64_fn_saved = fn;
    return nextcall(_xftw64)(mode, dir, _xftw64_fn_wrapper, nopenfd);
}

#else
typedef int empty_translation_unit;
#endif
