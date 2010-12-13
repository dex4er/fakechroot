# check_func_argtypes.m4 - check correct return and arguments type for function
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_CHECK_FUNC_RAW_DECL(FUNCTION, PROLOGUE)
# -------------------------------------
AC_DEFUN([ACX_CHECK_FUNC_RAW_DECL],
    [AH_TEMPLATE([HAVE_RAW_DECL_]AS_TR_CPP($1),
            [Define to 1 if `$1' is declared even after undefining macros.])
        AS_VAR_PUSHDEF([acx_var], [acx_cv_have_raw_decl_$1])
        AC_CACHE_CHECK([whether $1 is declared without a macro],
            acx_var,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$2], [
@%:@undef $1
(void) $1;
                    ])],
            [AS_VAR_SET(acx_var, [yes])], [AS_VAR_SET(acx_var, [no])])])
        AS_VAR_IF(acx_var, [yes],
            [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_RAW_DECL_$1]), [1])
                AS_VAR_SET(acx_var, [yes])])
        AS_VAR_POPDEF([acx_var])
])
