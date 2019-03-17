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
#include <errno.h>
#include <stdlib.h>

#include "libfakechroot.h"
#include "strlcpy.h"


wrapper(mkostemps64, int, (char * template, int suffixlen, int flags))
{
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr = tmp;
    char *xxxsrc, *xxxdst;
    int xxxlen = 0;
    int fd;
    char fakechroot_abspath[FAKECHROOT_PATH_MAX];
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("mkostemps64(\"%s\", %d, %d)", template, suffixlen, flags);

    if (strlen(template)+suffixlen < 6) {
        __set_errno(EINVAL);
        return -1;
    }

    strlcpy(tmp, template, FAKECHROOT_PATH_MAX);

    if (!fakechroot_localdir(tmp)) {
        expand_chroot_path(tmpptr);
    }

    for (xxxdst = template; *xxxdst; xxxdst++);
    for (xxxdst -= 1 + suffixlen; *xxxdst == 'X'; xxxdst--, xxxlen++);
    xxxdst++;

    for (xxxsrc = tmpptr; *xxxsrc; xxxsrc++);
    for (xxxsrc -= 1 + suffixlen; *xxxsrc == 'X'; xxxsrc--);
    xxxsrc++;

    if ((fd = nextcall(mkostemps64)(tmpptr, suffixlen, flags)) == -1 || !*tmpptr) {
        goto error;
    }

    memmove(xxxdst, xxxsrc, xxxlen);
    return fd;

error:
    *template = '\0';
    return fd;
}

#else
typedef int empty_translation_unit;
#endif
