#include "hmappriv.h"
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "hashmap.h"
#include "memcached_client.h"
#include <openssl/sha.h>

const int PATH_DATA_SIZE = 1024;

bool hmap_check(hmap_t* pmap, const char* container,const char* pname,const char* abs_path){
    char key[KEY_SIZE]; sprintf(key,"%s:%s:ALLOW_LIST", container, pname);
    char* allow_data = (char*)get_item_hmap(pmap, key);
    log_debug("allow_data: %s",allow_data);
    if(allow_data){
        char* valret = NULL;
        char* rest = allow_data;
        char* token = NULL;

        while((token = strtok_r(rest, ";", &rest))){
            if((valret = strstr(abs_path, token))){
                return true;
            }
        }
        return false;
    }
    return false;
}


void get_process_info(struct ProcessInfo* pinfo){
    char pathbase[PNAME_SIZE];
    FILE *fp;
    pinfo->pid = getpid();
    pinfo->ppid = getppid();
    pinfo->groupid = getpgrp();
    strcpy(pathbase,"/proc/");
    sprintf(pathbase+strlen(pathbase),"%d",pinfo->pid);
    sprintf(pathbase+strlen(pathbase),"%s","/status");

    fp = fopen(pathbase,"r");
    if(fp == NULL){
        log_fatal("can't open /proc/%d/status",pinfo->pid);
    }
    if(!fscanf(fp,"Name:%s",pinfo->pname)){
        log_fatal("can't read info from file: /proc/%d/status",pinfo->pid);
    }
    fclose(fp);
}

bool hmap_priv_check(hmap_t* pmap, const char* abs_path){
    //local check
    struct ProcessInfo pinfo;
    get_process_info(&pinfo);
    return hmap_check(pmap,"container",pinfo.pname,abs_path);
}

bool mem_check_v(const char* container, const char* pname, int n, char** paths){
    char allow_key[MEMCACHED_MAX_KEY];
    sprintf(allow_key,"%s:%s",container,pname);
    char* value = getValue(allow_key);
    bool result = false;
    if(value){
        if(strcmp(value,"all")==0){
            return true;
        }
        for(int i=0;i<n;i++){
            char* valret = NULL;
            char* rest = value;
            char* token = NULL;
            bool t_result = false;

            while((token = strtok_r(rest,";",&rest))){
                if((valret = strstr(paths[i], token))){
                    t_result = true;
                    break;
                }
            }
            if(i==0){
                result = t_result;
            }else{
                result &= t_result;
                if(!result){
                    return false;
                }
            }
        }
    }
    return result;
}

void mem_priv_check_v(int n, ...){
    char * containermode = getenv("ContainerMode");
    if (containermode && strcmp(containermode,"chroot")==0){
        return;
    }
    struct ProcessInfo pinfo;
    char buff[PATH_DATA_SIZE];
    get_process_info(&pinfo);
    va_list args;
    va_start(args,n);
    char* paths[n];
    for(int i=0;i<n;i++){
        paths[i] = va_arg(args,char*);
    }
    va_end(args);

    //get container info from envs
    char * containerid = getenv("ContainerId");
    char * containerroot = getenv("ContainerRoot");

    if(containerid && containerroot){
        bool t_result = true;
        char *valret = NULL;
        for(int i=0;i<n;i++){
            if((valret = strstr(paths[i], containerroot))==NULL){
                t_result = false;
                break;
            }
        }
        if(t_result){
            return;
        }
        //get container info ends

        if(!mem_check_v(containerid, pinfo.pname, n, paths)){
            for(int i=0;i<n;i++){
                sprintf(buff+strlen(buff),"%s;",paths[i]);
            }
            log_fatal("[Program: %s] in [Container: %s] with [RootDir: %s] on [Paths: %s] is not allowed!", pinfo.pname,containerid,containerroot, buff);
            exit(EXIT_FAILURE);
        }
    }else{
        log_fatal("fatal error, can't get container id and container root path");
        exit(EXIT_FAILURE);
    }
}

bool existStr(char *value, char * sep, int n, char **paths, int *nix){
    bool result = false;
    for(int i=0;i<n;i++){
        char* valret = NULL;
        char* rest = value;
        char* token = NULL;
        bool t_result = false;

        while((token = strtok_r(rest, sep ,&rest))){
            if((valret = strstr(paths[i], token))){
                t_result = true;
                break;
            }
        }
        nix[i] = t_result?1:0;
        if(i==0){
            result = t_result;
        }else{
            result &= t_result;
        }
    }
    return result;
}

bool existStrAlone(char *value, char *sep, char *str){
    bool result = false;
    char* valret = NULL;
    char* rest = value;
    char* token = NULL;

    while((token = strtok_r(rest, sep ,&rest))){
        if((valret = strstr(str, token))){
            result = true;
            break;
        }
    }
    return result;
}

void prependStr(char *s, const char *t){
    size_t len = strlen(t);
    size_t i;
    memmove(s+len, s, strlen(s)+1);
    for(size_t i=0;i<len;i++){
        s[i] = t[i];
    }
}

bool rt_mem_con_check(const char* container, const char* conRoot, const char* pname, int n, char** paths, char** rt_paths){
    char allow_key[MEMCACHED_MAX_KEY];
    sprintf(allow_key,"%s:%s",container,pname);
    char* value = getValue(allow_key);
    bool b_rt= false;
    bool b_mt= false;
    int *nix = NULL;
    log_debug("allow_key: %s, allow_value: %s", allow_key, value);
    if(value){
        if(strcmp(value,"all")==0){
            return true;
        }
        nix = (int *)calloc(n, sizeof(int));
        b_mt = existStr(value,";", n, paths, nix);
    }

    if(!b_mt){
        char map_key[MEMCACHED_MAX_KEY];
        sprintf(map_key,"map:%s:%s", container,pname);
        char *map_value = getValue(map_key);

        log_debug("map_key:%s, map_value:%s",map_key,map_value);
        if(map_value){
            bool v_all = false;
            if (strcmp(map_value,"all") == 0){
                v_all = true;
            }
            if(rt_paths == NULL){
                rt_paths = (char**)malloc(sizeof(char*)*n);
            }

            //replace all/only illegal
            if(nix == NULL){
                if(v_all){
                    for(int i=0;i<n;i++){
                        char *path = (char*)malloc(sizeof(char)*PATH_MAX_LENGTH);
                        strcpy(path, conRoot);
                        strcat(path, paths[i]);
                        rt_paths[i] = path;
                    }
                    b_rt = true;
                }else{
                    int *v_nix = (int *)calloc(n, sizeof(int));
                    bool v_b_mt = existStr(map_value,";", n, paths, v_nix);
                    //every path in paths is in map_value, we should map path to new
                    //path
                    if(v_b_mt){
                        for(int i=0;i<n;i++){
                            char * path = (char*)malloc(sizeof(char)*PATH_MAX_LENGTH);
                            strcpy(path, conRoot);
                            strcat(path, paths[i]);
                            rt_paths[i] = path;
                        }
                        b_rt = true;
                    }else{
                        for(int i=0;i<n;i++){
                            char *path = (char*)malloc(sizeof(char)*PATH_MAX_LENGTH);
                            if(v_nix[i] == 1){
                                //which exists in list replace it
                                strcpy(path, conRoot);
                                strcat(path, paths[i]);
                                b_rt |= true;
                            }else{
                                strcpy(path, paths[i]);
                                b_rt |= false;
                            }
                            rt_paths[i] = path;
                        }
                    }
                }

                //nix = NULL not settings for program in allow_list
                //nix = NULL ends
            }else{
                if(v_all){
                    for(int i=0;i<n;i++){
                        //the path which is not in nix will be replaced, if path in nix, which
                        //means program already has the access to that path, and it could
                        //directrly write and read to that path
                        char *path = (char*)malloc(sizeof(char)*PATH_MAX_LENGTH);
                        if(nix[i]==0){
                            strcpy(path,conRoot);
                            strcat(path, paths[i]);
                            b_rt |= true;
                        }else{
                            strcpy(path, paths[i]);
                            b_rt |= false;
                        }
                        rt_paths[i] = path;
                    }
                } else{
                    for(int i=0;i<n;i++){
                        char *path = (char*)malloc(sizeof(char)*PATH_MAX_LENGTH);
                        if(nix[i]==0){
                            bool v_b_r = existStrAlone(map_value, ";",paths[i]);
                            if(v_b_r){
                                strcpy(path,conRoot);
                                strcat(path, paths[i]);
                                b_rt |= true;
                            }else{
                                strcpy(path,paths[i]);
                                b_rt |= false;
                            }
                            rt_paths[i] = path;
                        }else{
                            strcpy(path, paths[i]);
                            rt_paths[i] = path;
                            b_rt |= false;
                        }
                    }
                }
            }
        } //map value ends
    }

    if(nix){
        free(nix);
    }
    return b_rt || b_mt;
}

bool rt_mem_check(int n, char** rt_paths, ...){
    char * check_switch = getenv("__priv_switch");
    if(!check_switch){
        return true;
    }
    char * containermode = getenv("ContainerMode");
    if (containermode && strcmp(containermode,"chroot")==0){
        return true;
    }
    struct ProcessInfo pinfo;
    char buff[PATH_DATA_SIZE];
    get_process_info(&pinfo);
    va_list args;
    va_start(args, rt_paths);
    char* paths[n];
    for(int i=0;i<n;i++){
        paths[i] = va_arg(args,char*);
    }
    va_end(args);

    //get container info from envs
    char * containerid = getenv("ContainerId");
    char * containerroot = getenv("ContainerRoot");

    if(containerid && containerroot){
        bool t_result = true;
        char *valret = NULL;
        for(int i=0;i<n;i++){
            if((valret = strstr(paths[i], containerroot))==NULL){
                t_result = false;
                break;
            }
        }
        if(t_result){
            return t_result; }
        //get container info ends

        if(!rt_mem_con_check(containerid, containerroot, pinfo.pname, n, paths, rt_paths)){
            for(int i=0;i<n;i++){
                sprintf(buff+strlen(buff),"%s;",paths[i]);
            }
            log_fatal("[Program: %s] in [Container: %s] with [RootDir: %s] on [Paths: %s] is not allowed!", pinfo.pname,containerid,containerroot, buff);
            return false;
        }
        return true;
    }

    log_fatal("fatal error, can't get container id and container root path");
    return false;
}
