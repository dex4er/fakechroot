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

#ifndef HAVE___LXSTAT

#include <sys/stat.h>
#include <unistd.h>
#include "libfakechroot.h"


wrapper(lstat, int, (const char * file_name, struct stat * buf))
{
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX], tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN status;
    const char *orig;

    debug("lstat(\"%s\", &buf)", file_name);
    orig = file_name;
    expand_chroot_path(file_name, fakechroot_path, fakechroot_buf);
    retval = nextcall(lstat)(file_name, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((buf->st_mode & S_IFMT) == S_IFLNK)
        if ((status = readlink(orig, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = status;
    return retval;
}

#endif
