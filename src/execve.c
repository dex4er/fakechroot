/*
   libfakechroot -- fake chroot environment
   Copyright (c) 2010-2015 Piotr Roszatycki <dexter@debian.org>

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

#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include "strchrnul.h"
#include "libfakechroot.h"
#include "open.h"
#include "setenv.h"
#include "readlink.h"
#include "unionfs.h"

wrapper(execve, int, (const char * filename, char * const argv [], char * const envp []))
{
    int status;
    int file;
    int is_base_orig = 0;
    char hashbang[FAKECHROOT_PATH_MAX];
    size_t argv_max = 1024;
    const char **newargv = alloca(argv_max * sizeof (const char *));
    char **newenvp, **ep;
    char *key, *env;
    char tmpkey[1024], *tp;
    char *cmdorig;
    char tmp[FAKECHROOT_PATH_MAX];
    char substfilename[FAKECHROOT_PATH_MAX];
    char newfilename[FAKECHROOT_PATH_MAX];
    char argv0[FAKECHROOT_PATH_MAX];
    char *ptr;
    unsigned int i, j, n, newenvppos;
    unsigned int do_cmd_subst = 0;
    size_t sizeenvp;
    char c;

    char *elfloader = getenv("FAKECHROOT_ELFLOADER");
    char *elfloader_opt_argv0 = getenv("FAKECHROOT_ELFLOADER_OPT_ARGV0");

    if (elfloader && !*elfloader) elfloader = NULL;
    if (elfloader_opt_argv0 && !*elfloader_opt_argv0) elfloader_opt_argv0 = NULL;

    debug("execve start(\"%s\", {\"%s\", ...}, {\"%s\", ...})", filename, argv[0], envp ? envp[0] : "(null)");

    strncpy(argv0, filename, FAKECHROOT_PATH_MAX);

    /* Substitute command only if FAKECHROOT_CMD_ORIG is not set. Unset variable if it is empty. */
    cmdorig = getenv("FAKECHROOT_CMD_ORIG");
    if (cmdorig == NULL){
        do_cmd_subst = fakechroot_try_cmd_subst(getenv("FAKECHROOT_CMD_SUBST"), argv0, substfilename);
    }else if (!*cmdorig){
        unsetenv("FAKECHROOT_CMD_ORIG");
    }

    /* Scan envp and check its size */
    sizeenvp = 0;
    if (envp) {
        for (ep = (char **)envp; *ep != NULL; ++ep) {
            sizeenvp++;
        }
    }

    /* Copy envp to newenvp */
    newenvp = malloc( (sizeenvp + preserve_env_list_count + 1) * sizeof (char *) );
    if (newenvp == NULL) {
        __set_errno(ENOMEM);
        return -1;
    }
    newenvppos = 0;

    /* Create new envp */
    newenvp[newenvppos] = malloc(strlen("FAKECHROOT=true") + 1);
    strcpy(newenvp[newenvppos], "FAKECHROOT=true");
    //num of newenvp
    newenvppos++;

    /* Preserve old environment variables if not overwritten by new */
    for (j = 0; j < preserve_env_list_count; j++) {
        key = preserve_env_list[j];
        env = getenv(key);
        if (env != NULL && *env) {
            if (do_cmd_subst && strcmp(key, "FAKECHROOT_BASE") == 0) {
                key = "FAKECHROOT_BASE_ORIG";
                is_base_orig = 1;
            }
            if (envp) {
                for (ep = (char **) envp; *ep != NULL; ++ep) {
                    strncpy(tmpkey, *ep, 1024);
                    tmpkey[1023] = 0;
                    if ((tp = strchr(tmpkey, '=')) != NULL) {
                        *tp = 0;
                        if (strcmp(tmpkey, key) == 0) {
                            goto skip1;
                        }
                    }
                }
            }
            newenvp[newenvppos] = malloc(strlen(key) + strlen(env) + 3);
            strcpy(newenvp[newenvppos], key);
            strcat(newenvp[newenvppos], "=");
            strcat(newenvp[newenvppos], env);
            newenvppos++;
skip1: ;
        }
    }

    /* Append old envp to new envp */
    if (envp) {
        for (ep = (char **) envp; *ep != NULL; ++ep) {
            strncpy(tmpkey, *ep, 1024);
            tmpkey[1023] = 0;
            if ((tp = strchr(tmpkey, '=')) != NULL) {
                *tp = 0;
                if (strcmp(tmpkey, "FAKECHROOT") == 0 ||
                        (is_base_orig && strcmp(tmpkey, "FAKECHROOT_BASE") == 0))
                {
                    goto skip2;
                }
            }
            newenvp[newenvppos] = *ep;
            newenvppos++;
skip2: ;
        }
    }

    newenvp[newenvppos] = NULL;

    if (newenvp == NULL) {
        __set_errno(ENOMEM);
        return -1;
    }

    if (do_cmd_subst) {
        newenvp[newenvppos] = malloc(strlen("FAKECHROOT_CMD_ORIG=") + strlen(filename) + 1);
        strcpy(newenvp[newenvppos], "FAKECHROOT_CMD_ORIG=");
        strcat(newenvp[newenvppos], filename);
        newenvppos++;
    }

    newenvp[newenvppos] = NULL;

    /* Exec substituted command */
    if (do_cmd_subst) {
        debug("nextcall(execve) exec substituted command(\"%s\", {\"%s\", ...}, {\"%s\", ...})", substfilename, argv[0], newenvp[0]);
        status = nextcall(execve)(substfilename, (char * const *)argv, newenvp);
        goto error;
    }

    /* Check hashbang */
    //make filename drop const
    char orig_filename[FAKECHROOT_PATH_MAX];
    strcpy(orig_filename, filename);

    expand_chroot_path(filename);
    strcpy(tmp, filename);
    filename = tmp;

    //when the target does not exist
    if ((file = nextcall(open)(filename, O_RDONLY)) == -1) {
        //get exposed exe
        const char * exe = getenv("ContainerJailedEXE");
        if(exe){
            char exe_dup[FAKECHROOT_PATH_MAX];
            strcpy(exe_dup, exe);
            char *str_tmp = strtok(exe_dup,":");
            while (str_tmp != NULL){
                if(strcmp(orig_filename, str_tmp) == 0){
                    goto exe_excute;
                }
                str_tmp = strtok(NULL,":");
            }
        }

        debug("could not find executable program with name: %s", orig_filename);
exe_err:
        __set_errno(ENOENT);
        return -1;

exe_excute:
        debug("try to execute exposed program %s from host", orig_filename);
        int menvppos = 0;
        char ** menvp = malloc(sizeof(char *)*newenvppos);
        if(!menvp){
            goto exe_err;
        }
        char ** mep;
        for(mep = newenvp; *mep != NULL; mep++){
            char tvar[1024];
            strncpy(tvar, *mep, 1024);
            tvar[1023] = '\0';
            if(strncmp(tvar,"LD_PRELOAD",strlen("LD_PRELOAD")) == 0){
                continue;
            }
            if(strncmp(tvar,"FAKECHROOT",strlen("FAKECHROOT")) == 0){
                continue;
            }
            if(strncmp(tvar,"Container",strlen("Container")) == 0){
                continue;
            }
            if(strncmp(tvar,"PWD",strlen("PWD")) == 0){
                continue;
            }
            if(strncmp(tvar,"LD_LIBRARY_LPMX",strlen("LD_LIBRARY_LPMX")) == 0){
                continue;
            }
            if(strncmp(tvar,"SHELL",strlen("SHELL")) == 0){
                continue;
            }
            if(strncmp(tvar,"__",strlen("__")) == 0){
                continue;
            }
            if(strncmp(tvar,"MEMCACHED_PID",strlen("MEMCACHED_PID")) == 0){
                continue;
            }

            debug("env var %s will be added",*mep);
            menvp[menvppos] = *mep;
            menvppos++;
        }
        menvp[menvppos] = NULL;
        INITIAL_SYS(execve)
        status = real_execve(orig_filename, (char * const *)argv, menvp);
        free(menvp);
        goto error;
    }

    i = read(file, hashbang, FAKECHROOT_PATH_MAX-2);
    close(file);
    if (i == -1) {
        __set_errno(ENOENT);
        return -1;
    }

    //hashbang here may be ELF
    /* No hashbang in argv */
    if (hashbang[0] != '#' || hashbang[1] != '!') {
        if (!elfloader) {
            status = nextcall(execve)(filename, argv, newenvp);
            goto error;
        }

        /* Run via elfloader */
        for (i = 0, n = (elfloader_opt_argv0 ? 3 : 1); argv[i] != NULL && i < argv_max; ) {
            newargv[n++] = argv[i++];
        }

        newargv[n] = 0;

        n = 0;
        newargv[n++] = elfloader;
        if (elfloader_opt_argv0) {
            newargv[n++] = elfloader_opt_argv0;
            newargv[n++] = argv0;
        }
        newargv[n] = filename;

        debug("nextcall(execve) run via elfloader with no hashbang(\"%s\", {\"%s\", \"%s\", ...}, {\"%s\", ...})", elfloader, newargv[0], newargv[n], newenvp[0]);
        status = nextcall(execve)(elfloader, (char * const *)newargv, newenvp);
        goto error;
    }

    /* For hashbang we must fix argv[0] */
    hashbang[i] = hashbang[i+1] = 0;
    for (i = j = 2; (hashbang[i] == ' ' || hashbang[i] == '\t') && i < FAKECHROOT_PATH_MAX; i++, j++);
    for (n = 0; i < FAKECHROOT_PATH_MAX; i++) {
        c = hashbang[i];
        if (hashbang[i] == 0 || hashbang[i] == ' ' || hashbang[i] == '\t' || hashbang[i] == '\n') {
            hashbang[i] = 0;
            if (i > j) {
                if (n == 0) {
                    ptr = &hashbang[j];
                    expand_chroot_path(ptr);
                    strcpy(newfilename, ptr);
                }
                newargv[n++] = &hashbang[j];
            }
            j = i + 1;
        }
        if (c == '\n' || c == 0)
            break;
    }

    newargv[n++] = argv0;

    for (i = 1; argv[i] != NULL && i < argv_max; ) {
        newargv[n++] = argv[i++];
    }

    newargv[n] = 0;

    if (!elfloader) {
        status = nextcall(execve)(newfilename, (char * const *)newargv, newenvp);
        goto error;
    }

    /* Run via elfloader */
    j = elfloader_opt_argv0 ? 3 : 1;
    if (n >= argv_max - 1) {
        n = argv_max - j - 1;
    }
    newargv[n+j] = 0;
    for (i = n; i >= j; i--) {
        newargv[i] = newargv[i-j];
    }
    n = 0;
    newargv[n++] = elfloader;
    if (elfloader_opt_argv0) {
        newargv[n++] = elfloader_opt_argv0;
        newargv[n++] = argv0;
    }
    newargv[n] = newfilename;
    debug("nextcall(execve) run via elfloader with hashbang(\"%s\", {\"%s\", \"%s\", \"%s\", ...}, {\"%s\", ...})", elfloader, newargv[0], newargv[1], newargv[n], newenvp[0]);
    status = nextcall(execve)(elfloader, (char * const *)newargv, newenvp);

error:
    free(newenvp);

    return status;
}

//wrapper(execve, int, (const char * filename, char * const argv [], char * const envp []))
//{
//    int status;
//    int file;
//    int is_base_orig = 0;
//    char hashbang[FAKECHROOT_PATH_MAX];
//    size_t argv_max = 1024;
//    const char **newargv = alloca(argv_max * sizeof (const char *));
//    char **newenvp, **ep;
//    char *key, *env;
//    char tmpkey[1024], *tp;
//    char *cmdorig;
//    char tmp[FAKECHROOT_PATH_MAX];
//    char substfilename[FAKECHROOT_PATH_MAX];
//    char newfilename[FAKECHROOT_PATH_MAX];
//    char argv0[FAKECHROOT_PATH_MAX];
//    char *ptr;
//    unsigned int i, j, n, newenvppos;
//    unsigned int do_cmd_subst = 0;
//    size_t sizeenvp;
//    char c;
//
//    char *elfloader = getenv("FAKECHROOT_ELFLOADER");
//    char *elfloader_opt_argv0 = getenv("FAKECHROOT_ELFLOADER_OPT_ARGV0");
//
//    if (elfloader && !*elfloader) elfloader = NULL;
//    if (elfloader_opt_argv0 && !*elfloader_opt_argv0) elfloader_opt_argv0 = NULL;
//
//    debug("execve start(\"%s\", {\"%s\", ...}, {\"%s\", ...})", filename, argv[0], envp ? envp[0] : "(null)");
//
//    strncpy(argv0, filename, FAKECHROOT_PATH_MAX);
//
//    /* Substitute command only if FAKECHROOT_CMD_ORIG is not set. Unset variable if it is empty. */
//    cmdorig = getenv("FAKECHROOT_CMD_ORIG");
//    if (cmdorig == NULL){
//        do_cmd_subst = fakechroot_try_cmd_subst(getenv("FAKECHROOT_CMD_SUBST"), argv0, substfilename);
//    }else if (!*cmdorig){
//        unsetenv("FAKECHROOT_CMD_ORIG");
//    }
//
//    /* Scan envp and check its size */
//    sizeenvp = 0;
//    if (envp) {
//        for (ep = (char **)envp; *ep != NULL; ++ep) {
//            sizeenvp++;
//        }
//    }
//
//    /* Copy envp to newenvp */
//    newenvp = malloc( (sizeenvp + preserve_env_list_count + 1) * sizeof (char *) );
//    if (newenvp == NULL) {
//        __set_errno(ENOMEM);
//        return -1;
//    }
//    newenvppos = 0;
//
//    /* Create new envp */
//    newenvp[newenvppos] = malloc(strlen("FAKECHROOT=true") + 1);
//    strcpy(newenvp[newenvppos], "FAKECHROOT=true");
//    //num of newenvp
//    newenvppos++;
//
//    /* Preserve old environment variables if not overwritten by new */
//    for (j = 0; j < preserve_env_list_count; j++) {
//        key = preserve_env_list[j];
//        env = getenv(key);
//        if (env != NULL && *env) {
//            if (do_cmd_subst && strcmp(key, "FAKECHROOT_BASE") == 0) {
//                key = "FAKECHROOT_BASE_ORIG";
//                is_base_orig = 1;
//            }
//            if (envp) {
//                for (ep = (char **) envp; *ep != NULL; ++ep) {
//                    strncpy(tmpkey, *ep, 1024);
//                    tmpkey[1023] = 0;
//                    if ((tp = strchr(tmpkey, '=')) != NULL) {
//                        *tp = 0;
//                        if (strcmp(tmpkey, key) == 0) {
//                            goto skip1;
//                        }
//                    }
//                }
//            }
//            newenvp[newenvppos] = malloc(strlen(key) + strlen(env) + 3);
//            strcpy(newenvp[newenvppos], key);
//            strcat(newenvp[newenvppos], "=");
//            strcat(newenvp[newenvppos], env);
//            newenvppos++;
//skip1: ;
//        }
//    }
//
//    /* Append old envp to new envp */
//    if (envp) {
//        for (ep = (char **) envp; *ep != NULL; ++ep) {
//            strncpy(tmpkey, *ep, 1024);
//            tmpkey[1023] = 0;
//            if ((tp = strchr(tmpkey, '=')) != NULL) {
//                *tp = 0;
//                if (strcmp(tmpkey, "FAKECHROOT") == 0 ||
//                        (is_base_orig && strcmp(tmpkey, "FAKECHROOT_BASE") == 0))
//                {
//                    goto skip2;
//                }
//            }
//            newenvp[newenvppos] = *ep;
//            newenvppos++;
//skip2: ;
//        }
//    }
//
//    newenvp[newenvppos] = NULL;
//
//    if (newenvp == NULL) {
//        __set_errno(ENOMEM);
//        return -1;
//    }
//
//    if (do_cmd_subst) {
//        newenvp[newenvppos] = malloc(strlen("FAKECHROOT_CMD_ORIG=") + strlen(filename) + 1);
//        strcpy(newenvp[newenvppos], "FAKECHROOT_CMD_ORIG=");
//        strcat(newenvp[newenvppos], filename);
//        newenvppos++;
//    }
//
//    newenvp[newenvppos] = NULL;
//
//    /* Exec substituted command */
//    if (do_cmd_subst) {
//        debug("nextcall(execve) exec substituted command(\"%s\", {\"%s\", ...}, {\"%s\", ...})", substfilename, argv[0], newenvp[0]);
//        status = nextcall(execve)(substfilename, (char * const *)argv, newenvp);
//        goto error;
//    }
//
//    /* Check hashbang */
//    expand_chroot_path(filename);
//    strcpy(tmp, filename);
//    filename = tmp;
//
//    if ((file = nextcall(open)(filename, O_RDONLY)) == -1) {
//        __set_errno(ENOENT);
//        return -1;
//    }
//
//    i = read(file, hashbang, FAKECHROOT_PATH_MAX-2);
//    close(file);
//    if (i == -1) {
//        __set_errno(ENOENT);
//        return -1;
//    }
//
//    //hashbang here may be ELF
//    /* No hashbang in argv */
//    if (hashbang[0] != '#' || hashbang[1] != '!') {
//        if (!elfloader) {
//            status = nextcall(execve)(filename, argv, newenvp);
//            goto error;
//        }
//
//        /* Run via elfloader */
//        for (i = 0, n = (elfloader_opt_argv0 ? 3 : 1); argv[i] != NULL && i < argv_max; ) {
//            newargv[n++] = argv[i++];
//        }
//
//        newargv[n] = 0;
//
//        n = 0;
//        newargv[n++] = elfloader;
//        if (elfloader_opt_argv0) {
//            newargv[n++] = elfloader_opt_argv0;
//            newargv[n++] = argv0;
//        }
//        newargv[n] = filename;
//
//        debug("nextcall(execve) run via elfloader with no hashbang(\"%s\", {\"%s\", \"%s\", ...}, {\"%s\", ...})", elfloader, newargv[0], newargv[n], newenvp[0]);
//        status = nextcall(execve)(elfloader, (char * const *)newargv, newenvp);
//        goto error;
//    }
//
//    /* For hashbang we must fix argv[0] */
//    hashbang[i] = hashbang[i+1] = 0;
//    for (i = j = 2; (hashbang[i] == ' ' || hashbang[i] == '\t') && i < FAKECHROOT_PATH_MAX; i++, j++);
//    for (n = 0; i < FAKECHROOT_PATH_MAX; i++) {
//        c = hashbang[i];
//        if (hashbang[i] == 0 || hashbang[i] == ' ' || hashbang[i] == '\t' || hashbang[i] == '\n') {
//            hashbang[i] = 0;
//            if (i > j) {
//                if (n == 0) {
//                    ptr = &hashbang[j];
//                    expand_chroot_path(ptr);
//                    strcpy(newfilename, ptr);
//                }
//                newargv[n++] = &hashbang[j];
//            }
//            j = i + 1;
//        }
//        if (c == '\n' || c == 0)
//            break;
//    }
//
//    newargv[n++] = argv0;
//
//    for (i = 1; argv[i] != NULL && i < argv_max; ) {
//        newargv[n++] = argv[i++];
//    }
//
//    newargv[n] = 0;
//
//    if (!elfloader) {
//        status = nextcall(execve)(newfilename, (char * const *)newargv, newenvp);
//        goto error;
//    }
//
//    /* Run via elfloader */
//    j = elfloader_opt_argv0 ? 3 : 1;
//    if (n >= argv_max - 1) {
//        n = argv_max - j - 1;
//    }
//    newargv[n+j] = 0;
//    for (i = n; i >= j; i--) {
//        newargv[i] = newargv[i-j];
//    }
//    n = 0;
//    newargv[n++] = elfloader;
//    if (elfloader_opt_argv0) {
//        newargv[n++] = elfloader_opt_argv0;
//        newargv[n++] = argv0;
//    }
//    newargv[n] = newfilename;
//    debug("nextcall(execve) run via elfloader with hashbang(\"%s\", {\"%s\", \"%s\", \"%s\", ...}, {\"%s\", ...})", elfloader, newargv[0], newargv[1], newargv[n], newenvp[0]);
//    status = nextcall(execve)(elfloader, (char * const *)newargv, newenvp);
//
//error:
//    free(newenvp);
//
//    return status;
//}
