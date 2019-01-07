#include "unionfs.h"
#include "crypt.h"
#include "log.h"
#include "memcached_client.h"
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
    INITIAL_SYS(opendir)
        INITIAL_SYS(readdir)

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
        sprintf(tmp->d_name,"%s",entry->d_name);
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

    INITIAL_SYS(opendir)
        INITIAL_SYS(readdir)

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
        sprintf(tmp->d_name,"%s",entry->d_name);
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

void getDirentsOnlyNames(const char* name, char ***names,size_t *num){
    INITIAL_SYS(opendir)
        INITIAL_SYS(readdir)

        DIR* dirp = real_opendir(name);
    struct dirent* entry = NULL;
    *names = (char **)malloc(sizeof(char *)*MAX_VALUE_SIZE);
    *num = 0;
    while(entry = real_readdir(dirp)){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        (*names)[*num] = strdup(entry->d_name);
        (*num)++;
    }
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
        sprintf(abs_item_path, "%s/%s", abs_path, curr->d_name);
        if (transWh2path(curr->d_name, PREFIX_WH, m_trans)) {
            p->wh_masked[p->wh_masked_num] = m_trans;
            p->wh_masked_num += 1;
            //log_debug("wh file %s is added to wh_map",curr->d_name);
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

/**
 * code is modified as all layers except rw layers are relocated
 */
char ** getLayerPaths(size_t *num){
    const char * dockerbase = getenv("DockerBase");
    const char* croot = getenv("ContainerRoot");
    if(!croot){
        log_fatal("can't get container root info");
        return NULL;
    }
    if(dockerbase && strcmp(dockerbase,"TRUE") == 0){
        const char * clayers = getenv("ContainerLayers");
        //const char * base = getenv("ContainerBasePath");
        if (!clayers) {
            log_fatal("can't get container layers info, please set env variable 'ContainerLayers'");
            return NULL;
        }
        char ccroot[MAX_PATH];
        strcpy(ccroot, croot);
        dirname(ccroot);
        *num = 0;
        char *str_tmp;
        char **paths = (char**)malloc(sizeof(char*) * MAX_LAYERS);
        char cclayers[MAX_PATH];
        strcpy(cclayers, clayers);
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
        strcpy(keys[i], curr->d_name);
        key_lengths[i] = strlen(curr->d_name);
        curr = curr->next;
    }
    char** values = (char**)malloc(sizeof(char*) * num);
    for (int i = 0; i < num; i++) {
        values[i] = (char*)malloc(sizeof(char) * MAX_PATH);
    }
    getMultipleValues((const char **)keys, key_lengths, num, values);
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

//delete item pointed by curr pointer, make it point to the next item
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

/**
  char* struct2hash(void* pointer, enum hash_type type)
  {
  if (!pointer) {
  return NULL;
  }
  unsigned char ubytes[16];
  char salt[20];
  const char* const salts = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

//retrieve 16 unpredicable bytes form the os
if (getentropy(ubytes, 16)) {
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
 **/

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
                if(strlen(path) > strlen(layers[i])){
                    strcpy(rel_path, path + strlen(layers[i]) + 1);
                }else{
                    strcpy(rel_path,".");
                }
                strcpy(layer_path, layers[i]);
                if(layers){
                    for(size_t j =0;j<num;j++){
                        free(layers[j]);
                    }
                }
                return 0;
            }

            //check other layers rather than rw one
            char * layer_name = basename(layers[i]);
            if(strcmp(layer_name, "rw") != 0){
                char newlayer[MAX_PATH];
                const char * base_path = getenv("ContainerBasePath");
                if(!base_path){
                    log_fatal("can't get variable 'ContainerBasePath'");
                    return -1;
                }
                sprintf(newlayer, "%s/%s", base_path, layer_name);

                char * ret = strstr(path, newlayer);
                if(ret){
                    if(strlen(path) > strlen(newlayer)){
                        strcpy(rel_path, path + strlen(newlayer) + 1);
                    }else{
                        strcpy(rel_path,".");
                    }
                    strcpy(layer_path, newlayer);
                    if(layers){
                        for(size_t j =0;j<num;j++){
                            free(layers[j]);
                        }
                    }
                    return 0;

                }
            }
        }
    }

    if(layers){
        for(size_t j =0;j<num;j++){
            free(layers[j]);
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

int narrow_path(const char *path, char *resolved){
    if(path && *path == '/'){
        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        int ret = get_relative_path_layer(path, rel_path, layer_path);
        if(ret == 0){
            if(strcmp(rel_path, ".") == 0){
                strcpy(resolved,"/");
            }else{
                sprintf(resolved,"/%s",rel_path);
            }
            return 0;
        }
        strcpy(resolved, path);
        return 0;
    }
    strcpy(resolved, path);
    return -1;
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

//TYPE_LINK here is symlink
bool is_file_type(const char* path, enum filetype t)
{
    INITIAL_SYS(__lxstat)
        struct stat path_stat;
    int ret = real___lxstat(1,path, &path_stat);
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
        strcpy(tname, name + lenpre + 1);
    }
    return b_contain;
}

bool findFileInLayers(const char *file,char *resolved){
    size_t num;
    char ** layers = getLayerPaths(&num);
    if(num > 0){
        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        if(*file == '/'){
            int ret = get_relative_path_layer(file, rel_path, layer_path);
            if(ret == 0){
                for(size_t i = 0; i<num; i++){
                    char tmp[MAX_PATH];
                    if(strcmp(rel_path, ".") == 0){
                        strcpy(tmp, layers[i]);
                    }else{
                        sprintf(tmp,"%s/%s", layers[i],rel_path);
                    }
                    if(getParentWh(tmp)){
                        strcpy(resolved,file);
                        return false;
                    }
                    if(xstat(tmp)){
                        strcpy(resolved,tmp);
                        return true;
                    }
                    if(strcmp(layer_path, layers[i]) == 0){
                        strcpy(resolved,file);
                        return false;
                    }
                }
            }else{
                for(size_t i = 0; i<num; i++){
                    char tmp[MAX_PATH];
                    sprintf(tmp,"%s%s",layers[i],file);
                    if(getParentWh(tmp)){
                        strcpy(resolved,file);
                        return false;
                    }
                    if(xstat(tmp)){
                        strcpy(resolved,tmp);
                        return true;
                    }
                }
            }
        }else{
            for(size_t i = 0; i<num; i++){
                char tmp[MAX_PATH];
                sprintf(tmp,"%s/%s",layers[i],file);
                if(getParentWh(tmp)){
                    strcpy(resolved,file);
                    return false;
                }
                if(xstat(tmp)){
                    strcpy(resolved,tmp);
                    return true;
                }
            }
        }
    }
    strcpy(resolved,file);
    return false;
}

bool findFileInLayersSkip(const char *file, char *resolved, size_t skip){
    size_t num;
    char ** layers = getLayerPaths(&num);
    if(num > 0 && num > skip && skip > 0){
        char rel_path[MAX_PATH];
        char layer_path[MAX_PATH];
        if(*file == '/'){
            int ret = get_relative_path_layer(file, rel_path, layer_path);
            if(ret == 0){
                for(size_t i = skip; i<num; i++){
                    char tmp[MAX_PATH];
                    if (strcmp(rel_path,".") == 0) {
                        strcpy(tmp, layers[i]);
                    } else {
                        sprintf(tmp,"%s/%s", layers[i],rel_path);
                    }
                    if(getParentWh(tmp)){
                        strcpy(resolved,file);
                        return false;
                    }
                    if(xstat(tmp)){
                        strcpy(resolved,tmp);
                        return true;
                    }
                    if(strcmp(layer_path, layers[i]) == 0){
                        strcpy(resolved,file);
                        return false;
                    }
                }
            }else{
                for(size_t i = skip; i<num; i++){
                    char tmp[MAX_PATH];
                    sprintf(tmp,"%s%s",layers[i],file);
                    if(getParentWh(tmp)){
                        strcpy(resolved,file);
                        return false;
                    }
                    if(xstat(tmp)){
                        strcpy(resolved,tmp);
                        return true;
                    }
                }
            }
        }else{
            for(size_t i = skip; i<num; i++){
                char tmp[MAX_PATH];
                sprintf(tmp,"%s/%s",layers[i],file);
                if(getParentWh(tmp)){
                    strcpy(resolved,file);
                    return false;
                }
                if(xstat(tmp)){
                    strcpy(resolved,tmp);
                    return true;
                }
            }
        }
    }
    strcpy(resolved,file);
    return false;
}

//copy file to rw layers
bool copyFile2RW(const char *abs_path, char *resolved){
    if(!xstat(abs_path)){
        strcpy(resolved,abs_path);
        return false;
    }

    if(!is_file_type(abs_path,TYPE_FILE)){
        strcpy(resolved,abs_path);
        return false;
    }

    INITIAL_SYS(fopen)
        INITIAL_SYS(mkdir)

        char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(abs_path, rel_path, layer_path);
    const char * container_root = getenv("ContainerRoot");
    if(ret == 0){
        if(strcmp(layer_path, container_root) == 0){
            strcpy(resolved, abs_path);
            return true;
        }else{
            char destpath[MAX_PATH];
            sprintf(destpath,"%s/%s", container_root, rel_path);
            FILE *src, *dest;
            src = real_fopen(abs_path, "r");

            //check if the dest path folder exists
            int mk_ret = recurMkdir(destpath);
            if(mk_ret != 0){
                log_fatal("creating dirs of path: %s encounters failure with error %s", destpath, strerror(errno));
                return false;
            }

            if(xstat(destpath)){
                //truncate and rewrite
                dest = real_fopen(destpath, "w");
            }else{
                dest = real_fopen(destpath, "w+");
            }
            if(src == NULL || dest == NULL){
                log_fatal("open file encounters error, src: %s -> %p, dest: %s -> %p", abs_path,src, destpath,dest);
                return -1;
            }
            char ch = fgetc(src);
            while(ch != EOF){
                fputc(ch,dest);
                ch = fgetc(src);
            }
            fclose(src);
            fclose(dest);
            log_debug("finished copying from %s to %s",abs_path,destpath);
            strcpy(resolved,destpath);
            return true;
        }
    }else{
        strcpy(resolved, abs_path);
        return false;
    }
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
    if(abs_path == NULL || *abs_path == '\0'){
        return false;
    }
    INITIAL_SYS(__xstat)
        struct stat st;
    if(real___xstat(1,abs_path,&st) == 0){
        return true;
    }
    return false;
}

bool lxstat(const char *abs_path){
    if(abs_path == NULL || *abs_path == '\0'){
        return false;
    }
    INITIAL_SYS(__lxstat)
        struct stat st;
    if(real___lxstat(1, abs_path, &st) == 0){
        return true;
    }
    return false;
}

int recurMkdir(const char *path){
    if(path == NULL || *path == '\0' || *path != '/'){
        log_fatal("can't make dir as the input parameter is either null, empty or not absolute path, path: %s", path);
        errno = EEXIST;
        return -1;
    }

    if(strcmp(path,"/") == 0){
        return 0;
    }

    char dname[MAX_PATH];
    strcpy(dname, path);
    dirname(dname);
    if(!xstat(dname)){
        recurMkdir(dname);
    }

    if(!xstat(dname)){
        INITIAL_SYS(mkdir)
            log_debug("start creating dir %s", dname);
        int ret = real_mkdir(dname, FOLDER_PERM);
        if(ret != 0){
            log_fatal("creating dirs %s encounters failure with error %s", dname, strerror(errno));
            return -1;
        }
    }
    return 0;
}

//path should be the folder name
int recurMkdirMode(const char *path, mode_t mode){
    if(path == NULL || *path == '\0' || *path != '/'){
        log_fatal("can't make dir as the input parameter is either null, empty or not absolute path, path: %s", path);
        errno = EEXIST;
        return -1;
    }

    if(strcmp(path,"/") == 0){
        return 0;
    }

    if(!xstat(path)){
        char dname[MAX_PATH];
        strcpy(dname, path);
        dirname(dname);
        recurMkdirMode(dname, mode);
    }

    if(!xstat(path)){
        INITIAL_SYS(mkdir)
            log_debug("start creating dir %s", path);
        int ret = real_mkdir(path, mode);
        if(ret != 0){
            log_fatal("creating dirs %s encounters failure with error %s", path, strerror(errno));
            return -1;
        }
    }
    return 0;
}

bool pathExcluded(const char *abs_path){
    if(abs_path == NULL || *abs_path == '\0'){
        return false;
    }
    if(*abs_path != '/'){
        log_error("input path should be absolute path rather than relative path: %s",abs_path);
        return false;
    }
    const char *exclude_path = getenv("FAKECHROOT_EXCLUDE_PATH");
    if(exclude_path){
        char exclude_path_dup[MAX_PATH];
        strcpy(exclude_path_dup, exclude_path);
        char *str_tmp = strtok(exclude_path_dup,":");
        while (str_tmp){
            if(strncmp(str_tmp, abs_path,strlen(str_tmp)) == 0){
                return true;
            }
            str_tmp = strtok(NULL,":");
        }
    }
    return false;
}

bool resolveSymlink(const char *link, char *target){
    if(is_file_type(link,TYPE_LINK)){
        INITIAL_SYS(readlink)
            char resolved[MAX_PATH];
        ssize_t size = real_readlink(link,resolved,MAX_PATH - 1);
        if(size == -1){
            log_fatal("can't resolve link %s",link);
            strcpy(target,link);
            return false;
        }else{
            resolved[size] = '\0';
            if(pathExcluded(resolved)){
                strcpy(target,resolved);
                return true;
            }else{
                char ** paths;
                size_t num;
                paths = getLayerPaths(&num);
                bool b_resolved = false;
                if(num > 0){
                    char tmp[MAX_PATH];
                    for(size_t i = 0; i< num; i++){
                        memset(tmp,'\0',MAX_PATH);
                        if(*resolved == '/'){
                            sprintf(tmp, "%s%s", paths[i],resolved);
                        }else{
                            sprintf(tmp, "%s/%s", paths[i],resolved);
                        }
                        if(!xstat(tmp)){
                            log_debug("symlink failed resolved: %s",tmp);
                            if(getParentWh(tmp)){
                                break;
                            }
                            continue;
                        }else{
                            log_debug("symlink successfully resolved: %s",tmp);
                            char tmp_solved[MAX_PATH];
                            snprintf(tmp_solved,MAX_PATH,"%s",tmp);
                            b_resolved = true;
                            strcpy(target,tmp_solved);
                            break;
                        }

                    }
                }
                if(!b_resolved){
                    const char * container_root = getenv("ContainerRoot");
                    char tmp[MAX_PATH];
                    if(*resolved == '/'){
                        snprintf(tmp, MAX_PATH,"%s%s",container_root,resolved);
                    }else{
                        snprintf(tmp, MAX_PATH,"%s/%s",container_root,resolved);
                    }
                    strcpy(target,tmp);
                    return false;
                }
                return true;
            }
        }
    }else{
        strcpy(target,link);
        return false;
    }
}

bool pathIncluded(const char *abs_path){
    if(abs_path == NULL || *abs_path == '\0'){
        return false;
    }
    if(*abs_path != '/'){
        log_error("input path should be absolute path rather than relative path");
        return false;
    }
    const char *include_path= getenv("FAKECHROOT_INCLUDE_PATH");
    if(include_path){
        char include_path_dup[MAX_PATH];
        strcpy(include_path_dup, include_path);
        char *str_tmp = strtok(include_path_dup,":");
        while (str_tmp){
            if(strncmp(str_tmp, abs_path,strlen(str_tmp)) == 0){
                return true;
            }
            str_tmp = strtok(NULL,":");
        }
    }
    return false;

}

/**----------------------------------------------------------------------------------**/
int fufs_open_impl(const char* function, ...){
    int dirfd = -1;
    const char *path;
    int oflag;
    mode_t mode;

    va_list args;
    va_start(args,function);
    if(strcmp(function,"openat") == 0 || strcmp(function,"openat64") == 0){
        dirfd = va_arg(args,int);
    }

    path = va_arg(args, const char *);
    oflag = va_arg(args, int);
    mode = va_arg(args, mode_t);
    va_end(args);

    INITIAL_SYS(open)
        INITIAL_SYS(openat)
        INITIAL_SYS(open64)
        INITIAL_SYS(openat64)

        //not exists or excluded directly calling real open
        if(!xstat(path) || pathExcluded(path)){
            if(oflag & O_DIRECTORY){
                goto end_folder;
            }
            goto end_file;
        }else{
            //if it exists, then copy and write
            char rel_path[MAX_PATH];
            char layer_path[MAX_PATH];
            int ret = get_relative_path_layer(path, rel_path, layer_path);
            if(ret == 0){
                const char * container_root = getenv("ContainerRoot");
                if(strcmp(layer_path,container_root) == 0){
                    if(oflag & O_DIRECTORY){
                        goto end_folder;
                    }
                    goto end_file;
                }else{
                    if(oflag & O_DIRECTORY || (xstat(path) && is_file_type(path,TYPE_DIR))){
                        goto end_folder;
                    }

                    //read only
                    if(oflag == 0){
                        goto end_file;
                    }

                    //copy and write
                    char destpath[MAX_PATH];
                    if(!copyFile2RW(path, destpath)){
                        log_fatal("copy from %s to %s encounters error", path, destpath);
                        return -1;
                    }
                    if(strcmp(function,"openat") == 0){
                        return RETURN_SYS(openat,(dirfd,destpath,oflag,mode))
                    }
                    if(strcmp(function,"open") == 0){
                        return RETURN_SYS(open,(destpath,oflag,mode))
                    }
                    if(strcmp(function,"openat64") == 0){
                        return RETURN_SYS(openat64,(dirfd,destpath,oflag,mode))
                    }
                    if(strcmp(function,"open64") == 0){
                        return RETURN_SYS(open64,(destpath,oflag,mode))
                    }
                    goto err;
                }
            }else{
                log_fatal("%s file doesn't exist in container", path);
                return -1;
            }
        }

end_folder:
    if(!xstat(path)){
        INITIAL_SYS(mkdir)
            int ret = recurMkdirMode(path,FOLDER_PERM);
        if(ret != 0){
            log_fatal("creating dirs %s encounters failure with error %s", path, strerror(errno));
            return -1;
        }
    }
    goto end;


end_file:
    if(!xstat(path)){
        INITIAL_SYS(mkdir)
            char dname[MAX_PATH];
        strcpy(dname,path);
        dirname(dname);
        int ret = recurMkdirMode(dname,FOLDER_PERM);
        if(ret != 0){
            log_fatal("creating dirs %s encounters failure with error %s", dname, strerror(errno));
            return -1;
        }
    }
    goto end;

end:
    if(strcmp(function,"openat") == 0){
        return RETURN_SYS(openat,(dirfd,path,oflag,mode))
    }
    if(strcmp(function,"open") == 0){
        return RETURN_SYS(open,(path,oflag,mode))
    }
    if(strcmp(function,"openat64") == 0){
        return RETURN_SYS(openat64,(dirfd,path,oflag,mode))
    }
    if(strcmp(function,"open64") == 0){
        return RETURN_SYS(open64,(path,oflag,mode))
    }

err:
    errno = EACCES;
    return -1;
}

FILE* fufs_fopen_impl(const char * function, ...){
    const char *path;
    const char *mode;
    FILE *stream;

    va_list args;
    va_start(args,function);
    path = va_arg(args, const char *);
    mode = va_arg(args, const char *);

    if(strcmp(function,"freopen") == 0 || strcmp(function,"freopen64") == 0){
        stream= va_arg(args,FILE *);
    }
    va_end(args);

    INITIAL_SYS(fopen)
        INITIAL_SYS(fopen64)
        INITIAL_SYS(freopen)
        INITIAL_SYS(freopen64)

        if(!xstat(path) || pathExcluded(path)){
            goto end;
        }else{
            char rel_path[MAX_PATH];
            char layer_path[MAX_PATH];
            int ret = get_relative_path_layer(path, rel_path, layer_path);
            if(ret == 0){
                const char * container_root = getenv("ContainerRoot");
                if(strcmp(layer_path,container_root) == 0){
                    goto end;
                }else{
                    //copy and write
                    if(strncmp(mode,"r",1) == 0){
                        goto end;
                    }

                    char destpath[MAX_PATH];
                    if(!copyFile2RW(path, destpath)){
                        log_fatal("copy from %s to %s encounters error", path, destpath);
                        return NULL;
                    }
                    if(strcmp(function,"fopen") == 0){
                        return RETURN_SYS(fopen,(destpath,mode))
                    }
                    if(strcmp(function,"fopen64") == 0){
                        return RETURN_SYS(fopen64,(destpath,mode))
                    }
                    if(strcmp(function,"freopen") == 0){
                        return RETURN_SYS(freopen,(destpath,mode,stream))
                    }
                    if(strcmp(function,"freopen64") == 0){
                        return RETURN_SYS(freopen64,(destpath,mode,stream))
                    }
                    goto err;
                }
            }else{
                log_fatal("%s file doesn't exist in container", path);
                return NULL;
            }

        }

end:
    if(!xstat(path)){
        INITIAL_SYS(mkdir)
            int ret = recurMkdir(path);
        if(ret != 0){
            log_fatal("creating dirs %s encounters failure with error %s", path, strerror(errno));
            return NULL;
        }
    }

    if(strcmp(function,"fopen") == 0){
        return RETURN_SYS(fopen,(path,mode))
    }
    if(strcmp(function,"fopen64") == 0){
        return RETURN_SYS(fopen64,(path,mode))
    }
    if(strcmp(function,"freopen") == 0){
        return RETURN_SYS(freopen,(path,mode,stream))
    }
    if(strcmp(function,"freopen64") == 0){
        return RETURN_SYS(freopen64,(path,mode,stream))
    }

err:
    errno = EACCES;
    return NULL;
}

int fufs_unlink_impl(const char* function,...){
    va_list args;
    va_start(args,function);
    int dirfd = -1;
    const char *abs_path;
    int oflag;

    if(strcmp(function,"unlinkat") == 0){
        dirfd = va_arg(args,int);
        abs_path = va_arg(args, const char *);
        oflag = va_arg(args, int);
    }else{
        abs_path = va_arg(args, const char *);
    }
    va_end(args);

    INITIAL_SYS(unlink)
        INITIAL_SYS(unlinkat)

        if(!lxstat(abs_path)){
            errno = ENOENT;
            return -1;
        }else if(pathExcluded(abs_path)){
            goto end;
        }else{
            //check if deleting folder with all .wh files inside?
            if(is_file_type(abs_path, TYPE_DIR)){
                char **names;
                size_t num;
                getDirentsOnlyNames(abs_path, &names,&num);
                bool is_all_wh = false;
                for(size_t i = 0;i<num;i++){
                    if(strncmp(names[i],".wh",3) == 0){
                        if(i == 0){
                            is_all_wh = true;
                        }else{
                            is_all_wh = is_all_wh & true;
                        }
                    }else{
                        is_all_wh = false;
                        break;
                    }
                }
                if(is_all_wh){
                    log_debug("all files in folder: %s are whiteout files, will entirely delete everything",abs_path);
                    char tmp[MAX_PATH];
                    for(size_t i = 0; i<num; i++){
                        sprintf(tmp,"%s/%s",abs_path,names[i]);
                        log_debug("all files in folder: %s are whiteout files, delete target item: %s",abs_path, tmp);
                        real_unlink(tmp);
                    }
                }
            }

            char rel_path[MAX_PATH];
            char layer_path[MAX_PATH];
            int ret = get_relative_path_layer(abs_path,rel_path, layer_path);
            if(ret == -1){
                log_fatal("request path is not in container, path: %s", abs_path);
                return -1;
            }
            const char * root_path = getenv("ContainerRoot");

            char * bname = basename(rel_path);
            char dname[MAX_PATH];
            strcpy(dname, rel_path);
            dirname(dname);

            //if remove .wh file
            if(strncmp(bname,".wh",3) == 0){
                goto end;
            }


            INITIAL_SYS(creat)
                if(strcmp(root_path, layer_path) == 0){
                    //if file does not exist in other lyaers, then we directly delete them
                    char layers_resolved[MAX_PATH];
                    if(!findFileInLayersSkip(abs_path, layers_resolved, 1)){
                        goto end;
                    }

                    char whpath[MAX_PATH];
                    if(strcmp(dname, ".") == 0){
                        sprintf(whpath,"%s/.wh.%s",root_path,bname);
                    }else{
                        sprintf(whpath,"%s/%s/.wh.%s",root_path,dname,bname);
                    }
                    if(!xstat(whpath)){
                        int fd = real_creat(whpath,FILE_PERM);
                        if(fd < 0){
                            log_fatal("%s can't create file: %s with error: %s", function, whpath, strerror(errno));
                            return -1;
                        }
                        close(fd);
                    }
                    goto end;
                }else{
                    //request path is in other layers rather than rw layer
                    char whpath[MAX_PATH];
                    if(strcmp(dname, ".") == 0){
                        sprintf(whpath,"%s/.wh.%s",root_path,bname);
                    }else{
                        sprintf(whpath,"%s/%s",root_path,dname);
                        if(!xstat(whpath)){
                            recurMkdirMode(whpath,FOLDER_PERM);
                        }
                        sprintf(whpath,"%s/.wh.%s", whpath,bname);
                    }
                    int fd = real_creat(whpath, FILE_PERM);
                    if(fd < 0){
                        log_fatal("%s can't create file: %s", function, whpath);
                        return -1;
                    }
                    close(fd);
                    //do not goto end, it will remove the real one in other layers.
                    return 0;
                }
        }

end:
    if(strcmp(function,"unlinkat") == 0){
        return RETURN_SYS(unlinkat,(dirfd,abs_path,oflag))
    }else{
        return RETURN_SYS(unlink,(abs_path))
    }
}

struct dirent_obj* fufs_opendir_impl(const char* function,...){
    //container layer from top to lower
    va_list args;
    va_start(args,function);
    const char * abs_path = va_arg(args,const char *);
    va_end(args);

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
        log_fatal("%s is not inside the container, abs path: %s", rel_path, abs_path);
        return NULL;
    }

    for (int i = 0; i < num; i++) {
        char each_layer_path[MAX_PATH];
        sprintf(each_layer_path, "%s/%s", layers[i], rel_path);
        log_debug("preparing for accessing target layer: %s", each_layer_path);

        if(xstat(each_layer_path)){
            struct dirent_layers_entry* entry = getDirContent(each_layer_path);

            if (entry->data || entry->wh_masked_num > 0) {
                //intialized part for the first layer
                if (head == NULL && tail == NULL && is_empty_hmap(wh_map) && is_empty_hmap(dirent_map)) {
                    head = tail = entry->data;
                    if(head){
                        while (tail->next != NULL) {
                            //log_debug("item added to dirent_map %s", tail->d_name);
                            add_item_hmap(dirent_map, tail->d_name, NULL);
                            tail = tail->next;
                        }
                        //log_debug("item added to dirent_map %s", tail->d_name);
                        add_item_hmap(dirent_map, tail->d_name, NULL);
                    }
                    for (size_t wh_i = 0; wh_i < entry->wh_masked_num; wh_i++) {
                        //log_debug("item added to wh_map %s", entry->wh_masked[wh_i]);
                        add_item_hmap(wh_map, entry->wh_masked[wh_i], NULL);
                    }
                } else {
                    //need to merge folders files and hide wh
                    //as head and tail are not null, which means previous manipulation is produced
                    //but head and tail may be null
                    struct dirent_obj* prew = tail;
                    if(prew){
                        //tail is not NULL, meaning that head is not NULL, head should not be reset
                        if(entry->data){
                            prew->next = entry->data;
                            tail = prew->next;
                        }else{
                            goto ends;
                        }
                    }else{
                        //tail is NULL, meaning that head may be NULL as well
                        if(entry->data){
                            prew = tail = entry->data;
                            if(!head){
                                head = prew;
                            }
                        }else{
                            goto ends;
                        }
                    }
                    while (tail->next != NULL) {
                        if (!contain_item_hmap(dirent_map, tail->d_name) && !contain_item_hmap(wh_map, tail->d_name)) {
                            //log_debug("item added to dirent_map %s", tail->d_name);
                            add_item_hmap(dirent_map, tail->d_name, NULL);
                        } else {
                            //log_debug("item deteled from dirent_map %s", tail->d_name);
                            deleteItemInChainByPointer(&head, &tail);
                            if (!tail) {
                                tail = prew;
                            }
                            continue;
                        }
                        prew = tail;
                        tail = tail->next;
                    }
                    if (!contain_item_hmap(dirent_map, tail->d_name) && !contain_item_hmap(wh_map, tail->d_name)) {
                        //log_debug("item added to dirent_map %s", tail->d_name);
                        add_item_hmap(dirent_map, tail->d_name, NULL);
                    } else {
                        //log_debug("item deteled from dirent_map %s", tail->d_name);
                        deleteItemInChainByPointer(&head, &tail);
                        if (!tail) {
                            //reset tail to its previous item if tail is NULL
                            //as next iterator may modify tail e.g tail->next = new_list
                            tail = prew;
                        }
                    }

ends:
                    for (size_t wh_i = 0; wh_i < entry->wh_masked_num; wh_i++) {
                        //log_debug("item added to wh_map %s", entry->wh_masked[wh_i]);
                        add_item_hmap(wh_map, entry->wh_masked[wh_i], NULL);
                    }
                }
            } // exists data or white out files
        } // layer path exists

        //find if any whiteout parent folders exists
        if(getParentWh(each_layer_path) == 1){
            break;
        }
    }

    //clean resource
    if(dirent_map){
        destroy_hmap(dirent_map);
    }
    if(wh_map){
        destroy_hmap(wh_map);
    }

    return head;
}

int fufs_mkdir_impl(const char* function,...){
    va_list args;
    va_start(args,function);
    int dirfd = -1;
    const char *abs_path;
    mode_t mode;

    if(strcmp(function,"mkdirat") == 0){
        dirfd = va_arg(args,int);
        abs_path = va_arg(args, const char *);
        mode= va_arg(args, mode_t);
    }else{
        abs_path = va_arg(args, const char *);
        mode = va_arg(args, mode_t);
    }
    va_end(args);

    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(abs_path,rel_path,layer_path);
    if (ret == -1){
        log_fatal("%s is not inside the container", rel_path);
        return -1;
    }

    const char * container_root = getenv("ContainerRoot");
    char resolved[MAX_PATH];
    if(strcmp(layer_path,container_root) != 0){
        sprintf(resolved,"%s/%s",container_root,rel_path);
    }else{
        sprintf(resolved,"%s",abs_path);
    }

    dedotdot(resolved);
    return recurMkdirMode(resolved, mode);

    /**
      INITIAL_SYS(mkdir)
      INITIAL_SYS(mkdirat)

      if(strcmp(function,"mkdirat") == 0){
      return RETURN_SYS(mkdirat,(dirfd,resolved,mode))
      }else{
      return RETURN_SYS(mkdir,(resolved,mode))
      }
     **/
}

int fufs_link_impl(const char * function, ...){
    va_list args;
    va_start(args,function);
    int olddirfd, newdirfd,flags;
    const char *oldpath,*newpath;
    if(strcmp(function,"linkat") == 0){
        olddirfd = va_arg(args, int);
        oldpath = va_arg(args,const char *);
        newdirfd = va_arg(args, int);
        newpath = va_arg(args, const char *);
        flags = va_arg(args, int);
    }else{
        oldpath = va_arg(args, const char *);
        newpath = va_arg(args, const char *);
    }
    va_end(args);

    //newpath should be changed to rw folder
    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(newpath,rel_path,layer_path);
    if (ret == -1){
        log_fatal("%s is not inside the container", rel_path);
        return -1;
    }
    const char * container_root = getenv("ContainerRoot");
    char resolved[MAX_PATH];
    if(strcmp(layer_path,container_root) != 0){
        sprintf(resolved,"%s/%s",container_root,rel_path);
    }else{
        sprintf(resolved,"%s",newpath);
    }

    if(lxstat(resolved)){
        INITIAL_SYS(unlink)
            real_unlink(resolved);
    }

    INITIAL_SYS(linkat)
        INITIAL_SYS(link)


        if(strcmp(function,"linkat") == 0){
            return RETURN_SYS(linkat,(olddirfd,oldpath,newdirfd,resolved,flags))
        }else{
            return RETURN_SYS(link,(oldpath,resolved))
        }
}

int fufs_symlink_impl(const char *function, ...){
    va_list args;
    va_start(args,function);
    const char *target, *linkpath;
    int newdirfd;
    if(strcmp(function,"symlinkat") == 0){
        target = va_arg(args, const char *);
        newdirfd = va_arg(args, int);
        linkpath = va_arg(args, const char *);
    }else{
        target = va_arg(args, const char *);
        linkpath = va_arg(args, const char *);
    }
    va_end(args);

    //check the linkpath whether locating inside rw folder
    char resolved[MAX_PATH];
    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(linkpath,rel_path,layer_path);
    if (ret == -1){
        log_fatal("%s is not inside the container", rel_path);
        return -1;
    }
    const char * container_root = getenv("ContainerRoot");
    if(strcmp(layer_path,container_root) != 0){
        sprintf(resolved,"%s/%s",container_root,rel_path);
    }else{
        sprintf(resolved,"%s",linkpath);
    }

    INITIAL_SYS(symlinkat)
    INITIAL_SYS(symlink)

    char dir[MAX_PATH];
    strcpy(dir, resolved);
    dirname(dir);
    //parent folder does not exist
    if(!xstat(dir)){
        recurMkdirMode(dir, FOLDER_PERM);
    }

    if(strcmp(function,"symlinkat") == 0){
        return RETURN_SYS(symlinkat,(target,newdirfd,resolved))
    }else{
        return RETURN_SYS(symlink,(target,resolved))
    }
}

int fufs_creat_impl(const char *function,...){
    va_list args;
    va_start(args,function);
    const char *path = va_arg(args, const char *);
    mode_t mode = va_arg(args, mode_t);
    va_end(args);

    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(path, rel_path, layer_path);
    if (ret == -1){
        log_fatal("%s is not inside the container", rel_path);
        return -1;
    }
    const char * container_root = getenv("ContainerRoot");
    char resolved[MAX_PATH];
    if(strcmp(layer_path,container_root) != 0){
        sprintf(resolved,"%s/%s",container_root,rel_path);
    }else{
        sprintf(resolved,"%s",path);
    }

    INITIAL_SYS(creat64)
        INITIAL_SYS(creat)

        //create parent folder
        char dir[MAX_PATH];
    strcpy(dir, resolved);
    dirname(dir);
    if(!xstat(dir)){
        recurMkdirMode(dir,FOLDER_PERM);
    }

    if(strcmp(function,"creat64") == 0){
        return RETURN_SYS(creat64,(resolved,mode))
    }else{
        return RETURN_SYS(creat,(resolved,mode))
    }
}

int fufs_chmod_impl(const char* function, ...){
    va_list args;
    int fd;
    int flag;
    mode_t mode;
    const char *path;
    va_start(args, function);
    if(strcmp(function,"fchmodat") == 0){
        fd = va_arg(args, int);
        path = va_arg(args, const char *);
        mode = va_arg(args, mode_t);
        flag = va_arg(args, int);
    }else{
        path = va_arg(args,const char *);
        mode = va_arg(args, mode_t);
    }
    va_end(args);

    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(path, rel_path, layer_path);
    if(ret == -1){
        errno = EACCES;
        return -1;
    }

    INITIAL_SYS(chmod)
    INITIAL_SYS(lchmod)
    INITIAL_SYS(fchmodat)

    char resolved[MAX_PATH];
    const char * container_root = getenv("ContainerRoot");
    if(strcmp(layer_path,container_root) == 0){
        strcpy(resolved, path);
    }else{
        if(!copyFile2RW(path, resolved)){
            log_fatal("copy from %s to %s encounters error", path, resolved);
            return -1;
        }
    }

    if(strcmp(function, "chmod") == 0){
        return RETURN_SYS(chmod,(resolved, mode))
    }
    if(strcmp(function, "lchmod") == 0){
        return RETURN_SYS(lchmod,(resolved, mode))
    }
    if(strcmp(function, "fchmodat") == 0){
        return RETURN_SYS(fchmodat,(fd, resolved, mode, flag))
    }
}

int fufs_rmdir_impl(const char* function, ...){
    va_list args;
    va_start(args, function);
    const char * path = va_arg(args, const char *);
    va_end(args);

    char rel_path[MAX_PATH];
    char layer_path[MAX_PATH];
    int ret = get_relative_path_layer(path, rel_path, layer_path);
    if(ret == -1){
        errno = EACCES;
        return -1;
    }

    INITIAL_SYS(mkdir)
        INITIAL_SYS(creat)

        const char * container_root = getenv("ContainerRoot");

    char * bname = basename(rel_path);
    char dname[MAX_PATH];
    strcpy(dname, rel_path);
    dirname(dname);
    if(strcmp(layer_path,container_root) == 0){
        INITIAL_SYS(rmdir)
            char wh[MAX_PATH];
        sprintf(wh,"%s/%s/.wh.%s",container_root,dname,bname);

        char wh_dname[MAX_PATH];
        strcpy(wh_dname, wh);
        dirname(wh_dname);
        if(!xstat(wh_dname)){
            recurMkdirMode(wh_dname,FOLDER_PERM);
        }

        int fd = real_creat(wh,FILE_PERM);
        if(fd < 0){
            log_fatal("%s can't create file: %s with error: %s", function, wh, strerror(errno));
            return -1;
        }
        close(fd);
        if(xstat(path) && is_file_type(path, TYPE_DIR)){
            INITIAL_SYS(unlink)
                char **names;
            size_t num;
            getDirentsOnlyNames(path, &names,&num);
            bool is_all_wh = false;
            for(size_t i = 0;i<num;i++){
                if(strncmp(names[i],".wh",3) == 0){
                    if(i == 0){
                        is_all_wh = true;
                    }else{
                        is_all_wh = is_all_wh & true;
                    }
                }else{
                    is_all_wh = false;
                    break;
                }
            }
            if(is_all_wh){
                log_debug("all files in folder: %s are whiteout files, will entirely delete everything",path);
                char tmp[MAX_PATH];
                for(size_t i = 0; i<num; i++){
                    sprintf(tmp,"%s/%s",path,names[i]);
                    log_debug("all files in folder: %s are whiteout files, delete target item: %s",path, tmp);
                    real_unlink(tmp);
                }
            }
        }
        return RETURN_SYS(rmdir,(path))
    }else{
        char new_path[MAX_PATH];
        sprintf(new_path,"%s/%s", container_root,rel_path);
        int ret = recurMkdirMode(new_path,FOLDER_PERM);
        if(ret == 0){
            char wh[MAX_PATH];
            char n_dname[MAX_PATH];
            strcpy(n_dname, new_path);
            dirname(n_dname);
            sprintf(wh,"%s/.wh.%s",n_dname,bname);
            int fd = real_creat(wh, FILE_PERM);
            if(fd < 0){
                log_fatal("%s can't create file: %s with error: %s", function, wh, strerror(errno));
                return -1;
            }
            close(fd);
            return 0;
        }
    }
    errno = EACCES;
    return -1;
}

int fufs_rename_impl(const char* function, ...){
    va_list args;
    va_start(args, function);
    int olddirfd, newdirfd;
    const char *oldpath, *newpath;
    if(strcmp(function,"renameat") == 0){
        olddirfd = va_arg(args, int);
        oldpath = va_arg(args, const char *);
        newdirfd = va_arg(args, int);
        newpath = va_arg(args, const char *);
    }else{
        oldpath = va_arg(args, const char *);
        newpath = va_arg(args, const char *);
    }
    va_end(args);

    const char * container_root = getenv("ContainerRoot");
    char old_rel_path[MAX_PATH];
    char old_layer_path[MAX_PATH];
    int old_ret = get_relative_path_layer(oldpath, old_rel_path, old_layer_path);
    if(old_ret != 0 && strncmp(oldpath,"/tmp",strlen("/tmp")) != 0){
        log_fatal("request path is not in container, path: %s", oldpath);
        return -1;
    }
    char old_resolved[MAX_PATH];
    strcpy(old_resolved, oldpath);
    if(old_ret == 0 && strcmp(old_layer_path,container_root) != 0){
        memset(old_resolved, 0, MAX_PATH);
        copyFile2RW(oldpath, old_resolved);
        //fake deleting oldpath
        char * bname = basename(old_resolved);
        char whpath[MAX_PATH];
        sprintf(whpath, "%s/%s/.wh.%s", container_root, old_rel_path, bname);
        if(!xstat(whpath)){
            int fd = real_creat(whpath,FILE_PERM);
            if(fd < 0){
                log_fatal("%s can't create file: %s with error: %s", function, whpath, strerror(errno));
                return -1;
            }
            close(fd);
        }
    }

    char new_rel_path[MAX_PATH];
    char new_layer_path[MAX_PATH];
    int new_ret = get_relative_path_layer(newpath, new_rel_path, new_layer_path);
    if(new_ret != 0 && strncmp(newpath, "/tmp", strlen("/tmp")) != 0){
        log_fatal("request path is not in container, path: %s", newpath);
        return -1;
    }

    INITIAL_SYS(rename)
        INITIAL_SYS(renameat)
        if(strcmp(new_layer_path, container_root) == 0){
            if(strcmp(function,"renameat") == 0){
                return RETURN_SYS(renameat,(olddirfd,old_resolved,newdirfd,newpath))
            }else{
                return RETURN_SYS(rename,(old_resolved,newpath))
            }
        }else{
            //newpath is not in rw folder, replacing it by force and delete original one
            char new_resolved[MAX_PATH];
            sprintf(new_resolved,"%s/%s",container_root,new_rel_path);
            unlink(newpath);
            if(strcmp(function,"renameat") == 0){
                return RETURN_SYS(renameat,(olddirfd,old_resolved,newdirfd,new_resolved))
            }else{
                return RETURN_SYS(rename,(old_resolved,new_resolved))
            }
        }
    errno = EACCES;
    return -1;
}
