#include "unionfs.h"
#include "crypt.h"
#include "log.h"
#include "memcached_client.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

DIR* getDirents(const char* name, struct dirent_obj** darr, size_t* num)
{
    if (!real_opendir) {
        real_opendir = (OPENDIR)dlsym(RTLD_NEXT, "opendir");
    }
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

DIR* getDirentsWithName(const char* name, struct dirent_obj** darr, size_t* num, char** names)
{
    if (!real_opendir) {
        real_opendir = (OPENDIR)dlsym(RTLD_NEXT, "opendir");
    }
    if (!real_readdir) {
        real_readdir = (READDIR)dlsym(RTLD_NEXT, "readdir");
    }

    DIR* dirp = real_opendir(name);
    struct dirent* entry = NULL;
    struct dirent_obj* curr = NULL;
    *names = (char *)malloc(MAX_VALUE_SIZE);
    *darr = NULL;
    *num = 0;
    while (entry = real_readdir(dirp)) {
        struct dirent_obj* tmp = (struct dirent_obj*)malloc(sizeof(struct dirent_obj));
        strcat(*names,entry->d_name); 
        strcat(*names,";");
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
        if (values[i] != NULL && strlen(values[i]) != 0) {
            log_debug("delete dirent according to query on memcached value: %s, name is: %s", values[i], keys[i]);
            deleteItemInChain(&darr, i);
        }
    }
}

void deleteItemInChain(struct dirent_obj** darr, size_t num)
{
    size_t i = 0;
    struct dirent_obj *curr, *prew = *darr;
    if (*darr == NULL) {
        return;
    }
    //delete header
    if (num == 0) {
        curr = curr->next;
        free(prew);
        *darr = curr;
        return;
    }
    for (int i = 0; i < num; i++) {
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

void addItemToHead(struct dirent_obj** darr, struct dirent* item)
{
    if (*darr == NULL || item == NULL) {
        return;
    }
    struct dirent_obj* curr = (struct dirent_obj*)malloc(sizeof(struct dirent_obj));
    curr->dp = item;
    curr->next = *darr;
    *darr = curr;
}

struct dirent* popItemFromHead(struct dirent_obj** darr)
{
    if (*darr == NULL) {
        return NULL;
    }
    struct dirent_obj* curr = *darr;
    if (curr != NULL) {
        *darr = curr->next;
        return curr->dp;
    }
    return NULL;
}

void clearItems(struct dirent_obj** darr)
{
    if (*darr == NULL) {
        return;
    }
    while (*darr != NULL) {
        struct dirent_obj* next = (*darr)->next;
        free(*darr);
        *darr = next;
    }
    darr = NULL;
}

char* struct2hash(void* pointer, enum hash_type type)
{
    if (!pointer) {
        return NULL;
    }
    unsigned char ubytes[16];
    char salt[20];
    const char* const salts = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    //retrieve 16 unpredicable bytes form the os
    if (getentropy(ubytes, sizeof ubytes)) {
        log_fatal("can't retrieve random bytes from os");
        return NULL;
    }
    salt[0] = '$';
    if (type == md5) {
        salt[1] = '1';
    } else if (type == sha256) {
        salt[1] = '5';
    } else {
        log_fatal("hash type error, it should be either 'md5' or 'sha256'");
        return NULL;
    }
    salt[2] = '$';
    for (int i = 0; i < 16; i++) {
        salt[3 + i] = salts[ubytes[i] & 0x3f];
    }
    salt[19] = '\0';

    char* hash = crypt((char*)pointer, salt);
    if (!hash || hash[0] == '*') {
        log_fatal("can't hash the struct");
        return NULL;
    }
    if (type == md5) {
        log_debug("md5 %s", hash);
        char* value = (char*)malloc(sizeof(char) * 23);
        strcpy(value, hash + 12);
        return value;
    } else if (type == sha256) {
        log_debug("sha256 %s", hash);
        char* value = (char*)malloc(sizeof(char) * 44);
        strcpy(value, hash + 20);
        return value;
    } else {
        return NULL;
    }
    return NULL;
}
