# check_opendir_calls_internally.m4 - check if opendir calls other function
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_CHECK_OPENDIR_CALLS_INTERNALLY(FUNCTION, [HEADER])
# -------------------------------------------
AC_DEFUN([ACX_CHECK_OPENDIR_CALLS_INTERNALLY],
    [m4_define([myname], [OPENDIR_CALLS_$1])
        AH_TEMPLATE(AS_TR_CPP(myname),
            [Define to 1 if opendir function calls $1 function internally.])
        AS_VAR_PUSHDEF([ac_var], [ac_cv_opendir_calls_internally_$1])
        AC_CACHE_CHECK([whether opendir function calls $1 function internally],
            ac_var,
            [AC_RUN_IFELSE([AC_LANG_PROGRAM([
$2
#include <stdlib.h>
#include <dirent.h>

void $1() {
    exit(0);
}
                    ], [
opendir("/");
exit(1);
                    ])],
            [AS_VAR_SET(ac_var, [yes])], [AS_VAR_SET(ac_var, [no])])])
        AS_VAR_IF(ac_var, [yes],
            [AC_DEFINE_UNQUOTED(AS_TR_CPP(myname), [1])
                [ac_cv_opendir_calls_internally_$1=yes]])
        AS_VAR_POPDEF([ac_var])
        m4_undefine([myname])
])
