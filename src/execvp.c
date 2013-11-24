/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013 Piotr Roszatycki <dexter@debian.org>

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
#include <errno.h>
#include <stddef.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include "strchrnul.h"
#include "libfakechroot.h"

#ifndef __GLIBC__
extern char **environ;
#endif

#define DEFAULT_PATH ":/usr/bin:/bin"


/*
   execvp function taken from GNU C Library.
   Copyright (C) 1991,92, 1995-99, 2002, 2004, 2005, 2007, 2009
   Free Software Foundation, Inc.
 */
wrapper(execvp, int, (const char * file, char * const argv[]))
{
    debug("execvp(\"%s\", {\"%s\", ...})", file, argv[0]);
    if (*file == '\0') {
        /* We check the simple case first. */
        __set_errno(ENOENT);
        return -1;
    }

    if (strchr(file, '/') != NULL) {
        /* Don't search when it contains a slash.  */
        return execve(file, argv, environ);
    } else {
        int got_eacces = 0;
        char *path, *p, *name;
        size_t len;
        size_t pathlen;

        path = getenv("PATH");
        if (path == NULL) {
            /* There is no `PATH' in the environment.
             The default search path is the current directory
             followed by the path `confstr' returns for `_CS_PATH'.  */
#ifdef _CS_PATH
            len = confstr(_CS_PATH, (char *) NULL, 0);
            path = (char *) alloca(1 + len);
            path[0] = ':';
            (void) confstr(_CS_PATH, path + 1, len);
#else
            path = (char *) alloca(sizeof(DEFAULT_PATH));
            strcpy(path, DEFAULT_PATH);
#endif
        }

        len = strlen(file) + 1;
        pathlen = strlen(path);
        name = alloca(pathlen + len + 1);
        /* Copy the file name at the top.  */
        name = (char *) memcpy(name + pathlen + 1, file, len);
        /* And add the slash.  */
        *--name = '/';

        p = path;
        do {
            char *startp;

            path = p;
            p = strchrnul(path, ':');

            if (p == path)
                /* Two adjacent colons, or a colon at the beginning or the end
                 of `PATH' means to search the current directory.  */
                startp = name + 1;
            else
                startp = (char *) memcpy(name - (p - path), path, p - path);

            /* Try to execute this name.  If it works, execv will not return.  */
            execve(startp, argv, environ);

            switch (errno) {
            case EACCES:
                /* Record the we got a `Permission denied' error.  If we end
                 up finding no executable we can use, we want to diagnose
                 that we did find one but were denied access.  */
                got_eacces = 1;
                break;
            case ENOENT:
            case ESTALE:
            case ENOTDIR:
                /* Those errors indicate the file is missing or not executable
                 by us, in which case we want to just try the next path
                 directory.  */
                break;

            default:
                /* Some other error means we found an executable file, but
                 something went wrong executing it; return the error to our
                 caller.  */
                return -1;
            }
        } while (*p++ != '\0');

        /* We tried every element and none of them worked.  */
        if (got_eacces)
            /* At least one failure was due to permissions, so report that
             error.  */
            __set_errno(EACCES);
    }

    /* Return the error from the last attempt (probably ENOENT).  */
    return -1;
}
