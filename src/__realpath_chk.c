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

#ifdef HAVE___REALPATH_CHK

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define _FORTIFY_SOURCE 2
#include <stddef.h>
#include <stdlib.h>
#include "libfakechroot.h"


#ifdef HAVE___CHK_FAIL
extern void __chk_fail (void) __attribute__((__noreturn__));
#else
__attribute__((__noreturn__)) void __chk_fail (void)
{
        exit(-1);
}
#endif


wrapper(__realpath_chk, char *, (const char * path, char * resolved, size_t resolvedlen))
{
    debug("__realpath_chk(\"%s\", &buf, %zd)", path, resolvedlen);
    if (resolvedlen < FAKECHROOT_PATH_MAX)
        __chk_fail();

    return realpath(path, resolved);
}

#else
typedef int empty_translation_unit;
#endif
