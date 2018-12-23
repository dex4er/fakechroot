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

#include "libfakechroot.h"
#include "unionfs.h"
#include <dirent.h>

wrapper(opendir, DIR*, (const char* name))
{
    expand_chroot_path(name);
    debug("opendir(\"%s\")", name);
    if(!xstat(name) || !is_file_type(name,TYPE_DIR)){
        errno = ENOENT;
        return NULL;
    }
    size_t num;
    struct dirent_obj* tmp = NULL;
    DIR* dirp = getDirents(name, &tmp, &num);
    if(pathExcluded(name)){
        darr = tmp;
        return dirp;
    }
    darr = WRAPPER_FUFS(opendir,opendir,name)
   /**
    while(darr){
        debug("dirents item: %s", darr->abs_path);
        darr = darr->next;
    }
    **/
    return dirp;
}

#else
typedef int empty_translation_unit;
#endif
