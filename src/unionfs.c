/**
 *
 *unionfs function is implemented by Xu Yang. We didin't plan to design a complete union file system such as aufs, overlay2 and etc...
 *We expect to modify the paramters of system calls when file/folders modification/creation/deletion file system manipulation happens
 *
 */
#define _GNU_SOURCE
#include "unionfs.h"
#include <string.h>
#include <unistd.h>
#include "rel2abs.h"
#include "log.h"

int create_hardlink(const char * src){
    const char * workspace = getenv("CONTAINER_WORKSPACE");
    if(!workspace){
        log_fatal("can't get env virable 'CONTAINER_WORKSPACE',retreat");
        return -1;
    }
    char resolved[FAKECHROOT_PATH_MAX];
    rel2abs(src,resolved);
    narrow_chroot_path(resolved);

    char new_location[FAKECHROOT_PATH_MAX];
    sprintf(new_location,"%s/%s",workspace,resolved);
    int ret = link(src,new_location);
    if ret == -1{
        log_fatal("link from %s to %s failed with errno: %d",src,new_location,errno);
        return errno;
    }
    return 0;
}

int get_all_parents(const char * path, char ** parents, int * lengths, int *n){
    char resolved[FAKECHROOT_PATH_MAX];
    if(*path != '/'){
        rel2abs(path, resolved);
    }else{
        strcpy(resolved,path);
    }
    int i = 0;
    while(resolved != '/'){
        dirname(resolved);
        parents[i] = strdup(resolved);
        lengths[i] = strlen(resolved);
        i++;
    }
    *n = i;
    return 0;
}
