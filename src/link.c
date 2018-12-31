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
#include "log.h"
#include "unionfs.h"
#include "dedotdot.h"
#include <libgen.h>

/**
  wrapper(link, int, (const char *oldpath, const char *newpath))
  {
  char tmp[FAKECHROOT_PATH_MAX];
  debug("link(\"%s\", \"%s\")", oldpath, newpath);
  expand_chroot_path(oldpath);
  strcpy(tmp, oldpath);
  oldpath = tmp;
  expand_chroot_path(newpath);

  char** rt_paths = NULL;
  bool r = rt_mem_check(2, rt_paths, oldpath, newpath);
  if (r && rt_paths){
  return WRAPPER_FUFS(link,link,rt_paths[0],rt_paths[1])
  }else if(r && !rt_paths){
  return WRAPPER_FUFS(link,link,oldpath,newpath)
  }else {
  errno = EACCES;
  return -1;
  }
  }
 **/

wrapper(link, int, (const char *oldpath, const char *newpath))
{
    char old_resolved[MAX_PATH];
    const char * container_root = getenv("ContainerRoot");
    if(*oldpath == '/'){
        expand_chroot_path(oldpath);
        char oldpath_dup[MAX_PATH];
        strcpy(oldpath_dup, oldpath);

        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        int ret = get_relative_path_layer(oldpath, rel_path, layer_path);
        if(ret == 0){
            //in other layers
            if(strcmp(layer_path, container_root) != 0){
                if(copyFile2RW(oldpath_dup, old_resolved)){
                    //fake deleting old one

                    char * bname = basename(rel_path);
                    char dname[MAX_PATH];
                    strcpy(dname, rel_path);
                    dirname(dname);
                    char whpath[MAX_PATH];
                    if(strcmp(dname, ".") == 0){
                        sprintf(whpath,"%s/.wh.%s",container_root,bname);
                    }else{
                        sprintf(whpath,"%s/%s/.wh.%s",container_root,dname,bname);
                    }
                    if(!xstat(whpath)){
                        INITIAL_SYS(creat)
                            int fd = real_creat(whpath,FILE_PERM);
                        if(fd < 0){
                            debug("can't create .wh file, %s", whpath);
                            return -1;
                        }
                        close(fd);
                    }

                }else{
                    debug("can't copy file %s", oldpath_dup);
                    errno = EACCES;
                    return -1;
                }
            }else{
                strcpy(old_resolved, oldpath_dup);
            }
        }else{
            if(copyFile2RW(oldpath_dup, old_resolved)){
                char whpath[MAX_PATH];
                char * bname = basename(old_resolved);
                char dname[MAX_PATH];
                strcpy(dname, old_resolved);
                dirname(dname);
                sprintf(whpath,"%s/.wh.%s", dname, bname);
                if(!xstat(whpath)){
                    INITIAL_SYS(creat)
                        int fd = real_creat(whpath,FILE_PERM);
                    if(fd < 0){
                        debug("can't create .wh file, %s", whpath);
                        return -1;
                    }
                    close(fd);
                }
            }else{
                debug("can't copy file %s", oldpath_dup);
                errno = EACCES;
                return -1;
            }
        }
    }else{
        strcpy(old_resolved, oldpath);
    }
    dedotdot(old_resolved);

    expand_chroot_path(newpath);

    debug("link oldpath: %s, newpath: %s", old_resolved, newpath);
    char** rt_paths = NULL;
    bool r = rt_mem_check(2, rt_paths, old_resolved, newpath);
    if (r && rt_paths){
        return WRAPPER_FUFS(link,link,rt_paths[0],rt_paths[1])
    }else if(r && !rt_paths){
        return WRAPPER_FUFS(link,link,old_resolved,newpath)
    }else {
        errno = EACCES;
        return -1;
    }
}
