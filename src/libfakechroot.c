/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2003, 2005, 2007-2011 Piotr Roszatycki <dexter@debian.org>
    Copyright (c) 2007 Mark Eichin <eichin@metacarta.com>
    Copyright (c) 2006, 2007 Alexander Shishkin <virtuoso@slind.org>

    klik2 support -- give direct access to a list of directories
    Copyright (c) 2006, 2007 Lionel Tricon <lionel.tricon@free.fr>

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

#define _GNU_SOURCE
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <dlfcn.h>
#include "setenv.h"
#include "libfakechroot.h"


/* Useful to exclude a list of directories or files */
static char *exclude_list[32];
static int exclude_length[32];
static int list_max = 0;
static int first = 0;
static char *home_path = NULL;


LOCAL int fakechroot_debug (const char *fmt, ...)
{
    va_list ap;
    int ret;
    char newfmt[2048];

    if (!getenv("FAKECHROOT_DEBUG"))
        return 0;

    sprintf(newfmt, PACKAGE ": %s\n", fmt);

    va_start(ap, fmt);
    ret = vfprintf(stderr, newfmt, ap);
    va_end(ap);

    return ret;
}


#include "getcwd.h"


/* Bootstrap the library */
void fakechroot_init (void) CONSTRUCTOR;
void fakechroot_init (void)
{
    int i,j;
    struct passwd* passwd = NULL;
    char *pointer;

    if ((pointer = getenv("FAKECHROOT_DETECT"))) {
        /* printf causes coredump on FreeBSD */
        if (write(STDOUT_FILENO, PACKAGE, sizeof(PACKAGE)-1) &&
            write(STDOUT_FILENO, " ", 1) &&
            write(STDOUT_FILENO, VERSION, sizeof(VERSION)-1) &&
            write(STDOUT_FILENO, "\n", 1)) { /* -Wunused-result */ }
        _Exit(atoi(pointer));
    }

    debug("fakechroot_init()");
    debug("FAKECHROOT_BASE=\"%s\"", getenv("FAKECHROOT_BASE"));
    debug("FAKECHROOT_BASE_ORIG=\"%s\"", getenv("FAKECHROOT_BASE_ORIG"));
    debug("FAKECHROOT_CMD_ORIG=\"%s\"", getenv("FAKECHROOT_CMD_ORIG"));

    if (!first) {
        first = 1;

        /* We get a list of directories or files */
        if ((pointer = getenv("FAKECHROOT_EXCLUDE_PATH"))) {
            for (i=0; list_max<32 ;) {
                for (j=i; pointer[j]!=':' && pointer[j]!='\0'; j++);
                exclude_list[list_max] = malloc(j-i+2);
                memset(exclude_list[list_max], '\0', j-i+2);
                strncpy(exclude_list[list_max], &(pointer[i]), j-i);
                exclude_length[list_max] = strlen(exclude_list[list_max]);
                list_max++;
                if (pointer[j] != ':') break;
                i=j+1;
            }
        }

        /* We get the home of the user */
        passwd = getpwuid(getuid());
        if (passwd && passwd->pw_dir) {
            home_path = malloc(strlen(passwd->pw_dir)+2);
            strcpy(home_path, passwd->pw_dir);
            strcat(home_path, "/");
        }

        setenv("FAKECHROOT", "true", 1);
        setenv("FAKECHROOT_VERSION", FAKECHROOT, 1);
    }
}


/* Lazily load function */
LOCAL fakechroot_wrapperfn_t fakechroot_loadfunc (struct fakechroot_wrapper * w)
{
    char *msg;
    if (!(w->nextfunc = dlsym(RTLD_NEXT, w->name))) {;
        msg = dlerror();
        fprintf(stderr, "%s: %s: %s\n", PACKAGE, w->name, msg != NULL ? msg : "unresolved symbol");
        exit(EXIT_FAILURE);
    }
    return w->nextfunc;
}


/* Check if path is on exclude list */
LOCAL int fakechroot_localdir (const char * p_path)
{
    char *v_path = (char*)p_path;
    char *fakechroot_path, *fakechroot_ptr;
    char cwd_path[FAKECHROOT_PATH_MAX];
    int i, len;

    if (!p_path) return 0;

    if (!first) fakechroot_init();

    /* We need to expand ~ paths */
    if (home_path!=NULL && p_path[0]=='~') {
        strcpy(cwd_path, home_path);
        strcat(cwd_path, &(p_path[1]));
        v_path = cwd_path;
    }

    /* We need to expand relative paths */
    if (p_path[0] != '/') {
        nextcall(getcwd)(cwd_path, FAKECHROOT_PATH_MAX);
        v_path = cwd_path;
        narrow_chroot_path(v_path, fakechroot_path, fakechroot_ptr);
    }

    /* We try to find if we need direct access to a file */
    len = strlen(v_path);
    for (i=0; i<list_max; i++) {
        if (exclude_length[i]>len ||
            v_path[exclude_length[i]-1]!=(exclude_list[i])[exclude_length[i]-1] ||
            strncmp(exclude_list[i],v_path,exclude_length[i])!=0) continue;
        if (exclude_length[i]==len || v_path[exclude_length[i]]=='/') return 1;
    }

    return 0;
}
