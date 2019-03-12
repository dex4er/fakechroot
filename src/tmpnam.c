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

#include "libfakechroot.h"

#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L
#include <string.h>


wrapper(tmpnam, char *, (char * s))
{
    char fakechroot_abspath[FAKECHROOT_PATH_MAX];
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    char *ptr, *ptr2;

    debug("tmpnam(&s)");
    if (s != NULL) {
        return nextcall(tmpnam)(s);
    }

    ptr = nextcall(tmpnam)(NULL);

    expand_chroot_path(ptr);

    ptr2 = malloc(strlen(ptr));
    if (ptr2 == NULL) return NULL;

    strcpy(ptr2, ptr);
    return ptr2;
}
