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

#ifdef HAVE_GET_CURRENT_DIR_NAME

#include "libfakechroot.h"


wrapper(get_current_dir_name, char *, (void))
{
    char *cwd, *oldptr, *newptr;

    debug("get_current_dir_name()");
    if ((cwd = nextcall(get_current_dir_name)()) == NULL) {
        return NULL;
    }
    oldptr = cwd;
    narrow_chroot_path(cwd);
    if (cwd == NULL) {
        return NULL;
    }
    if ((newptr = malloc(strlen(cwd)+1)) == NULL) {
        free(oldptr);
        return NULL;
    }
    strcpy(newptr, cwd);
    free(oldptr);
    return newptr;
}

#else
typedef int empty_translation_unit;
#endif
