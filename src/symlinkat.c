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

#ifdef HAVE_SYMLINKAT

#define _ATFILE_SOURCE
#include "libfakechroot.h"
#include "unionfs.h"
#include "dedotdot.h"
#include "getcwd_real.h"

wrapper(symlinkat, int, (const char * oldpath, int newdirfd, const char * newpath))
{
    //char tmp[FAKECHROOT_PATH_MAX];
    //expand_chroot_rel_path(oldpath);
    //strcpy(tmp, oldpath);
    //oldpath = tmp;

    char old_resolved[MAX_PATH];
    if(*oldpath == '/'){
        expand_chroot_path(oldpath);
        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        int ret = get_relative_path_layer(oldpath, rel_path, layer_path);
        if(ret == 0){
            char abs_oldpath[MAX_PATH];
            sprintf(abs_oldpath,"/%s", rel_path);
            if(!lxstat(abs_oldpath)){
                strcpy(old_resolved, abs_oldpath);
            }else{
                strcpy(old_resolved, oldpath);
            }
        }else{
            if(!lxstat(oldpath)){
                strcpy(old_resolved, oldpath);
            }else{
                const char * container_root = getenv("ContainerRoot");
                sprintf(old_resolved, "%s%s", container_root, oldpath);
            }
        }
    }else{
        strcpy(old_resolved, oldpath);
    }
    dedotdot(old_resolved);

    char new_resolved[MAX_PATH];
    if(*newpath == '/'){
        expand_chroot_path(newpath);
        strcpy(new_resolved, newpath);
    }else{
        char cwd[MAX_PATH];
        getcwd_real(cwd,MAX_PATH);
        sprintf(new_resolved, "%s/%s", cwd, newpath);
    }
    dedotdot(new_resolved);

    debug("symlinkat oldpath: %s, newpath: %s, newdirfd: %d", old_resolved, new_resolved, newdirfd);

    char** rt_paths = NULL;
    bool r = rt_mem_check(2, rt_paths, old_resolved, new_resolved);
    if (r && rt_paths){
        return WRAPPER_FUFS(symlink, symlinkat, rt_paths[0], newdirfd, rt_paths[1])
    }else if(r && !rt_paths){
        return WRAPPER_FUFS(symlink, symlinkat, old_resolved, newdirfd, new_resolved)
    }else{
        errno = EACCES;
        return -1;
    }
}

#else
typedef int empty_translation_unit;
#endif
