/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2013 Piotr Roszatycki <dexter@debian.org>

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

#ifdef HAVE_CLEARENV

#ifndef __GLIBC__
extern char **environ;
#endif

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#include "libfakechroot.h"

extern int __clearenv(void);


wrapper(clearenv, int, (void))
{
    int j, n;
    char *key, *env;
    char **tmpkey, **tmpenv;

    debug("clearenv()");

    /* Preserve old environment variables */
    tmpkey = alloca( (preserve_env_list_count + 1) * sizeof (char *) );
    tmpenv = alloca( (preserve_env_list_count + 1) * sizeof (char *) );

    for (j = 0, n = 0; j < preserve_env_list_count; j++) {
        key = preserve_env_list[j];
        env = getenv(key);
        if (env != NULL) {
            tmpkey[n] = alloca(strlen(key) + 1);
            tmpenv[n] = alloca(strlen(env) + 1);
            strcpy(tmpkey[n], key);
            strcpy(tmpenv[n], env);
            n++;
        }
    }

    tmpkey[n] = NULL;
    tmpenv[n] = NULL;

    /* Clear */
    __clearenv();

    /* Set one variable explicitly so environ won't be NULL */
    setenv("FAKECHROOT", "true", 0);

    /* Recover preserved variables */
    for (n = 0; tmpkey[n]; n++) {
        if (setenv(tmpkey[n], tmpenv[n], 1) != 0) {
            return -1;
        }
    }

    return 0;
}

#else
typedef int empty_translation_unit;
#endif
