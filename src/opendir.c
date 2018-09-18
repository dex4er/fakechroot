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

#if !defined(OPENDIR_CALLS___OPEN) && !defined(OPENDIR_CALLS___OPENDIR2)

#include <dirent.h>
#include "libfakechroot.h"
#include "unionfs.h"

struct dirent_obj* darr = NULL;
wrapper(opendir, DIR *, (const char * name))
{
    debug("opendir(\"%s\")", name);
    expand_chroot_path(name);
    size_t num;
    DIR * dirp = getDirents(name, &darr,&num);
    struct dirent * item = (struct dirent*)malloc(sizeof(struct dirent));
    strcpy(item->d_name,"hello world");
    addItemToHead(&darr,item);
    //filterMemDirents(name,darr,num);
    return dirp;
    //return nextcall(opendir)(name);
}

#else
typedef int empty_translation_unit;
#endif
