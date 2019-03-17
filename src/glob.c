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

#include <glob.h>
#include "libfakechroot.h"


wrapper(glob, int, (const char * pattern, int flags, int (* errfunc) (const char *, int), glob_t * pglob))
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];
    int rc, i;

    debug("glob(\"%s\", %d, &errfunc, &pglob)", pattern, flags);
    expand_chroot_rel_path(pattern);

    rc = nextcall(glob)(pattern, flags, errfunc, pglob);
    if (rc < 0)
        return rc;

    for (i = 0; i < pglob->gl_pathc; i++) {
        const char *fakechroot_base = getenv("FAKECHROOT_BASE");
        char tmp[FAKECHROOT_PATH_MAX], *tmpptr;

        strcpy(tmp, pglob->gl_pathv[i]);

        if (fakechroot_base != NULL) {
            const char *ptr = strstr(tmp, fakechroot_base);
            if (ptr != tmp) {
                tmpptr = tmp;
            } else {
                tmpptr = tmp + strlen(fakechroot_base);
            }
            strcpy(pglob->gl_pathv[i], tmpptr);
        }
    }
    return rc;
}
