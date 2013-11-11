/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2013 Piotr Roszatycki <dexter@debian.org>

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

#ifdef HAVE_MKOSTEMPS64

#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include "libfakechroot.h"


wrapper(mkostemps64, int, (char * template, int suffixlen, int flags))
{
    char tmp[FAKECHROOT_PATH_MAX], *oldtemplate, *ptr;
    int fd;

    debug("mkostemps64(\"%s\", %d, %d)", template, suffixlen, flags);
    oldtemplate = template;

    expand_chroot_path(template);

    if ((fd = nextcall(mkostemps64)(template, suffixlen, flags)) == -1) {
        return -1;
    }
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr);
    if (ptr != NULL) {
        strcpy(oldtemplate, ptr);
    }
    return fd;
}

#endif
