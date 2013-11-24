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

#ifdef HAVE___GETWD_CHK

#define _FORTIFY_SOURCE 2
#include <stddef.h>
#include "libfakechroot.h"


wrapper(__getwd_chk, char *, (char * buf, size_t buflen))
{
    char *cwd;

    debug("__getwd_chk(&buf, %zd)", buflen);
    if ((cwd = nextcall(__getwd_chk)(buf, buflen)) == NULL) {
        return NULL;
    }
    narrow_chroot_path(cwd);
    return cwd;
}

#else
typedef int empty_translation_unit;
#endif
