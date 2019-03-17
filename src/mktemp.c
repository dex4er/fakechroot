/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010-2015 Piotr Roszatycki <dexter@debian.org>

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

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>

#include "libfakechroot.h"
#include "strlcpy.h"


wrapper(mktemp, char *, (char * template))
{
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr = tmp;
    char *xxxsrc, *xxxdst;
    int xxxlen = 0;
    char fakechroot_abspath[FAKECHROOT_PATH_MAX];
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("mktemp(\"%s\")", template);

    strlcpy(tmp, template, FAKECHROOT_PATH_MAX);

    if (!fakechroot_localdir(tmp)) {
        expand_chroot_path(tmpptr);
    }

    for (xxxdst = template; *xxxdst; xxxdst++);
    for (xxxdst--; *xxxdst == 'X'; xxxdst--, xxxlen++);
    xxxdst++;

    for (xxxsrc = tmpptr; *xxxsrc; xxxsrc++);
    for (xxxsrc--; *xxxsrc == 'X'; xxxsrc--);
    xxxsrc++;

    if (nextcall(mktemp)(tmpptr) == NULL || !*tmpptr) {
        goto error;
    }

    memmove(xxxdst, xxxsrc, xxxlen);
    return template;

error:
    *template = '\0';
    return template;
}
