/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013, 2015 Piotr Roszatycki <dexter@debian.org>

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
#define _XOPEN_SOURCE 500
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "libfakechroot.h"
#include "readlink.h"


LOCAL int __lxstat64_rel(int, const char *, struct stat64 *);


wrapper(__lxstat64, int, (int ver, const char * filename, struct stat64 * buf))
{
    debug("__lxstat64(%d, \"%s\", &buf)", ver, filename);

    if (filename && !fakechroot_localdir(filename)) {
        char abs_filename[FAKECHROOT_PATH_MAX];
        rel2abs(filename, abs_filename);
        filename = abs_filename;
    }

    return __lxstat64_rel(ver, filename, buf);
}


/* Prevent looping with realpath() */
LOCAL int __lxstat64_rel(int ver, const char * filename, struct stat64 * buf)
{
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    char tmp[FAKECHROOT_PATH_MAX];
    int retval;
    READLINK_TYPE_RETURN linksize;
    const char *orig_filename;

    debug("__lxstat64_rel(%d, \"%s\", &buf)", ver, filename);
    orig_filename = filename;
    expand_chroot_rel_path(filename);
    retval = nextcall(__lxstat64)(ver, filename, buf);
    /* deal with http://bugs.debian.org/561991 */
    if ((retval == 0) && (buf->st_mode & S_IFMT) == S_IFLNK)
        if ((linksize = readlink(orig_filename, tmp, sizeof(tmp)-1)) != -1)
            buf->st_size = linksize;

    return retval;
}


#else
typedef int empty_translation_unit;
#endif
