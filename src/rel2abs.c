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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "libfakechroot.h"
#include "strlcpy.h"
#include "dedotdot.h"
#include "getcwd_real.h"


LOCAL char * rel2abs(const char * name, char * resolved)
{
    char cwd[FAKECHROOT_PATH_MAX - 1];

    debug("rel2abs(\"%s\", &resolved)", name);

    if (name == NULL) {
        resolved = NULL;
        goto end;
    }

    if (*name == '\0') {
        *resolved = '\0';
        goto end;
    }

    getcwd_real(cwd, FAKECHROOT_PATH_MAX - 1);
    narrow_chroot_path(cwd);

    if (*name == '/') {
        strlcpy(resolved, name, FAKECHROOT_PATH_MAX);
    }
    else {
        snprintf(resolved, FAKECHROOT_PATH_MAX, "%s/%s", cwd, name);
    }

    dedotdot(resolved);

end:
    debug("rel2abs(\"%s\", \"%s\")", name, resolved);
    return resolved;
}
