# check_func_argtypes.m4 - check correct return and arguments type for function
#
# Copyright (c) 2005, 2008, 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.


# ACX_CHECK_FUNC_ARGTYPES(FUNCTION-NAME, PROLOGUE, HEADER-FILES, TYPES-DEFAULT,
#     TYPES-RETURN, TYPES-ARG1, TYPES-ARG2, ...])
# ----------------------------------------------------------------------------
# Determine the correct type to be passed to each of the FUNCTION-NAME
# function's arguments, and define those types in `func_TYPE_RETURN',
# `HAVE_func_type_TYPE_RETURN', `func_TYPE_ARG1', `HAVE_func_type_TYPE_ARG1',
# etc. variables.
#
# Example:
#
#   ACX_CHECK_FUNC_ARGTYPES([readlink],
#       [
#   #define _BSD_SOURCE 1
#   #define _DEFAULT_SOURCE 1
#       ], [unistd.h],
#       [[ssize_t], [const char *_], [char *_], [size_t _]],
#       [[ssize_t], [int]],
#       [[const char *_]],
#       [[char *_]],
#       [[size_t _], [int _]])


AC_DEFUN([ACX_CHECK_FUNC_ARGTYPES],
    [_AHX_CHECK_FUNC_ARGTYPES([$1], m4_shiftn(4, $@))
        AC_CHECK_HEADERS([$3])
        AC_CACHE_CHECK([types of arguments for $1],
            [acx_cv_func_$1_args],
                [[for acx_return in ]m4_map([_ACX_CHECK_FUNC_ARGTYPES_QUOTE], [$5]), [; do ]
                    m4_define([myarglist])
                    m4_define([myacarglist])
                    m4_define([myfuncarglist])
                    m4_for([myargn], [6], m4_count($@), [1],
                        [m4_define([myacvar], [acx_arg]m4_eval(myargn - 5))
                            m4_define([myacarglist], m4_make_list(myacarglist, myacvar))
                            m4_define([myarglist], m4_make_list(myarglist, myacvar[(arg]m4_eval(myargn - 5)[)]))
                            m4_join([ ], [for], myacvar, [in], m4_map([_ACX_CHECK_FUNC_ARGTYPES_QUOTE], m4_argn(myargn, $@)), [; do ])
                            m4_undefine([myacvar])])
                    AC_COMPILE_IFELSE(
                        [AC_LANG_PROGRAM(
                                [ACX_INCLUDES_HEADERS([$3], [$2])],
                                [m4_for([myargn], [6], m4_count($@), [1],
                                    [m4_define([myacvar], [acx_arg]m4_eval(myargn - 5))
                                        [@%:@define ]myacvar[(_) $]myacvar
                                        m4_undefine([myacvar])])]
                                [extern $acx_return $1 (]m4_join([, ], myarglist)[);])],
                            [m4_define([myaccvargs], m4_if(m4_cmp(m4_count(myacarglist), 0), [1],
                                [;$]m4_join([;$], myacarglist)))
                                [acx_cv_func_$1_args="$acx_return]myaccvargs["; break ]m4_eval(m4_count($@) - 4)
                                m4_undefine([myaccvargs])])
                    m4_for([myargn], [5], m4_count($@), [1], [
                        done])
                    [: ${acx_cv_func_$1_args='(default) ]m4_join([;], $4)['}]
                    m4_undefine([myarglist])
                    m4_undefine([myacarglist])
                    m4_undefine([myfuncarglist])])
        [
            acx_save_IFS=$IFS; IFS=';'
            set dummy `echo "$acx_cv_func_$1_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
            IFS=$acx_save_IFS
            shift
            ]
        AC_DEFINE_UNQUOTED(AS_TR_CPP([$1_TYPE_RETURN]), $[1],
            [Define to the type of return value for `$1'.])
        AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1_TYPE_RETURN_$[1]]), [1])
        m4_if(m4_cmp(m4_count($@), 5), [1],
            [m4_for([myargn], [6], m4_count($@), [1],
                    [m4_define([myargi], m4_eval(myargn - 5))
                        AC_DEFINE_UNQUOTED(AS_TR_CPP([$1_TYPE_ARG]myargi)[(_)], [$]m4_eval(myargi + 1),
                            [Define to the type of arg ]myargi[ for `$1'.])
                        AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1_TYPE_ARG]myargi[_$]m4_eval(myargi + 1)), [1])
                        [rm -f conftest*]
                        m4_undefine([myargi])])])])


# Compatibility with older autoconf
m4_ifndef([m4_argn],
        [m4_define([m4_argn],
                [m4_car(m4_shiftn([$1], $@))])])


# _AHX_CHECK_FUNC_ARGTYPES(FUNCTION-NAME, TYPES-RETURN, [TYPES-ARG1,
#     TYPES_ARG2, ...])
# -----------------------------------------------------------------
m4_define([_AHX_CHECK_FUNC_ARGTYPES],
    [m4_foreach([mytype], [$2],
            [m4_define([myname], [HAVE_$1_TYPE_RETURN_]mytype)
                AH_TEMPLATE(AS_TR_CPP([myname]), [Define to 1 if the type of return value for `$1' is `]mytype['])
                m4_undefine([myname])])
        m4_if(m4_cmp(m4_count($@), 2), [1],
                [m4_for([myargn], [3], m4_count($@), [1],
                        [m4_foreach([mytype], m4_argn(myargn, $@),
                            [m4_define([myargi], m4_eval(myargn - 2))
                                m4_define([myname], [HAVE_$1_TYPE_ARG]myargi[_]mytype)
                                AH_TEMPLATE(AS_TR_CPP([myname]), [Define to 1 if the type of arg ]myargi[ for `$1' is `]mytype['])
                                m4_undefine([myargi])
                                m4_undefine([myname])])])])])


# _ACX_CHECK_FUNC_ARGTYPES_QUOTE(STRING)
# -------------------------------------
m4_define([_ACX_CHECK_FUNC_ARGTYPES_QUOTE], [']AS_ESCAPE($1, [''])[' ])
