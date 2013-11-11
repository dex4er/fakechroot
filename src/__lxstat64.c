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

#ifdef HAVE___LXSTAT64

#define _LARGEFILE64_SOURCE
#define _XOPEN_SOURCE
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "libfakechroot.h"
#include "readlink.h"


wrapper(__lxstat64, int, (int ver, const char * filename, struct stat64 * buf))
{
    char tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN linksize;
    const char *orig_filename;

    debug("__lxstat64(%d, \"%s\", &buf)", ver, filename);
    orig_filename = filename;
    expand_chroot_path(filename);
    retval = nextcall(__lxstat64)(ver, filename, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((buf->st_mode & S_IFMT) == S_IFLNK)
        if ((linksize = readlink(orig_filename, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = linksize;

    return retval;
}


#else
typedef int empty_translation_unit;
#endif
