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

#ifdef HAVE_GETSOCKNAME

#define _GNU_SOURCE
#include <stddef.h>
#include <sys/socket.h>

#ifdef AF_UNIX

#include <sys/un.h>

#include "libfakechroot.h"
#include "strlcpy.h"

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
    socklen_t origlen = *addrlen;

    debug("getsockname(%d, &addr, &addrlen)", s);

    status = nextcall(getsockname)(s, addr, addrlen);
    if (status == 0 && SOCKADDR(addr)->sa_family == AF_UNIX) {
        struct sockaddr_un *addr_un = SOCKADDR_UN(addr);
        size_t path_max = origlen - offsetof(struct sockaddr_un, sun_path);
        if (path_max > origlen) {
            /* underflow, addr does not have space for the path */
            return status;
        } else if (path_max > sizeof(addr_un->sun_path)) {
            path_max = sizeof(addr_un->sun_path);
        }
        if (addr_un->sun_path && *(addr_un->sun_path)) {
            char tmp[FAKECHROOT_PATH_MAX];
            strlcpy(tmp, addr_un->sun_path, FAKECHROOT_PATH_MAX);
            narrow_chroot_path(tmp);
            strlcpy(addr_un->sun_path, tmp, path_max);
            *addrlen = SUN_LEN(addr_un);
        }
    }

    return status;
}

#else
typedef int empty_translation_unit;
#endif

#else
typedef int empty_translation_unit;
#endif
