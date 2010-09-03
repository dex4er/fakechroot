# check_func_argtypes.m4 - check correct return and arguments type for function
#
# Copyright (c) 2005, 2008, 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# _AH_CHECK_FUNC_ARGTYPES(FUNCTION-NAME, TYPES-RETURN, [TYPES-ARG1,
#     TYPES_ARG2, ...])
# -----------------------------------------------------------------
m4_define([_AH_CHECK_FUNC_ARGTYPES],
    [m4_foreach([mytype], [$2],
            [m4_define([myname], m4_join([], [HAVE_], $1, [_TYPE_RETURN_], mytype))
                AH_TEMPLATE(AS_TR_CPP([myname]), m4_join([], [Define to 1 if the type of return value for `$1' is `], mytype, [']))])
        m4_if(m4_cmp(m4_count($@), 2), [1],
                [m4_for([myargn], [3], m4_count($@), [1],
                        [m4_foreach([mytype], m4_argn(myargn, $@),
                            [m4_define([myargi], m4_eval(myargn - 2))
                                m4_define([myname], m4_join([], [HAVE_], $1, [_TYPE_ARG], myargi, [_], mytype))
                                AH_TEMPLATE(AS_TR_CPP([myname]), m4_join([], [Define to 1 if the type of arg ], myargi, [ for `$1' is `], mytype, [']))])])])])

# _AC_CHECK_FUNC_ARGTYPES_QUOTE(STRING)
# -------------------------------------
m4_define([_AC_CHECK_FUNC_ARGTYPES_QUOTE], [m4_join([], ['], AS_ESCAPE($1, ['']), ['], [ ])])

# AC_CHECK_FUNC_ARGTYPES(FUNCTION-NAME, HEADER-FILES, PROLOGUE, TYPES-DEFAULT,
#     TYPES-RETURN, TYPES-ARG1, TYPES-ARG2, ...])
# ----------------------------------------------------------------------------
# Determine the correct type to be passed to each of the FUNCTION-NAME
# function's arguments, and define those types in `func_TYPE_RETURN',
# `HAVE_func_type_TYPE_RETURN', `func_TYPE_ARG1', `HAVE_func_type_TYPE_ARG1',
# etc. variables.
AC_DEFUN([AC_CHECK_FUNC_ARGTYPES],
    [_AH_CHECK_FUNC_ARGTYPES([$1], m4_shift4($@))
        AC_CHECK_HEADERS([$2])
        AC_CACHE_CHECK([types of arguments for $1],
            [ac_cv_func_$1_args],
                [m4_join([ ], [for ac_return in], m4_map([_AC_CHECK_FUNC_ARGTYPES_QUOTE], [$5]), [; do])
                    m4_define([myarglist])
                    m4_define([myacarglist])
                    m4_define([myfuncarglist])
                    m4_for([myargn], [6], m4_count($@), [1],
                        [m4_define([myacvar], m4_join([], [ac_arg], m4_eval(myargn - 5)))
                            m4_define([myacarglist], m4_make_list(myacarglist, myacvar))
                            m4_define([myarglist], m4_make_list(myarglist, m4_join([], myacvar, [(arg], m4_eval(myargn - 5), [)])))
                            m4_join([ ], [for], myacvar, [in], m4_map([_AC_CHECK_FUNC_ARGTYPES_QUOTE], m4_argn(myargn, $@)), [; do ])])
                    AC_COMPILE_IFELSE(
                        [AC_LANG_PROGRAM(
                                [AC_INCLUDES_HEADERS([$2], [$3])],
                                [m4_for([myargn], [6], m4_count($@), [1],
                                    [m4_define([myacvar], m4_join([], [ac_arg], m4_eval(myargn - 5)))
                                        m4_define([myargvar], m4_join([], [arg], m4_eval(myargn - 5)))
                                        m4_join([], [#define ],
                                            myacvar, [(_) $], myacvar
                                            )])]
                                m4_join([], [extern $ac_return bind (], m4_join([, ], myarglist),[);]))],
                            [m4_define([myaccvargs], m4_if(m4_cmp(m4_count(myacarglist), 0), [1],
                                m4_join([], [;$], m4_join([;$], myacarglist))))
                                m4_join([], [ac_cv_func_$1_args="$ac_return], myaccvargs, ["; break ], m4_eval(m4_count($@) - 4))])
                    m4_for([myargn], [5], m4_count($@), [1], m4_echo([done; ]))
                    m4_join([], [: ${ac_cv_func_$1_args='(default) ], m4_join([;], $4), ['}])])
        m4_echo([
            ac_save_IFS=$IFS; IFS=';'
            set dummy `echo "$ac_cv_func_$1_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
            IFS=$ac_save_IFS
            shift
            ])
        AC_DEFINE_UNQUOTED(AS_TR_CPP([$1_TYPE_RETURN]), $[1],
            m4_join([], [Define to the type of return value for `], $1, ['.]))
        AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1_TYPE_RETURN_$[1]]), [1])
        m4_if(m4_cmp(m4_count($@), 5), [1],
            [m4_for([myargn], [6], m4_count($@), [1],
                    [m4_define([myargi], m4_eval(myargn - 5))
                        AC_DEFINE_UNQUOTED(m4_join([], AS_TR_CPP(m4_join([], $1, [_TYPE_ARG], myargi)), [(_)]), m4_join([], [$], m4_eval(myargi + 1)),
                            m4_join([], [Define to the type of arg ], myargi, [ for `], $1, ['.]))
                        AC_DEFINE_UNQUOTED(AS_TR_CPP(m4_join([], [HAVE_$1_TYPE_ARG], myargi, [_$], m4_eval(myargi + 1))), [1])
                        m4_echo([rm -f conftest*])])])])
