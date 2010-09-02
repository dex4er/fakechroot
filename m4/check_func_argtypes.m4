dnl  AC_FUNC_BIND_ARGTYPES
dnl  ---------------------
dnl  Determine the correct type to be passed to each of the `bind'
dnl  function's arguments, and define those types in `BIND_TYPE_RETURN',
dnl  `BIND_TYPE_ARG1', `BIND_TYPE_ARG2', and `BIND_TYPE_ARG3'.

# AH_CHECK_FUNC_ARGTYPES(FUNCTION-NAME, TYPES-RETURN, [TYPES-ARG1, TYPES_ARG2, ...])
# ----------------------------------------------------------------------------------
m4_define([AH_CHECK_FUNC_ARGTYPES],
[m4_foreach([mytype], [$2],
  [m4_define([myname], m4_join([], [HAVE_], $1, [_], mytype, [_TYPE_RETURN]))
   AH_TEMPLATE(AS_TR_CPP([myname]), [Define to 1 if the type of return value for `$1' is `mytype'])
 ])
 m4_if(m4_cmp(m4_count($@), 2), [1],
 [m4_for([myargn], [3], m4_count($@), [1],
  [m4_foreach([mytype], m4_argn(myargn, $@),
   [m4_define([myargi], m4_eval(myargn - 2))
    m4_define([myname], m4_join([], [HAVE_], $1, [_], mytype, [_TYPE_ARG], myargi))
    AH_TEMPLATE(AS_TR_CPP([myname]), [Define to 1 if the type of return value for `$1' is `mytype'])
   ])
  ])
 ])
])

# ac_check_func_argtypes_quote(STRING)
# ------------------------------------
m4_define([ac_check_func_argtypes_quote], [m4_join([], ['], AS_ESCAPE($1, ['']), ['], [ ])])

# AC_CHECK_FUNC_ARGTYPES(FUNCTION-NAME, TYPES-DEFAULT, TYPES-RETURN, TYPES-ARG1, TYPES-ARG2, ...])
# ------------------------------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_FUNC_ARGTYPES],
[AH_CHECK_FUNC_ARGTYPES([$1], m4_shift2($@))
 AC_CACHE_CHECK([types of arguments for myfunc],
  [ac_cv_func_$1_args],
  [m4_fatal(
   m4_join([ ], [for ac_return in], m4_map([ac_check_func_argtypes_quote], [$3]), [; do])
   m4_for([myargn], [4], m4_count($@), [1],
    [m4_define([myacvar], m4_join([], [ac_arg], m4_eval(myargn - 3)))
     m4_join([ ], [for], myacvar, [in], m4_map([ac_check_func_argtypes_quote], m4_argn(myargn, $@)), [; do])
   ])
   AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM(
    [AC_INCLUDES_DEFAULT],
     [m4_for([myargn], [4], m4_count($@), [1],
      [m4_define([myacvar], m4_join([], [ac_arg], m4_eval(myargn - 3)))
       m4_define([myargvar], m4_join([], [arg], m4_eval(myargn - 3)))
m4_join([], [#define ], myacvar, [(name) $], myacvar)
     ])]
m4_join([], [extern $ac_return bind (], [ac_arg1(myargvar), ac_arg2(myargvar), ac_arg3(myargvar));])
   )],
   [ac_cv_func_bind_args="$ac_return;$ac_arg1;$ac_arg2;$ac_arg3"; break 4])
# Provide a safe default value.
: ${ac_cv_func_bind_args='(default) int;int name;const struct sockaddr *name;socklen_t name'}
])
ac_save_IFS=$IFS; IFS=';'
set dummy `echo "$ac_cv_func_bind_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
IFS=$ac_save_IFS
shift
AC_DEFINE_UNQUOTED(BIND_TYPE_RETURN, $[1],
                   [Define to the type of return value for `bind'.])
AC_DEFINE_UNQUOTED(AS_TR_CPP([BIND_TYPE_RETURN_$[1]]), [1])
AC_DEFINE_UNQUOTED(BIND_TYPE_ARG1(name), $[2],
                   [Define to the type of arg 1 for `bind'.])
AC_DEFINE_UNQUOTED(AS_TR_CPP([BIND_TYPE_ARG1_$[2]]), [1])
AC_DEFINE_UNQUOTED(BIND_TYPE_ARG2(name), $[3],
                   [Define to the type of arg 2 for `bind'.])
AC_DEFINE_UNQUOTED(AS_TR_CPP([BIND_TYPE_ARG2_$[3]]), [1])
AC_DEFINE_UNQUOTED(BIND_TYPE_ARG3(name), $[4],
                   [Define to the type of arg 3 for `bind'.])
AC_DEFINE_UNQUOTED(AS_TR_CPP([BIND_TYPE_ARG3_$[4]]), [1])
rm -f conftest*
])
