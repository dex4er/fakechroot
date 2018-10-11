#include "unionfs.h"
#include "crypt.h"
#include "log.h"
#include "memcached_client.h"
#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "dedotdot.h"
#include <fcntl.h>

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
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        struct dirent_obj* tmp = (struct dirent_obj*)malloc(sizeof(struct dirent_obj));
        tmp->dp = entry;
        tmp->next = NULL;
        if(name[strlen(name)-1] == '/'){
            sprintf(tmp->abs_path,"%s%s",name,entry->d_name);
        }else{
            sprintf(tmp->abs_path,"%s/%s",name,entry->d_name);
        }
        dedotdot(tmp->abs_path);
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
    *names = (char*)malloc(MAX_VALUE_SIZE);
    *darr = NULL;
    *num = 0;
    while (entry = real_readdir(dirp)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        struct dirent_obj* tmp = (struct dirent_obj*)malloc(sizeof(struct dirent_obj));
        strcat(*names, entry->d_name);
        strcat(*names, ";");
        tmp->dp = entry;
        tmp->next = NULL;
        if(name[strlen(name)-1] == '/'){
            sprintf(tmp->abs_path,"%s%s",name,entry->d_name);
        }else{
            sprintf(tmp->abs_path,"%s/%s",name,entry->d_name);
        }
        dedotdot(tmp->abs_path);
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

//this function will not delete the whiteout target folders/files, only hide the .wh/.op files
struct dirent_layers_entry* getDirContent(const char* abs_path)
{
    if (!abs_path || *abs_path == '\0') {
        return NULL;
    }
    struct dirent_obj* darr;
    size_t num;
    struct dirent_layers_entry* p = (struct dirent_layers_entry*)malloc(sizeof(struct dirent_layers_entry));
    getDirents(abs_path, &darr, &num);
    strcpy(p->path, abs_path);
    struct dirent_obj* curr = darr;
    p->folder_num = p->wh_masked_num = p->file_num = 0;
    p->wh_masked = (char**)malloc(sizeof(char*) * MAX_ITEMS);
    while (curr) {
        char* m_trans = (char*)malloc(sizeof(char) * MAX_PATH);
        char abs_item_path[MAX_PATH];
        sprintf(abs_item_path, "%s/%s", abs_path, curr->dp->d_name);
        if (transWh2path(curr->dp->d_name, PREFIX_WH, m_trans)) {
            p->wh_masked[p->wh_masked_num] = m_trans;
            p->wh_masked_num += 1;
            log_debug("wh file %s is added to wh_map",curr->dp->d_name);
            deleteItemInChainByPointer(&darr, &curr);
            continue;
        }
        if (is_file_type(abs_item_path, TYPE_FILE)) {
            p->file_num += 1;
        }
        if (is_file_type(abs_item_path, TYPE_DIR)) {
            p->folder_num += 1;
        }
        curr = curr->next;
    }
    p->data = darr;
    return p;
}

char ** getLayerPaths(size_t *num){
    const char * dockerbase = getenv("DockerBase");
    const char* croot = getenv("ContainerRoot");
    if(!croot){
        log_fatal("can't get container root info");
        return NULL;
    }
    if(dockerbase && strcmp(dockerbase,"TRUE") == 0){
        const char * clayers = getenv("ContainerLayers");
        if (!clayers) {
            log_fatal("can't get container layers info");
            return NULL;
        }
        char* ccroot = strdup(croot);
        ccroot = dirname(ccroot);
        *num = 0;
        char *str_tmp;
        char **paths = (char**)malloc(sizeof(char*) * MAX_LAYERS);
        char *cclayers = strdup(clayers);
        str_tmp = strtok(cclayers,":");
        while (str_tmp != NULL){
            paths[*num] = (char *)malloc(MAX_PATH);
            sprintf(paths[*num], "%s/%s", ccroot, str_tmp);
            str_tmp = strtok(NULL,":");
            (*num) ++;
        }
        return paths;
    }
    *num = 0;
    return NULL;
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

void deleteItemInChainByPointer(struct dirent_obj** darr, struct dirent_obj** curr)
{
    if (*darr == NULL || *curr == NULL) {
        return;
    }
    if (*darr == *curr) {
        *curr = (*curr)->next;
        free(*darr);
        *darr = *curr;
        return;
    }
    struct dirent_obj *p1, *p2;
    p1 = p2 = *darr;
    while (p2) {
        if (p2 == *curr) {
            p1->next = (*curr)->next;
            free(*curr);
            *curr = p1->next;
            return;
        }
        p1 = p2;
        p2 = p2->next;
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

int get_relative_path(const char* path, char* rel_path)
{
    const char* container_path = getenv("ContainerRoot");
    if (container_path) {
        if (strncmp(container_path, path, strlen(container_path)) != 0) {
            strcpy(rel_path, path);
            return -1;
        }
        if (strlen(path) == strlen(container_path)) {
            strcpy(rel_path, "");
        } else {
            strncpy(rel_path, path + strlen(container_path), strlen(path) - strlen(container_path));
            if (rel_path[strlen(rel_path) - 1] == '/') {
                rel_path[strlen(rel_path) - 1] = '\0';
            }
        }
        return 0;
    } else {
        return -1;
    }
}

int get_relative_path_layer(const char *path, char * rel_path, char * layer_path){
    size_t num;
    char ** layers = getLayerPaths(&num);
    if(layers && num>0){
        for(size_t i = 0; i<num; i++){
            char * ret = strstr(path, layers[i]);
            if(ret){
                strcpy(rel_path, path + strlen(layers[i]) + 1);
                strcpy(layer_path, layers[i]);
                for(size_t j =0;j<num;j++){
                    free(layers[j]);
                }
                return 0;
            }
        }
    }
    return -1;
}

int get_abs_path(const char* path, char* abs_path, bool force)
{
    const char* container_path = getenv("ContainerRoot");
    if (container_path) {
        if (force) {
            if (*path == '/') {
                sprintf(abs_path, "%s%s", container_path, path);
            } else {
                sprintf(abs_path, "%s/%s", container_path, path);
            }
        } else {
            if (*path == '/') {
                strcpy(abs_path, path);
            } else {
                sprintf(abs_path, "%s/%s", container_path, path);
            }
        }
        return 0;
    } else {
        log_fatal("can't get variable 'ContainerRoot'");
        return -1;
    }
}

int get_abs_path_base(const char *base, const char *path, char * abs_path, bool force){
    if (base) {
        if (force) {
            if (*path == '/') {
                sprintf(abs_path, "%s%s", base, path);
            } else {
                sprintf(abs_path, "%s/%s", base, path);
            }
        } else {
            if (*path == '/') {
                strcpy(abs_path, path);
            } else {
                sprintf(abs_path, "%s/%s", base, path);
            }
        }
        return 0;
    } else {
        log_fatal("base should not be NULL");
        return -1;
    }
}

int get_relative_path_base(const char *base, const char *path, char * rel_path){
    if (base) {
        if (strncmp(base, path, strlen(base)) != 0) {
            strcpy(rel_path, path);
            return -1;
        }
        if (strlen(path) == strlen(base)) {
            strcpy(rel_path, "");
        } else {
            strncpy(rel_path, path + strlen(base), strlen(path) - strlen(base));
            if (rel_path[strlen(rel_path) - 1] == '/') {
                rel_path[strlen(rel_path) - 1] = '\0';
            }
        }
        return 0;
    } else {
        return -1;
    }
}

int append_to_diff(const char* content)
{
    const char* docker = getenv("DockerBase");
    if (strcmp(docker, "TRUE") == 0) {
        const char* diff_path = getenv("ContainerDiff");
        if (diff_path) {
            char target_file[MAX_PATH];
            sprintf(target_file, "%s/.info", diff_path);
            FILE* pfile = fopen(target_file, "a");
            if (pfile == NULL) {
                log_fatal("can't open file %s", target_file);
                return -1;
            }
            fprintf(pfile, "%s\n", content);
            fclose(pfile);
            return 0;
        } else {
            log_debug("unable to get ContainerDiff variable while in docker_base mode");
            return -1;
        }
    }
    return 0;
}

bool is_file_type(const char* path, enum filetype t)
{
    struct stat path_stat;
    int ret = stat(path, &path_stat);
    if (ret == 0) {
        switch (t) {
            case TYPE_FILE:
                return S_ISREG(path_stat.st_mode);
            case TYPE_DIR:
                return S_ISDIR(path_stat.st_mode);
            case TYPE_LINK:
                return S_ISLNK(path_stat.st_mode);
            case TYPE_SOCK:
                return S_ISSOCK(path_stat.st_mode);
            default:
                log_fatal("filetype is not recognized");
                break;
        }
        return false;
    } else {
        log_fatal(strerror(errno));
        return false;
    }
}

bool transWh2path(const char* name, const char* pre, char* tname)
{
    size_t lenname = strlen(name);
    size_t lenpre = strlen(pre);
    bool b_contain = strncmp(pre, name, lenpre) == 0;
    if (b_contain) {
        char tmp[MAX_PATH];
        strcpy(tmp, name + lenpre);
        for (int i = 0; i < strlen(tmp); i++) {
            if (tmp[i] == '.') {
                tmp[i] = '/';
            }
        }
        if (tmp[0] == '/') {
            strcpy(tname, tmp + 1);
        } else {
            strcpy(tname, tmp);
        }
    }
    return b_contain;
}

/**
 * get any whiteout parent folders
 * if given path is as /f1/f2/f3
 * it will check if /.wh.f1 /f1/.wh.f2 /f1/f2/.wh.f3 exists
 * return: -1 error 0 not exists 1 exists
 */
int getParentWh(const char *abs_path){
    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(abs_path,rel_path, layer_path);
    if(ret == 0){
        while(1){
            char whpath[MAX_PATH];
            char * bname = basename(rel_path);
            char * dname = dirname(rel_path);
            if(strcmp(dname,".") == 0){
                sprintf(whpath, "%s/.wh.%s",layer_path, bname);
            }else{
                sprintf(whpath, "%s/%s/.wh.%s",layer_path, dname, bname);
            }
            if(xstat(whpath)){
                return 1;
            }
            if(strcmp(dname,".") == 0){
                break;
            }
        }
        return 0;
    }else{
        log_fatal("get relative path encounters error, layer path: %s, input path: %s", layer_path, abs_path);
        return -1;
    }
}

bool xstat(const char *abs_path){
    if(!real_xstat){
        real_xstat = (__XSTAT)dlsym(RTLD_NEXT, "__xstat");
    }
    struct stat st;
    if(real_xstat(1,abs_path,&st) == 0){
        return true;
    }
    return false;
}

// fake union fs functions
struct dirent_obj* fufs_opendir(const char* abs_path){
    //container layer from top to lower
    size_t num;
    char ** layers = getLayerPaths(&num);
    if(num < 1){
        log_fatal("can't find layer info");
        return NULL;
    }

    struct dirent_obj *head, *tail;
    head = tail = NULL;

    //map
    hmap_t* dirent_map = create_hmap(MAX_ITEMS);
    hmap_t* wh_map = create_hmap(MAX_ITEMS);

    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(abs_path, rel_path, layer_path);
    if (ret == -1) {
        log_fatal("%s is not inside the container", rel_path);
        return NULL;
    }

    for (int i = 0; i < num; i++) {
        char each_layer_path[MAX_PATH];
        sprintf(each_layer_path, "%s/%s", layers[i], rel_path);
        log_debug("target folder: %s", each_layer_path);
        if(strcmp(each_layer_path, abs_path) != 0){
            if(getParentWh(each_layer_path) == 1){
                break;
            }
        }

        if(xstat(each_layer_path)){
            struct dirent_layers_entry* entry = getDirContent(each_layer_path);
            if (entry->data) {
                if (head == NULL && tail == NULL) {
                    head = tail = entry->data;
                    while (tail->next != NULL) {
                        log_debug("item added to dirent_map %s", tail->dp->d_name);
                        add_item_hmap(dirent_map, tail->dp->d_name, NULL);
                        tail = tail->next;
                    }
                    add_item_hmap(dirent_map, tail->dp->d_name, NULL);
                    for (size_t wh_i = 0; wh_i < entry->wh_masked_num; wh_i++) {
                        log_debug("item added to wh_map %d , %s", entry->wh_masked_num, entry->wh_masked[wh_i]);
                        add_item_hmap(wh_map, entry->wh_masked[wh_i], NULL);
                    }
                } else {
                    //need to merge folders files and hide wh
                    struct dirent_obj* prew = tail;
                    tail->next = entry->data;
                    tail = tail->next;
                    while (tail->next != NULL) {
                        if (!contain_item_hmap(dirent_map, tail->dp->d_name) && !contain_item_hmap(wh_map, tail->dp->d_name)) {
                            log_debug("item added to dirent_map %s", tail->dp->d_name);
                            add_item_hmap(dirent_map, tail->dp->d_name, NULL);
                        } else {
                            log_debug("item deteled from dirent_map %s", tail->dp->d_name);
                            deleteItemInChainByPointer(&head, &tail);
                            if (!tail) {
                                tail = prew;
                            }
                            continue;
                        }
                        prew = tail;
                        tail = tail->next;
                    }
                    if (!contain_item_hmap(dirent_map, tail->dp->d_name) && !contain_item_hmap(wh_map, tail->dp->d_name)) {
                        log_debug("item added to dirent_map %s", tail->dp->d_name);
                        add_item_hmap(dirent_map, tail->dp->d_name, NULL);
                    } else {
                        log_debug("item deteled from dirent_map %s", tail->dp->d_name);
                        deleteItemInChainByPointer(&head, &tail);
                        if (!tail) {
                            tail = prew;
                        }
                    }
                    for (size_t wh_i = 0; wh_i < entry->wh_masked_num; wh_i++) {
                        log_debug("item added to wh_map %d , %s", entry->wh_masked_num, entry->wh_masked[wh_i]);
                        add_item_hmap(wh_map, entry->wh_masked[wh_i], NULL);
                    }
                }
            }
        }

        //find if any whiteout parent folders exists
        if(getParentWh(each_layer_path) == 1){
            break;
        }
    }

    return head;
}

int fufs_unlink(const char* abs_path){
    if(!xstat(abs_path)){
        return -1;
    }else{
        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        int ret = get_relative_path_layer(abs_path,rel_path, layer_path);
        if(ret == -1){
            log_fatal("request path is not in container, path: %s", abs_path);
            return -1;
        }
        const char * root_path = getenv("ContainerRoot");
        if(strcmp(root_path, layer_path)){
            return 1;
        }else{
            //request path is in other layers rather than rw layer
            char * bname = basename(rel_path);
            char * dname = dirname(rel_path);
            char whpath[MAX_PATH];
            if(strcmp(dname, ".") == 0){
                sprintf(whpath,"%s/.wh.%s",root_path,bname);
            }else{
                sprintf(whpath,"%s/%s",root_path,dname);
                if(!xstat(whpath)){
                    mkdir(whpath,0700);
                }
                sprintf(whpath,"%s/.wh.%s", whpath,bname);
            }
            int fp = open(whpath,O_CREAT|O_APPEND,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(fp == -1){
                log_fatal("can't create file: %s", whpath);
                return -1;
            }
            close(fp);
            return 0;
        }
    }
}

int fufs_open(const char* abs_path, int oflag, ...){
    va_list args;
    va_start(args,oflag);
    va_end(args);

    if(!real_open){
        real_open = (OPEN)dlsym(RTLD_NEXT,"open");
    }
    if(!xstat){
        return real_open(abs_path, oflag, args);
    }else{
        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        int ret = get_relative_path_layer(abs_path, rel_path, layer_path);
        if(ret == 0){
            const char * container_root = getenv("ContainerRoot");
            if(strcmp(layer_path,container_root)){
                return real_open(abs_path, oflag, args);
            }else{
                //copy and write
                FILE *src, *dest;
                char destpath[MAX_PATH];
                sprintf(destpath,"%s/%s", container_root, rel_path);
                src = fopen(abs_path, "r");
                dest = fopen(destpath, "w");
                if(src == NULL || dest == NULL){
                    log_fatal("open file encounters error, src: %s, dest: %s", abs_path, destpath);
                    return -1;
                }
                char ch = fgetc(src);
                while(ch != EOF){
                    fputc(ch,dest);
                    ch = fgetc(src);
                }
                fclose(src);
                fclose(dest);
                return real_open(destpath,oflag, args);
            }
        }else{
            log_fatal("%s file doesn't exist in container", abs_path);
            return -1;
        }
    }
}
