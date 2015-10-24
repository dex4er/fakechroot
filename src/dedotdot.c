/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2013-2015 Piotr Roszatycki <dexter@debian.org>

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

#include "libfakechroot.h"
#include "strlcpy.h"

/* mini_httpd - small HTTP server
 **
 ** Copyright © 1999,2000 by Jef Poskanzer <jef@mail.acme.com>.
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions
 ** are met:
 ** 1. Redistributions of source code must retain the above copyright
 **    notice, this list of conditions and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright
 **    notice, this list of conditions and the following disclaimer in the
 **    documentation and/or other materials provided with the distribution.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 ** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 ** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 ** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 ** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 ** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 ** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 ** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 ** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 ** SUCH DAMAGE.
 */

#include <string.h>

LOCAL void dedotdot(char * file)
{
    char c, *cp, *cp2;
    int l;

    if (!file || !*file)
        return;

    /* Collapse any multiple / sequences. */
    while ((cp = strstr(file, "//")) != (char*) 0) {
        for (cp2 = cp + 2; *cp2 == '/'; ++cp2)
            continue;
        (void) strlcpy(cp + 1, cp2, strlen(cp2) + 1);
    }

    /* Remove leading ./ and any /./ sequences. */
    while (strncmp(file, "./", 2) == 0)
        (void) strlcpy(file, file + 2, strlen(file) - 1);
    while ((cp = strstr(file, "/./")) != (char*) 0)
        (void) strlcpy(cp, cp + 2, strlen(cp) - 1);

    /* Alternate between removing leading ../ and removing foo/../ */
    for (;;) {
        while (strncmp(file, "/../", 4) == 0)
            (void) strlcpy(file, file + 3, strlen(file) - 2);
        cp = strstr(file, "/../");
        if (cp == (char*) 0 || strncmp(file, "../", 3) == 0)
            break;
        for (cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2)
            continue;
        (void) strlcpy(cp2 + 1, cp + 4, strlen(cp) - 3);
    }

    /* Also elide any foo/.. at the end. */
    while (strncmp(file, "../", 3) != 0 && (l = strlen(file)) > 3
            && strcmp((cp = file + l - 3), "/..") == 0) {

        for (cp2 = cp - 1; cp2 > file && *cp2 != '/'; --cp2)
            continue;

        if (cp2 < file)
            break;
        if (strncmp(cp2, "../", 3) == 0)
            break;

        c = *cp2;
        *cp2 = '\0';

        if (file == cp2 && c == '/') {
            strcpy(file, "/");
        }
    }

    /* Correct some paths */
    if (*file == '\0') {
        strcpy(file, ".");
    }
    else if (strcmp(file, "/.") == 0 || strcmp(file, "/..") == 0) {
        strcpy(file, "/");
    }

    /* Any /. and the end */
    for (l = strlen(file); l > 3 && strcmp((cp = file + l - 2), "/.") == 0; l -= 2) {
        *cp = '\0';
    }
}
