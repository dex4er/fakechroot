# check_fts_name_type.m4 - check the type for struct _ftsent.fts_name
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_CHECK_FTS_NAME_TYPE([PROLOG])
# ---------------------------------
AC_DEFUN([ACX_CHECK_FTS_NAME_TYPE],
    [AHX_CHECK_FTS_NAME_TYPE
        AC_CHECK_MEMBERS([struct _ftsent.fts_name],
            [m4_define([mytype], [char *])
               m4_define([myname], [HAVE_STRUCT__FTSENT_FTS_NAME_TYPE_]mytype)
                AS_VAR_PUSHDEF([acx_var], [acx_cv_struct__ftsent_fts_name_type_]mytype)
                AC_CACHE_CHECK([whether type of struct _ftsent.fts_name is ]mytype,
                    acx_var,
                    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                        [ACX_INCLUDES_HEADERS([sys/types.h sys/stat.h fts.h], [$1])
                        ], [
FTSENT foo;
foo.fts_name = (]mytype[)0;
                        ])],
                    [AS_VAR_SET(acx_var, [yes])], [AS_VAR_SET(acx_var, [no])])])
                AS_VAR_IF(acx_var, [yes],
                    [AC_DEFINE_UNQUOTED(AS_TR_CPP(myname), [1])
                        AS_VAR_SET(acx_var, [yes])])
                AS_VAR_POPDEF([acx_var])
                m4_undefine([myname])
                m4_undefine([mytype])
            ],, ACX_INCLUDES_HEADERS([sys/types.h fts.h]))])


AC_DEFUN([AHX_CHECK_FTS_NAME_TYPE],
    [m4_define([mytype], [char *])
        m4_define([myname], [HAVE_STRUCT__FTSENT_FTS_NAME_TYPE_]mytype)
            AH_TEMPLATE(AS_TR_CPP([myname]), [Define to 1 if the type of struct _ftsent.fts_name is `]mytype['])
            m4_undefine([myname])
            m4_undefine([mytype])])
