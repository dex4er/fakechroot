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

#include "libfakechroot.h"
#include "unionfs.h"

wrapper(unlink, int, (const char * pathname))
{
    debug("unlink(\"%s\")", pathname);
    expand_chroot_path(pathname);

    char** rt_paths = NULL;
    bool r = rt_mem_check(1, rt_paths, pathname);
    if (r && rt_paths){
        return WRAPPER_FUFS(unlink,unlink,rt_paths[0])
    }else if(r && !rt_paths){
        return WRAPPER_FUFS(unlink,unlink,pathname)
    }else {
        errno = EACCES;
        return -1;
    }
}
