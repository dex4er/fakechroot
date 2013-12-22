# prog_prove_opt.m4 - check if prove accepts command line option
#
# Copyright (c) 2013 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_PROG_PROVE_OPT([OPTS], [BODY], [ON-FALSE])
# ----------------------------------------------
AC_DEFUN([ACX_PROG_PROVE_OPT],
    [m4_define([myname], [PROVE_HAVE_OPT_$1])
        AC_CHECK_PROGS([PROVE], [prove])
        AS_VAR_PUSHDEF([acx_var], [acx_cv_prog_prove_opt_$1])
        AC_CACHE_CHECK([whether $PROVE accepts $1],
            acx_var,
            [_ACX_PROG_PROVE_TRY([$1], [$2],
                [AS_VAR_SET(acx_var, [yes])], [AS_VAR_SET(acx_var, [no])])])
        AS_VAR_IF(acx_var, [yes],
            [AS_VAR_APPEND([PROVEFLAGS], [" $1"])
                AC_SUBST([PROVEFLAGS])
                AC_SUBST(AS_TR_CPP(myname), [true])
                AS_VAR_SET(acx_var, [yes])],
            [$3])
        AS_VAR_POPDEF([acx_var])
        m4_undefine([myname])])

# _ACX_LANG_CONFTEST_PL([BODY])
# -----------------------------
m4_define([_ACX_LANG_CONFTEST_PL],
[cat <<_ACEOF >conftest.pl
$1
_ACEOF])

# _ACX_PROG_PROVE_TRY([OPTS], [BODY], [IF-WORKS], [IF-NOT])
# ---------------------------------------------------------
m4_define([_ACX_PROG_PROVE_TRY],
    [_ACX_LANG_CONFTEST_PL(m4_default([$2], [
printf "1..1\n";
printf "ok 1\n";
exit 0;
]))
        ac_try='$PROVE $1 conftest.pl >&AS_MESSAGE_LOG_FD'
        AS_IF([_AC_DO_VAR(ac_try)], [$3], [$4])])
