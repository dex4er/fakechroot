# check_c_attribute.m4 - check if C compiler supports __attribute__
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# AX_CHECK_C_ATTRIBUTE([ATTRIBUTE], [HEADER])
# -------------------------------------------
AC_DEFUN([AX_CHECK_C_ATTRIBUTE],
    [m4_define([myattr], m4_default([$1], [unused]))
        m4_define([myname], [HAVE___ATTRIBUTE__$1])
        AH_TEMPLATE(AS_TR_CPP(myname),
            [Define to 1 if compiler supports `__attribute__((]myattr[))' syntax.])
        AS_VAR_PUSHDEF([ac_var], [ac_cv_c_attribute_]myattr)
        AC_CACHE_CHECK([whether compiler supports __attribute__((]myattr[)) syntax],
            ac_var,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
$2
static void foo(void) __attribute__ ((]myattr[));
static void foo(void) {
    return 0;
}
                    ], [])],
            [AS_VAR_SET(ac_var, [yes])], [AS_VAR_SET(ac_var, [no])])])
        AS_VAR_IF(ac_var, [yes],
            [AC_DEFINE_UNQUOTED(AS_TR_CPP(myname), [1])
                [ac_cv_c_attribute_]myattr[=yes]])
        AS_VAR_POPDEF([ac_var])
        m4_undefine([myattr])
        m4_undefine([myname])
])
