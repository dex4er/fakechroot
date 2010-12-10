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

#if defined(HAVE_FTS_CHILDREN) && !defined(HAVE___OPENDIR2)

#include <fts.h>
#include "libfakechroot.h"


wrapper(fts_children, FTSENT *, (FTS * ftsp, int options))
{
    FTSENT *entry, *newentry;
    char *fakechroot_path, *fakechroot_ptr;

    debug("fts_children(&ftsp, %d)", options);
    if ((entry = nextcall(fts_children)(ftsp, options)) == NULL)
        return NULL;

    /* Memory leak. We can't free this resource. */
    if ((newentry = malloc(sizeof(FTSENT))) == NULL)
        return NULL;

    memmove(newentry, entry, sizeof(FTSENT));

    if ((newentry->fts_path = malloc(entry->fts_pathlen + 1)) == NULL)
        return NULL;

    memmove(newentry->fts_path, entry->fts_path, entry->fts_pathlen + 1);
    narrow_chroot_path(newentry->fts_path, fakechroot_path, fakechroot_ptr);

    return newentry;
}

#endif
