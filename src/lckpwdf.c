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

#ifdef HAVE_LCKPWDF

#include <unistd.h>
#include <fcntl.h>
#include "libfakechroot.h"
#include "open.h"


wrapper(lckpwdf, int, (void))
{
    char fakechroot_abspath[FAKECHROOT_PATH_MAX];
    char fakechroot_buf[FAKECHROOT_PATH_MAX];

    int file;
    debug("lckpwdf()");
    // lckpwdf will create an empty /etc/.pwd.lock
    // if that file doesn't exist yet, we create it here as well
    char* pwdlockfile = "/etc/.pwd.lock";
    expand_chroot_path(pwdlockfile);

    if ((file = nextcall(open)(pwdlockfile, O_RDONLY)) == 0) {
        // if the file already exists, don't touch it
        close(file);
        return 0;
    }

    if ((file = nextcall(open)(pwdlockfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        // we ignore any errors (maybe /etc doesn't exist or we don't have the
        // necessary permissions)
        return 0;
    }
    // the file remains empty
    close(file);
    return 0;
}

#else
typedef int empty_translation_unit;
#endif
