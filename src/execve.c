/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>

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
# include <alloca.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include "strchrnul.h"
#include "libfakechroot.h"
#include "open.h"
#include "unsetenv.h"


/* Parse the FAKECHROOT_CMD_SUBST environment variable (the first
 * parameter) and if there is a match with filename, return the
 * substitution in cmd_subst.  Returns non-zero if there was a match.
 *
 * FAKECHROOT_CMD_SUBST=cmd=subst:cmd=subst:...
 */
static int try_cmd_subst (char * env, const char * filename, char * cmd_subst)
{
    int len, len2;
    char *p;

    if (env == NULL || filename == NULL)
        return 0;

    /* Remove trailing dot from filename */
    if (filename[0] == '.' && filename[1] == '/')
        filename++;
    len = strlen(filename);

    do {
        p = strchrnul(env, ':');

        if (strncmp(env, filename, len) == 0 && env[len] == '=') {
            len2 = p - &env[len+1];
            if (len2 >= FAKECHROOT_PATH_MAX)
                len2 = FAKECHROOT_PATH_MAX - 1;
            strncpy(cmd_subst, &env[len+1], len2);
            cmd_subst[len2] = '\0';
            return 1;
        }

        env = p;
    } while (*env++ != '\0');

    return 0;
}


wrapper(execve, int, (const char * filename, char * const argv [], char * const envp []))
{
    int file;
    char hashbang[FAKECHROOT_PATH_MAX];
    size_t argv_max = 1024;
    const char **newargv = alloca(argv_max * sizeof (const char *));
    char **newenvp, **ep;
    char *key, *env;
    char *cmdorig;
    char tmp[FAKECHROOT_PATH_MAX];
    char substfilename[FAKECHROOT_PATH_MAX];
    char newfilename[FAKECHROOT_PATH_MAX];
    char argv0[FAKECHROOT_PATH_MAX];
    char *ptr;
    unsigned int i, j, n, len, newenvppos;
    unsigned int do_cmd_subst = 0;
    size_t sizeenvp;
    char c;
    char *fakechroot_path, fakechroot_buf[FAKECHROOT_PATH_MAX];
    char *envkey[] = {
        "FAKECHROOT",
        "FAKECHROOT_BASE",
        "FAKECHROOT_CMD_SUBST",
        "FAKECHROOT_DEBUG",
        "FAKECHROOT_DETECT",
        "FAKECHROOT_EXCLUDE_PATH",
        "FAKECHROOT_VERSION",
        "LD_LIBRARY_PATH",
        "LD_PRELOAD"
    };
    const int nr_envkey = sizeof envkey / sizeof envkey[0];

    debug("execve(\"%s\", {\"%s\", ...}, {\"%s\", ...})", filename, argv[0], envp[0]);

    strncpy(argv0, filename, FAKECHROOT_PATH_MAX);

    /* Substitute command only if FAKECHROOT_CMD_ORIG is not set. Unset variable if it is empty. */
    cmdorig = getenv("FAKECHROOT_CMD_ORIG");
    if (cmdorig == NULL)
        do_cmd_subst = try_cmd_subst(getenv("FAKECHROOT_CMD_SUBST"), argv0, substfilename);
    else if (!*cmdorig)
        unsetenv("FAKECHROOT_CMD_ORIG");

    /* Scan envp and check its size */
    sizeenvp = 0;
    for (ep = (char **)envp; *ep != NULL; ++ep) {
        sizeenvp++;
    }

    /* Copy envp to newenvp */
    newenvp = malloc( (sizeenvp + 1) * sizeof (char *) );
    if (newenvp == NULL) {
        __set_errno(ENOMEM);
        return -1;
    }
    for (ep = (char **) envp, newenvppos = 0; *ep != NULL; ++ep) {
        for (j = 0; j < nr_envkey; j++) {
            len = strlen(envkey[j]);
            if (strncmp(*ep, envkey[j], len) == 0 && (*ep)[len] == '=')
                goto skip;
        }
        newenvp[newenvppos] = *ep;
        newenvppos++;
    skip: ;
    }
    newenvp[newenvppos] = NULL;

    /* Add our variables to newenvp */
    newenvp = realloc(newenvp, (newenvppos + nr_envkey + 1) * sizeof(char *));

    if (newenvp == NULL) {
        __set_errno(ENOMEM);
        return -1;
    }

    for (j = 0; j < nr_envkey; j++) {
        key = envkey[j];
        env = getenv(key);
        if (env != NULL) {
            if (do_cmd_subst && strcmp(key, "FAKECHROOT_BASE") == 0) {
                key = "FAKECHROOT_BASE_ORIG";
            }
            newenvp[newenvppos] = malloc(strlen(key) + strlen(env) + 3);
            strcpy(newenvp[newenvppos], key);
            strcat(newenvp[newenvppos], "=");
            strcat(newenvp[newenvppos], env);
            newenvppos++;
        }
    }

    if (do_cmd_subst) {
        newenvp[newenvppos] = malloc(strlen("FAKECHROOT_CMD_ORIG=") + strlen(filename));
        strcpy(newenvp[newenvppos], "FAKECHROOT_CMD_ORIG=");
        strcat(newenvp[newenvppos], filename);
        newenvppos++;
    }

    newenvp[newenvppos] = NULL;

    /* Exec substituded command */
    if (do_cmd_subst) {
        debug("nextcall(execve)(\"%s\", {\"%s\", ...}, {\"%s\", ...})", substfilename, argv[0], newenvp[0]);
        return nextcall(execve)(substfilename, (char * const *)argv, newenvp);
    }

    /* Check hashbang */
    expand_chroot_path(filename, fakechroot_path, fakechroot_buf);
    strcpy(tmp, filename);
    filename = tmp;

    if ((file = nextcall(open)(filename, O_RDONLY)) == -1) {
        __set_errno(ENOENT);
        return -1;
    }

    i = read(file, hashbang, FAKECHROOT_PATH_MAX-2);
    close(file);
    if (i == -1) {
        __set_errno(ENOENT);
        return -1;
    }

    /* No hashbang in argv */
    if (hashbang[0] != '#' || hashbang[1] != '!')
        return nextcall(execve)(filename, argv, newenvp);

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
                    expand_chroot_path(ptr, fakechroot_path, fakechroot_buf);
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

    for (i = 1; argv[i] != NULL && i<argv_max; ) {
        newargv[n++] = argv[i++];
    }

    newargv[n] = 0;

    return nextcall(execve)(newfilename, (char * const *)newargv, newenvp);
}
