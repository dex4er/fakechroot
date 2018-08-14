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

#ifdef HAVE_SETXATTR

#include "libfakechroot.h"


wrapper_versioned(setxattr, "GLIBC_2.3", int, (const char * path, const char * name, const void * value, size_t size, int flags))
{
    debug("setxattr(\"%s\", \"%s\", &value, %zd, %d)", path, name, size, flags);
    expand_chroot_path(path);
    return nextcall(setxattr)(path, name, value, size, flags);
}

#else
typedef int empty_translation_unit;
#endif
