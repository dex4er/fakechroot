/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2014 Robin McCorkell <rmccorkell@karoshi.org.uk>

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

#ifdef HAVE_DL_ITERATE_PHDR

#define _GNU_SOURCE
#include <link.h>

#include "libfakechroot.h"


#define DL_ITERATE_PHDR_CALLBACK_ARGS struct dl_phdr_info * info, size_t size, void * data

static int (* dl_iterate_phdr_callback_saved)(DL_ITERATE_PHDR_CALLBACK_ARGS);

static int dl_iterate_phdr_callback(DL_ITERATE_PHDR_CALLBACK_ARGS)
{
    if (info->dlpi_name) {
        narrow_chroot_path(info->dlpi_name);
    }
    return dl_iterate_phdr_callback_saved(info, size, data);
}

wrapper(dl_iterate_phdr, int, (int (* callback)(DL_ITERATE_PHDR_CALLBACK_ARGS), void * data))
{
    debug("dl_iterate_phdr(&callback, 0x%x)", data);
    dl_iterate_phdr_callback_saved = callback;
    return nextcall(dl_iterate_phdr)(dl_iterate_phdr_callback, data);
}

#else
typedef int empty_translation_unit;
#endif
