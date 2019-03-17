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

#include <stdarg.h>
#include <stddef.h>
#include <fcntl.h>
#include "libfakechroot.h"


wrapper_alias(open, int, (const char * pathname, int flags, ...))
{
    char fakechroot_abspath[FAKECHROOT_PATH_MAX];
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    int mode = 0;

    va_list arg;
    va_start(arg, flags);

    debug("open(\"%s\", %d, ...)", pathname, flags);
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        mode = va_arg(arg, int);
        va_end(arg);
    }

    return nextcall(open)(pathname, flags, mode);
}
