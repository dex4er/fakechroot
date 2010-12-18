# check_c_attribute.m4 - check if C compiler supports __attribute__
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_CHECK_C_ATTRIBUTE([ATTRIBUTE], [HEADER])
# -------------------------------------------
AC_DEFUN([ACX_CHECK_C_ATTRIBUTE],
    [m4_define([myattr], m4_default([$1], [unused]))
        m4_define([myname], [HAVE___ATTRIBUTE__$1])
        AH_TEMPLATE(AS_TR_CPP(myname),
            [Define to 1 if compiler supports `__attribute__((]myattr[))' syntax.])
        AS_VAR_PUSHDEF([acx_var], [acx_cv_c_attribute_]myattr)
        AC_CACHE_CHECK([whether compiler supports __attribute__((]myattr[)) syntax],
            acx_var,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
$2
static int foo(void) __attribute__ ((]myattr[));
static int foo(void) {
    return 0;
}
                    ], [])],
            [AS_VAR_SET(acx_var, [yes])], [AS_VAR_SET(acx_var, [no])])])
        AS_VAR_IF(acx_var, [yes],
            [AC_DEFINE_UNQUOTED(AS_TR_CPP(myname), [1])
                AS_VAR_SET(acx_var, [yes])])
        AS_VAR_POPDEF([acx_var])
        m4_undefine([myattr])
        m4_undefine([myname])
])
