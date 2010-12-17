# check_c_alignof.m4 - check if C compiler supports __alignof__ keyword
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_CHECK_C_ALIGNOF([HEADER])
# -------------------------------------------
AC_DEFUN([ACX_CHECK_C_ALIGNOF],
    [m4_define([myname], [HAVE___ALIGNOF__])
        AH_TEMPLATE(AS_TR_CPP(myname),
            [Define to 1 if compiler supports `__alignof__' syntax.])
        AS_VAR_PUSHDEF([acx_var], [acx_cv_c_alignof])
        AC_CACHE_CHECK([whether compiler supports __alignof__ syntax],
            acx_var,
            [AC_LINK_IFELSE([AC_LANG_PROGRAM([
$1
@%:@include <stdio.h>
                    ], [
int a = __alignof__(char);
                    ])],
            [AS_VAR_SET(acx_var, [yes])], [AS_VAR_SET(acx_var, [no])])])
        AS_VAR_IF(acx_var, [yes],
            [AC_DEFINE_UNQUOTED(AS_TR_CPP(myname), [1])
                AS_VAR_SET(acx_var, [yes])])
        AS_VAR_POPDEF([acx_var])
        m4_undefine([myname])
])
