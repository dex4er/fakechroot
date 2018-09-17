#include "unionfs.h"
#include "memcached_client.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "log.h"

DIR * getDirents(const char* name, struct dirent_obj** darr,size_t *num)
{
    if (!real_opendir) {
        real_opendir = (OPENDIR)dlsym(RTLD_NEXT, "opendir"); }
    if (!real_readdir) {
        real_readdir = (READDIR)dlsym(RTLD_NEXT, "readdir");
    }

    DIR* dirp = real_opendir(name);
    struct dirent* entry = NULL;
    struct dirent_obj* curr = NULL;
    *darr = NULL;
    *num = 0;
    while (entry = real_readdir(dirp)) {
        struct dirent_obj* tmp = (struct dirent_obj*)malloc(sizeof(struct dirent_obj));
        tmp->dp = entry;
        tmp->next = NULL;
        if (*darr == NULL) {
            *darr = curr = tmp;
        } else {
            curr->next = tmp;
            curr = tmp;
        }
        (*num)++;
    }
    return dirp;
}

void filterMemDirents(const char* name, struct dirent_obj* darr, size_t num)
{
    struct dirent_obj* curr = darr;
    char** keys = (char**)malloc(sizeof(char*) * num);
    size_t* key_lengths = (size_t*)malloc(sizeof(size_t) * num);
    for (int i = 0; i < num; i++) {
        keys[i] = (char*)malloc(sizeof(char) * MAX_PATH);
        strcpy(keys[i], curr->dp->d_name);
        key_lengths[i] = strlen(curr->dp->d_name);
        curr = curr->next;
    }
    char** values = (char**)malloc(sizeof(char*) * num);
    for (int i = 0; i < num; i++) {
        values[i] = (char*)malloc(sizeof(char) * MAX_PATH);
    }
    getMultipleValues(keys, key_lengths, num, values);
    //delete item in chains at specific pos
    for (int i = 0; i < num; i++) {
        if (values[i] != NULL &&  strlen(*(values[i])) != 0) {
            log_debug("delete dirent according to query on memcached value: %s, name is: %s",*(values[i]),keys[i]);
            deleteItemInChain(darr, i);
        }
    }
}

void deleteItemInChain(struct dirent_obj* darr, size_t num)
{
    size_t i = 0;
    struct dirent_obj *curr, *prew = darr;
    if (darr == NULL) {
        return;
    }
    //delete header
    if (num == 0) {
        curr = curr->next;
        free(prew);
        darr = curr;
        return;
    }
    for (int i= 0;i < num; i++){
        if (curr == NULL) {
            break;
        }
        prew = curr;
        curr = curr->next;
    }
    if (curr) {
        prew->next = curr->next;
        free(curr);
    }
}

void addItemToHead(struct dirent_obj* darr, struct dirent* item)
{
    if (darr == NULL || item == NULL) {
        return;
    }
    struct dirent_obj* curr = (struct dirent_obj*)malloc(sizeof(struct dirent_obj));
    curr->dp = item;
    curr->next = darr;
    darr = curr;
}

struct dirent * popItemFromHead(struct dirent_obj * darr){
    if(darr == NULL){
        return NULL;
    }
    struct dirent_obj *curr = darr;
    if(curr != NULL){
        darr = curr->next;
        return curr->dp;
    }
    return NULL;
}

void clearItems(struct dirent_obj* darr)
{
    if (darr == NULL) {
        return;
    }
    while (darr != NULL) {
        struct dirent_obj* next = darr->next;
        free(darr);
        darr = next;
    }
}
