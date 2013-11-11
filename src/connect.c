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

#ifdef HAVE_CONNECT

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>

#ifdef AF_UNIX

#include <sys/un.h>
#include <errno.h>
#include "libfakechroot.h"

#ifdef HAVE_CONNECT_TYPE_ARG2___CONST_SOCKADDR_ARG__
# define SOCKADDR_UN(addr) ((addr).__sockaddr_un__)
#else
# define SOCKADDR_UN(addr) (addr)
#endif


wrapper(connect, int, (int sockfd, CONNECT_TYPE_ARG2(addr), socklen_t addrlen))
{
    struct sockaddr_un *addr_un = (struct sockaddr_un *)SOCKADDR_UN(addr);

    debug("connect(%d, &addr, %d)", sockfd, addrlen);

    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path && *(addr_un->sun_path)) {
        socklen_t newaddrlen;
        struct sockaddr_un newaddr_un;

        const char *af_unix_path = getenv("FAKECHROOT_AF_UNIX_PATH");
        const int af_unix_path_max = sizeof(addr_un->sun_path);
        char *path = addr_un->sun_path;

        if (af_unix_path != NULL) {
            char tmp[FAKECHROOT_PATH_MAX];
            tmp[af_unix_path_max] = 0;
            strncpy(tmp, af_unix_path, af_unix_path_max - 1);
            strcat(tmp, path);
            path = tmp;
        }
        else {
            expand_chroot_path(path);
        }

        if (strlen(path) >= sizeof(addr_un->sun_path)) {
            __set_errno(ENAMETOOLONG);
            return -1;
        }
        memset(&newaddr_un, 0, sizeof(struct sockaddr_un));
        newaddr_un.sun_family = addr_un->sun_family;
        strncpy(newaddr_un.sun_path, path, sizeof(newaddr_un.sun_path) - 1);
        newaddrlen = SUN_LEN(&newaddr_un);
        return nextcall(connect)(sockfd, (struct sockaddr *)&newaddr_un, newaddrlen);
    }
    return nextcall(connect)(sockfd, addr, addrlen);
}

#endif

#endif
