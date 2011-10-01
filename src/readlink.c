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

#include <stddef.h>
#include "libfakechroot.h"


wrapper(readlink, READLINK_TYPE_RETURN, (const char * path, char * buf, READLINK_TYPE_ARG3(bufsiz)))
{
    int status;
    char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("readlink(\"%s\", &buf, %zd)", path, bufsiz);
    expand_chroot_path(path, fakechroot_path, fakechroot_buf);

    if ((status = nextcall(readlink)(path, tmp, FAKECHROOT_PATH_MAX-1)) == -1) {
        return -1;
    }
    tmp[status] = '\0';

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
        fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
            tmpptr = tmp;
        } else {
            tmpptr = tmp + strlen(fakechroot_path);
            status -= strlen(fakechroot_path);
        }
        if (strlen(tmpptr) > bufsiz) {
            status = bufsiz;
        }
        strncpy(buf, tmpptr, status);
    } else {
        strncpy(buf, tmp, status);
    }
    return status;
}
