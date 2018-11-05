/*
   libfakechroot -- fake chroot environment
   Copyright (c) 2013 Piotr Roszatycki <dexter@debian.org>

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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "libfakechroot.h"
#include "strlcpy.h"
#include "dedotdot.h"
#include "getcwd_real.h"
#include "unionfs.h"

LOCAL char * rel2abs(const char * name, char * resolved)
{
    char cwd[FAKECHROOT_PATH_MAX];

    if (name == NULL) {
        resolved = NULL;
        goto end;
    }

    if (*name == '\0') {
        *resolved = '\0';
        goto end;
    }

    getcwd_real(cwd, FAKECHROOT_PATH_MAX); narrow_chroot_path(cwd);

    if (*name == '/') {
        strlcpy(resolved, name, FAKECHROOT_PATH_MAX);
    }
    else {
        snprintf(resolved, FAKECHROOT_PATH_MAX, "%s/%s", cwd, name);
    }

    dedotdot(resolved);


end:
    debug("rel2abs(\"%s\", \"%s\")", name, resolved);
    return resolved;
}

LOCAL char * rel2absLayer(const char * name, char * resolved){
    char cwd[FAKECHROOT_PATH_MAX];

    if (name == NULL){
        resolved = NULL;
        goto end;
    }
    if (*name == '\0') {
        *resolved = '\0';
        goto end;
    }

    getcwd_real(cwd, FAKECHROOT_PATH_MAX);
    narrow_chroot_path(cwd);

    debug("rel2absLayer cwd: %s, name: %s", cwd, name);

    if (*name == '/') {
        strlcpy(resolved, name, FAKECHROOT_PATH_MAX);
    }
    else {
        char tmp[FAKECHROOT_PATH_MAX];
        sprintf(tmp,"%s/%s",cwd,name);

        char rel_path[FAKECHROOT_PATH_MAX];
        char layer_path[FAKECHROOT_PATH_MAX];
        //test if the path exists in container layers
        int ret = get_relative_path_layer(tmp, rel_path, layer_path);
        debug("rel2absLayer target: %s, rel_path: %s, layer_path: %s", tmp,rel_path,layer_path);
        bool b_resolved = false;
        if(ret == 0){
            //exists?
            if(xstat(tmp)){
                if(!getParentWh(tmp)){
                    b_resolved = true;
                    snprintf(resolved,FAKECHROOT_PATH_MAX,"%s",tmp);
                }
                goto end;
            }else{
                //loop to find in each layer
                char ** paths;
                size_t num;
                paths = getLayerPaths(&num);
                if(num > 0){
                    for(size_t i = 0; i< num; i++){
                        char tmp[FAKECHROOT_PATH_MAX];
                        sprintf(tmp, "%s/%s", paths[i], rel_path);
                        struct stat st;
                        if(stat(tmp, &st) == -1){
                            debug("rel2absLayer failed resolved: %s",tmp);
                            if(getParentWh(tmp)){
                                break;
                            }
                            continue;
                        }else{
                            debug("rel2absLayer successfully resolved: %s",tmp);
                            snprintf(resolved,FAKECHROOT_PATH_MAX,"%s",tmp);
                            b_resolved = true;
                            break;
                        }

                    }
                }
                if(!b_resolved){
                    const char * container_root = getenv("ContainerRoot");
                    snprintf(resolved, FAKECHROOT_PATH_MAX,"%s/%s",container_root,rel_path);
                }
            }
        }else{
            snprintf(resolved, FAKECHROOT_PATH_MAX,"%s/%s",cwd,name);
        }
    }
    dedotdot(resolved);

end:
    dedotdot(resolved);
    debug("rel2absLayer ends(\"%s\", \"%s\")", name, resolved);
    return resolved;
}
