/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2013-2015 Piotr Roszatycki <dexter@debian.org>

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
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "libfakechroot.h"
#include "strlcpy.h"
#include "dedotdot.h"
#include "open.h"


LOCAL char * rel2absat(int dirfd, const char * name, char * resolved)
{
    int cwdfd = 0;
    char cwd[FAKECHROOT_PATH_MAX - 1];

    debug("rel2absat(%d, \"%s\", &resolved)", dirfd, name);

    if (name == NULL) {
        resolved = NULL;
        goto end;
    }

    if (*name == '\0') {
        *resolved = '\0';
        goto end;
    }

    if (*name == '/') {
        strlcpy(resolved, name, FAKECHROOT_PATH_MAX);
    } else if(dirfd == AT_FDCWD) {
        if (! getcwd(cwd, FAKECHROOT_PATH_MAX - 1)) {
            goto error;
        }
        snprintf(resolved, FAKECHROOT_PATH_MAX, "%s/%s", cwd, name);
    } else {
        if ((cwdfd = nextcall(open)(".", O_RDONLY|O_DIRECTORY)) == -1) {
            goto error;
        }

        if (fchdir(dirfd) == -1) {
            goto error;
        }
        if (! getcwd(cwd, FAKECHROOT_PATH_MAX - 1)) {
            goto error;
        }
        if (fchdir(cwdfd) == -1) {
            goto error;
        }
        (void)close(cwdfd);

        snprintf(resolved, FAKECHROOT_PATH_MAX, "%s/%s", cwd, name);
    }

    dedotdot(resolved);

end:
    debug("rel2absat(%d, \"%s\", \"%s\")", dirfd, name, resolved);
    return resolved;

error:
    if (cwdfd) {
        (void)close(cwdfd);
    }
    resolved = NULL;
    debug("rel2absat(%d, \"%s\", NULL)", dirfd, name);
    return resolved;
}

#else
typedef int empty_translation_unit;
#endif
