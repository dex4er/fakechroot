/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2011 Piotr Roszatycki <dexter@debian.org>

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

#ifdef HAVE_GETSOCKNAME

#define _GNU_SOURCE
#include <sys/socket.h>

#ifdef AF_UNIX

#include <sys/un.h>
#include "libfakechroot.h"

#ifdef HAVE_GETSOCKNAME_TYPE_ARG2___SOCKADDR_ARG__
# define SOCKADDR(addr) ((addr).__sockaddr__)
# define SOCKADDR_UN(addr) ((addr).__sockaddr_un__)
#else
# define SOCKADDR(addr) (addr)
# define SOCKADDR_UN(addr) (addr)
#endif


wrapper(getsockname, int, (int s, GETSOCKNAME_TYPE_ARG2(addr), socklen_t * addrlen))
{
    int status;
    socklen_t newaddrlen;
    struct sockaddr_un newaddr;
    char *fakechroot_path, *fakechroot_ptr, fakechroot_buf[FAKECHROOT_PATH_MAX];

    debug("getsockname(%d, &addr, &addrlen)", s);

    if (SOCKADDR(addr)->sa_family != AF_UNIX) {
        return nextcall(getsockname)(s, addr, addrlen);
    }

    newaddrlen = sizeof(struct sockaddr_un);
    memset(&newaddr, 0, newaddrlen);
    status = nextcall(getsockname)(s, (struct sockaddr *)&newaddr, &newaddrlen);
    if (status != 0) {
        return status;
    }
    if (newaddr.sun_family == AF_UNIX && newaddr.sun_path && *(newaddr.sun_path)) {
        strncpy(fakechroot_buf, newaddr.sun_path, FAKECHROOT_PATH_MAX);
        narrow_chroot_path(fakechroot_buf, fakechroot_path, fakechroot_ptr);
        strncpy(newaddr.sun_path, fakechroot_buf, UNIX_PATH_MAX);
    }

    memcpy((struct sockaddr_un *)SOCKADDR_UN(addr), &newaddr, *addrlen < sizeof(struct sockaddr_un) ? *addrlen : sizeof(struct sockaddr_un));
    *addrlen = SUN_LEN(&newaddr);
    return status;
}

#endif

#endif
