/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>

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

#if defined(HAVE_FTS_OPEN) && !defined(HAVE___OPENDIR2)

#include <fts.h>
#include "libfakechroot.h"


wrapper(fts_open, FTS *, (char * const * path_argv, int options, int (* compar)(const FTSENT **, const FTSENT **)))
{
    char *fakechroot_path, *fakechroot_buf;
    char *path;
    char * const *p;
    char **new_path_argv;
    char **np;
    int n;

    debug("fts_open({\"%s\", ...}, %d, &compar)", path_argv[0], options);
    for (n=0, p=path_argv; *p; n++, p++);
    if ((new_path_argv = malloc(n*(sizeof(char *)))) == NULL) {
        return NULL;
    }

    for (n=0, p=path_argv, np=new_path_argv; *p; n++, p++, np++) {
        path = *p;
        expand_chroot_path_malloc(path, fakechroot_path, fakechroot_buf);
        *np = path;
    }

    return nextcall(fts_open)(new_path_argv, options, compar);
}

#endif
