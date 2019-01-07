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

#ifdef HAVE_READLINKAT

#define _ATFILE_SOURCE
#include <sys/types.h>
#include <stddef.h>
#include "libfakechroot.h"
#include "unionfs.h"
#include <libgen.h>
#include "dedotdot.h"

wrapper(readlinkat, ssize_t, (int dirfd, const char * path, char * buf, size_t bufsiz))
{
    int linksize;
    char tmp[MAX_PATH];

    debug("readlinkat(%d, \"%s\", &buf, %zd)", dirfd, path, bufsiz);
    expand_chroot_path_at(dirfd, path);

    const char *proc_exclude_path = getenv("FAKECHROOT_EXCLUDE_PROC_PATH");
    if(proc_exclude_path){
        char proc_exclude_path_dup[MAX_PATH];
        strcpy(proc_exclude_path_dup, proc_exclude_path);
        char *proc_tmp = strtok(proc_exclude_path_dup,":");
        while(proc_tmp != NULL){
            if(strncmp(proc_tmp,path,strlen(proc_tmp)) == 0){
                linksize = 1;
                strcpy(buf,"/");
                return linksize;
            }
            proc_tmp = strtok(NULL,":");
        }
    }

    if ((linksize = nextcall(readlinkat)(dirfd, path, tmp, MAX_PATH-1)) == -1) {
        return -1;
    }
    tmp[linksize] = '\0';

    if(*tmp == '/'){
        char resolved[MAX_PATH];
        if(pathExcluded(tmp)){
            strncpy(buf, tmp, linksize);
        }else if(findFileInLayers(tmp,resolved)){
            char resolved_narrow[MAX_PATH];
            narrow_path(resolved, resolved_narrow);
            linksize = strlen(resolved_narrow);
            if(linksize > bufsiz){
                linksize = bufsiz;
            }
            strncpy(buf,resolved_narrow,linksize);
        }else{
            const char *container_root = getenv("ContainerRoot");
            char resolved_tmp[MAX_PATH];
            char resolved_narrow[MAX_PATH];
            if(container_root){
                if(*tmp == '/'){
                    sprintf(resolved_tmp, "%s%s",container_root, tmp);
                }
                narrow_path(resolved_tmp, resolved_narrow);
                linksize = strlen(resolved_narrow);
                if(linksize > bufsiz){
                    linksize = bufsiz;
                }
                strncpy(buf, resolved_narrow, linksize);
            }else{
                errno = ENOENT;
                return -1;
            }
        }
        buf[linksize] = '\0';
    }else{
        strcpy(buf, tmp);
    }

    debug("readlinkat resolved: %s", buf);
    return linksize;
}
/**
  wrapper(readlinkat, ssize_t, (int dirfd, const char * path, char * buf, size_t bufsiz))
  {
  int linksize;
  char tmp[FAKECHROOT_PATH_MAX], *tmpptr;
  const char *fakechroot_base = getenv("FAKECHROOT_BASE");

  debug("readlinkat(%d, \"%s\", &buf, %zd)", dirfd, path, bufsiz);
  expand_chroot_path_at(dirfd, path);

  if ((linksize = nextcall(readlinkat)(dirfd, path, tmp, FAKECHROOT_PATH_MAX-1)) == -1) {
  return -1;
  }
  tmp[linksize] = '\0';

  if (fakechroot_base != NULL) {
  tmpptr = strstr(tmp, fakechroot_base);
  if (tmpptr != tmp) {
  tmpptr = tmp;
  }
  else if (tmp[strlen(fakechroot_base)] == '\0') {
  tmpptr = "/";
  linksize = strlen(tmpptr);
  }
  else if (tmp[strlen(fakechroot_base)] == '/') {
  tmpptr = tmp + strlen(fakechroot_base);
  linksize -= strlen(fakechroot_base);
  }
  else {
  tmpptr = tmp;
  }
  if (strlen(tmpptr) > bufsiz) {
  linksize = bufsiz;
  }
  strncpy(buf, tmpptr, linksize);
  }
  else {
  strncpy(buf, tmp, linksize);
  }
  return linksize;
  }
 **/
#else
typedef int empty_translation_unit;
#endif
