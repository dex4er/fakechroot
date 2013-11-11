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


wrapper(mktemp, char *, (char * template))
{
    char tmp[FAKECHROOT_PATH_MAX];
    char *ptr = tmp, *ptr2 = NULL;

    debug("mktemp(\"%s\")", template);

    tmp[FAKECHROOT_PATH_MAX-1] = '\0';
    strncpy(tmp, template, FAKECHROOT_PATH_MAX-2);

    if (!fakechroot_localdir(tmp)) {
        expand_chroot_path(ptr);
        ptr2 = malloc(strlen(ptr));
        strcpy(ptr2, ptr);
        ptr = ptr2;
    }

    if (nextcall(mktemp)(ptr) == NULL || !*ptr) {
        strcpy(template, "");
        goto error;
    }

    narrow_chroot_path(ptr);

    strncpy(template, ptr, strlen(template));

error:
    if (ptr2) free(ptr2);
    return template;
}
