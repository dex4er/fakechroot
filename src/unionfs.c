/**
 *
 *unionfs function is implemented by Xu Yang. We didin't plan to design a complete union file system such as aufs, overlay2 and etc...
 *We expect to modify the paramters of system calls when file/folders modification/creation/deletion file system manipulation happens
 *
 */
#include "unionfs.h"
#include <string.h>
#include <unistd.h>
#include "rel2abs.h"
#include "log.h"
#include <errno.h>
#include <libgen.h>

int create_hardlink(const char * src){
    const char * workspace = getenv("CONTAINER_WORKSPACE");
    if(!workspace){
        log_fatal("can't get env virable 'CONTAINER_WORKSPACE',retreat");
        return -1;
    }
    char resolved[PATH_MAX_LENGTH];
    rel2abs(src,resolved);
    narrow_chroot_path(resolved);

    char new_location[PATH_MAX_LENGTH];
    sprintf(new_location,"%s/%s",workspace,resolved);
    int ret = link(src,new_location);
    if (ret == -1){
        log_fatal("link from %s to %s failed with errno: %d",src,new_location,errno);
        return errno;
    }
    return 0;
}

int get_all_parents(const char * path, char ** parents, int * lengths, size_t *n){
    char resolved[PATH_MAX_LENGTH];
    if(*path != '/'){
        rel2abs(path, resolved);
    }else{
        strcpy(resolved,path);
    }
    size_t i = 0;
    while(resolved != '/'){
        dirname(resolved);
        parents[i] = strdup(resolved);
        lengths[i] = strlen(resolved);
        i++;
    }
    *n = i;
    return 0;
}

bool b_parent_delete(int n, ...){
    log_debug("file");
    va_list args;
    va_start(args, n);
    char* paths[n];
    for(int i=0;i<n;i++){
        paths[i] = va_arg(args,char *);
    }
    va_end(args);

    for(int i=0;i<n;i++){
        char ** parents;
        int * lengths;
        size_t n;
        get_all_parents(paths[i], parents, lengths, &n);
        if(existKeys(parents, lengths, n)){
            return false;
        }
    }
    return true;
}


