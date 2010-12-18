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


#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>


#define debug fakechroot_debug


#ifdef HAVE___ATTRIBUTE__VISIBILITY
# define LOCAL __attribute__((visibility("hidden")))
#else
# define LOCAL
#endif

#ifdef HAVE___ATTRIBUTE__CONSTRUCTOR
# define CONSTRUCTOR __attribute__((constructor))
#else
# define CONSTRUCTOR
#endif

#ifdef HAVE___ATTRIBUTE__SECTION_DATA_FAKECHROOT
# define SECTION_DATA_FAKECHROOT __attribute__((section("data.fakechroot")))
#else
# define SECTION_DATA_FAKECHROOT
#endif

#if defined(PATH_MAX)
# define FAKECHROOT_PATH_MAX PATH_MAX
#elif defined(_POSIX_PATH_MAX)
# define FAKECHROOT_PATH_MAX _POSIX_PATH_MAX
#elif defined(MAXPATHLEN)
# define FAKECHROOT_PATH_MAX MAXPATHLEN
#else
# define FAKECHROOT_PATH_MAX 2048
#endif

#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX 108
#endif


#ifdef AF_UNIX
# ifndef SUN_LEN
#  define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
# endif
#endif

#ifndef __set_errno
# define __set_errno(e) (errno = (e))
#endif

#ifndef HAVE_VFORK
# define vfork fork
#endif


#define narrow_chroot_path(path, fakechroot_path, fakechroot_ptr) \
    { \
        if ((path) != NULL && *((char *)(path)) != '\0') { \
            fakechroot_path = getenv("FAKECHROOT_BASE"); \
            if (fakechroot_path != NULL) { \
                fakechroot_ptr = strstr((path), fakechroot_path); \
                if (fakechroot_ptr == (path)) { \
                    if (strlen((path)) == strlen(fakechroot_path)) { \
                        ((char *)(path))[0] = '/'; \
                        ((char *)(path))[1] = '\0'; \
                    } else { \
                        memmove((void*)(path), (path)+strlen(fakechroot_path), 1+strlen((path))-strlen(fakechroot_path)); \
                    } \
                } \
            } \
        } \
    }

#define expand_chroot_path(path, fakechroot_path, fakechroot_buf) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL && *((char *)(path)) == '/') { \
                fakechroot_path = getenv("FAKECHROOT_BASE"); \
                if (fakechroot_path != NULL) { \
                    strcpy(fakechroot_buf, fakechroot_path); \
                    strcat(fakechroot_buf, (path)); \
                    (path) = fakechroot_buf; \
                } \
            } \
        } \
    }

#define expand_chroot_path_malloc(path, fakechroot_path, fakechroot_buf) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL && *((char *)(path)) == '/') { \
                fakechroot_path = getenv("FAKECHROOT_BASE"); \
                if (fakechroot_path != NULL) { \
                    if ((fakechroot_buf = malloc(strlen(fakechroot_path)+strlen(path)+1)) == NULL) { \
                        __set_errno(ENOMEM); \
                        return NULL; \
                    } \
                    strcpy(fakechroot_buf, fakechroot_path); \
                    strcat(fakechroot_buf, (path)); \
                    (path) = fakechroot_buf; \
                } \
            } \
        } \
    }


#define wrapper_decl_proto(function) \
    extern LOCAL struct fakechroot_wrapper fakechroot_##function##_wrapper_decl SECTION_DATA_FAKECHROOT

#define wrapper_decl(function) \
    LOCAL struct fakechroot_wrapper fakechroot_##function##_wrapper_decl SECTION_DATA_FAKECHROOT = { \
        .func = (fakechroot_wrapperfn_t) function, \
        .nextfunc = NULL, \
        .name = #function \
    }

#define wrapper_fn_t(function, return_type, arguments) \
    typedef return_type (*fakechroot_##function##_fn_t) arguments

#define wrapper_proto(function, return_type, arguments) \
    extern return_type function arguments; \
    wrapper_fn_t(function, return_type, arguments); \
    wrapper_decl_proto(function)

#if __USE_FORTIFY_LEVEL > 0 && defined __extern_always_inline && defined __va_arg_pack_len
# define wrapper_fn_name(function) __##function##_alias
# define wrapper_proto_alias(function, return_type, arguments) \
    extern return_type __REDIRECT (wrapper_fn_name(function), arguments, function); \
    wrapper_fn_t(function, return_type, arguments); \
    wrapper_decl_proto(function)
#else
# define wrapper_fn_name(function) function
# define wrapper_proto_alias(function, return_type, arguments) \
    wrapper_proto(function, return_type, arguments)
#endif

#define wrapper(function, return_type, arguments) \
    wrapper_proto(function, return_type, arguments); \
    wrapper_decl(function); \
    return_type function arguments

#define wrapper_alias(function, return_type, arguments) \
    wrapper_proto_alias(function, return_type, arguments); \
    wrapper_decl(function); \
    return_type wrapper_fn_name(function) arguments

#define nextcall(function) \
    ( \
      (fakechroot_##function##_fn_t)( \
          fakechroot_##function##_wrapper_decl.nextfunc ? \
          fakechroot_##function##_wrapper_decl.nextfunc : \
          fakechroot_loadfunc(&fakechroot_##function##_wrapper_decl) \
      ) \
    )

typedef void (*fakechroot_wrapperfn_t)(void);

struct fakechroot_wrapper {
    fakechroot_wrapperfn_t func;
    fakechroot_wrapperfn_t nextfunc;
    const char *name;
};


int fakechroot_debug (const char *, ...);
fakechroot_wrapperfn_t fakechroot_loadfunc (struct fakechroot_wrapper *);
int fakechroot_localdir (const char *);
