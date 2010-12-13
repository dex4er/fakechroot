# check_c_attribute_section.m4 - check if C compiler supports section attribute
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_CHECK_C_ATTRIBUTE_SECTION([SECTION], [HEADER])
# -------------------------------------------
AC_DEFUN([ACX_CHECK_C_ATTRIBUTE_SECTION],
    [m4_define([mysect], m4_default([$1], [data]))
        m4_define([myname], [HAVE___ATTRIBUTE__SECTION_$1])
        AH_TEMPLATE(AS_TR_CPP(myname),
            [Define to 1 if compiler supports `__attribute__((section("]mysect[")))' syntax.])
        AS_VAR_PUSHDEF([acx_var], [acx_cv_c_attribute_section_]mysect)
        AC_CACHE_CHECK([whether compiler supports __attribute__((section("]mysect["))) syntax],
            acx_var,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
$2
int foo __attribute__ ((section ("]mysect[")));
                    ], [])],
            [AS_VAR_SET(acx_var, [yes])], [AS_VAR_SET(acx_var, [no])])])
        AS_VAR_IF(acx_var, [yes],
            [AC_DEFINE_UNQUOTED(AS_TR_CPP(myname), [1])
                AS_VAR_SET(acx_var, [yes])])
        AS_VAR_POPDEF([acx_var])
        m4_undefine([mysect])
        m4_undefine([myname])
])
