/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010-2015, 2019 Piotr Roszatycki <dexter@debian.org>

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


#ifndef __LIBFAKECHROOT_H
#define __LIBFAKECHROOT_H

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "rel2abs.h"
#include "rel2absat.h"


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


#define narrow_chroot_path(path) \
    { \
        if ((path) != NULL && *((char *)(path)) != '\0') { \
            const char *fakechroot_base = getenv("FAKECHROOT_BASE"); \
            if (fakechroot_base != NULL) { \
                char *fakechroot_ptr = strstr((path), fakechroot_base); \
                if (fakechroot_ptr == (path)) { \
                    const size_t fakechroot_base_len = strlen(fakechroot_base); \
                    const size_t path_len = strlen(path); \
                    if (path_len == fakechroot_base_len) { \
                        ((char *)(path))[0] = '/'; \
                        ((char *)(path))[1] = '\0'; \
                    } \
                    else if ( ((char *)(path))[fakechroot_base_len] == '/' ) { \
                        memmove((void *)(path), (path) + fakechroot_base_len, 1 + path_len - fakechroot_base_len); \
                    } \
                } \
            } \
        } \
    }

#define expand_chroot_rel_path(path) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL && *((char *)(path)) == '/') { \
                const char *fakechroot_base = getenv("FAKECHROOT_BASE"); \
                if (fakechroot_base != NULL ) { \
                    snprintf(fakechroot_buf, FAKECHROOT_PATH_MAX, "%s%s", fakechroot_base, (path)); \
                    (path) = fakechroot_buf; \
                } \
            } \
        } \
    }

#define expand_chroot_path(path) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL) { \
                rel2abs((path), fakechroot_abspath); \
                (path) = fakechroot_abspath; \
                expand_chroot_rel_path(path); \
            } \
        } \
    }

#define expand_chroot_path_at(dirfd, path) \
    { \
        if (!fakechroot_localdir(path)) { \
            if ((path) != NULL) { \
                rel2absat(dirfd, (path), fakechroot_abspath); \
                (path) = fakechroot_abspath; \
                expand_chroot_rel_path(path); \
            } \
        } \
    }


#define wrapper_decl_proto(function) \
    extern LOCAL struct fakechroot_wrapper fakechroot_##function##_wrapper_decl SECTION_DATA_FAKECHROOT

#define wrapper_decl(function) \
    LOCAL struct fakechroot_wrapper fakechroot_##function##_wrapper_decl SECTION_DATA_FAKECHROOT = { \
        (fakechroot_wrapperfn_t) function, \
        NULL, \
        #function \
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


#ifdef __GNUC__
# if __GNUC__ >= 6
#  pragma GCC diagnostic ignored "-Wnonnull-compare"
# endif
#endif

#ifdef __clang__
# if __clang_major__ >= 4 || __clang_major__ == 3 && __clang_minor__ >= 6
#  pragma clang diagnostic ignored "-Wpointer-bool-conversion"
# endif
#endif


typedef void (*fakechroot_wrapperfn_t)(void);

struct fakechroot_wrapper {
    fakechroot_wrapperfn_t func;
    fakechroot_wrapperfn_t nextfunc;
    const char *name;
};


extern char *preserve_env_list[];
extern const int preserve_env_list_count;

int fakechroot_debug (const char *, ...);
fakechroot_wrapperfn_t fakechroot_loadfunc (struct fakechroot_wrapper *);
int fakechroot_localdir (const char *);
int fakechroot_try_cmd_subst (char *, const char *, char *);


/* We don't want to define _BSD_SOURCE and _DEFAULT_SOURCE and include stdio.h */
#ifndef snprintf
int snprintf(char *, size_t, const char *, ...);
#endif

#endif
