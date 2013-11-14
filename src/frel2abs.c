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

#ifdef HAVE_FCHDIR

#define _BSD_SOURCE
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "libfakechroot.h"
#include "strlcpy.h"
#include "de_dotdot.h"
#include "open.h"


LOCAL char * frel2abs(int dirfd, const char * name, char * resolved)
{
    int cwdfd;
    char cwd[FAKECHROOT_PATH_MAX];

    debug("frel2abs(%d, \"%s\", &resolved)", dirfd, name);
    strlcpy(resolved, name, FAKECHROOT_PATH_MAX);

    if ((cwdfd = nextcall(open)(".", O_RDONLY|O_DIRECTORY)) == -1) {
        goto error;
    }

    if (fchdir(dirfd) == -1) {
        goto error;
    }
    if (getcwd(cwd, FAKECHROOT_PATH_MAX) == NULL) {
        goto error;
    }
    if (fchdir(cwdfd) == -1) {
        goto error;
    }
    (void)close(cwdfd);

    snprintf(resolved, FAKECHROOT_PATH_MAX, "%s/%s", cwd, name);
    de_dotdot(resolved);
    debug("frel2abs(%d, \"%s\", \"%s\")", dirfd, name, resolved);
    return resolved;

error:
    if (cwdfd) {
        (void)close(cwdfd);
    }
    resolved = NULL;
    debug("frel2abs(%d, \"%s\", NULL)", dirfd, name);
    return resolved;
}

#else
typedef int empty_translation_unit;
#endif
