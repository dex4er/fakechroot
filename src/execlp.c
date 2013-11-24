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

#include <stdarg.h>
#include <stddef.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include "libfakechroot.h"


/*
   execlp function taken from GNU C Library.
   Copyright (C) 1991,93,96,97,98,99,2002,2005 Free Software Foundation, Inc.
 */
wrapper(execlp, int, (const char * file, const char * arg, ...))
{
    size_t argv_max = 1024;
    const char **argv = alloca(argv_max * sizeof(const char *));
    unsigned int i;

    va_list args;
    va_start(args, arg);

    debug("execlp(\"%s\", \"%s\", ...)", file, arg);
    argv[0] = arg;

    i = 0;
    while (argv[i++] != NULL) {
        if (i == argv_max) {
            const char **nptr = alloca((argv_max *= 2) * sizeof(const char *));

            if ((char *) argv + i == (char *) nptr)
                /* Stack grows up.  */
                argv_max += i;
            else
                /* We have a hole in the stack.  */
                argv = (const char **) memcpy(nptr, argv, i
                        * sizeof(const char *));
        }

        argv[i] = va_arg(args, const char *);
    }
    va_end(args);

    return execvp(file, (char * const *) argv);
}
