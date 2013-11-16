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

#ifndef HAVE___XMKNOD

#include <sys/stat.h>
#include "libfakechroot.h"


wrapper(mknod, int, (const char * pathname, mode_t mode, dev_t dev))
{
    debug("mknod(\"%s\", 0%o, %ld)", pathname, mode, dev);
    expand_chroot_path(pathname);
    return nextcall(mknod)(pathname, mode, dev);
}

#else
typedef int empty_translation_unit;
#endif
