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
   AH_TEMPLATE(AS_TR_CPP([myname]), m4_join([], [Define to 1 if the type of return value for `$1' is `], mytype, [']))
 ])
 m4_if(m4_cmp(m4_count($@), 2), [1],
 [m4_for([myargn], [3], m4_count($@), [1],
  [m4_foreach([mytype], m4_argn(myargn, $@),
   [m4_define([myargi], m4_eval(myargn - 2))
    m4_define([myname], m4_join([], [HAVE_], $1, [_], mytype, [_TYPE_ARG], myargi))
    AH_TEMPLATE(AS_TR_CPP([myname]), m4_join([], [Define to 1 if the type of return value for `$1' is `], mytype, [']))
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
 AC_CACHE_CHECK([types of arguments for $1],
  [ac_cv_func_$1_args],
  [m4_join([ ], [for ac_return in], m4_map([ac_check_func_argtypes_quote], [$3]), [; do])
   m4_define([myarglist])
   m4_define([myacarglist])
   m4_define([myfuncarglist])
   m4_for([myargn], [4], m4_count($@), [1],
    [m4_define([myacvar], m4_join([], [ac_arg], m4_eval(myargn - 3)))
     m4_define([myacarglist], m4_make_list(myacarglist, myacvar))
     m4_define([myarglist], m4_make_list(myarglist, m4_join([], myacvar, [(arg], m4_eval(myargn - 3), [)])))
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
     m4_join([], [extern $ac_return bind (], m4_join([, ], myarglist),[);])
   )],
   [m4_define([myaccvargs], m4_if(m4_cmp(m4_count(myacarglist), 0), [1],
    m4_join([], [;$], m4_join([;$], myacarglist))))
    m4_join([], [ac_cv_func_$1_args="$ac_return], myaccvargs, ["; break ], m4_eval(m4_count($@) - 2))])
   m4_for([myargn], [3], m4_count($@), [1], m4_echo(
   [done
   ]))
   m4_join([], [: ${ac_cv_func_$1_args='(default) ], m4_join([;], $2), ['}])
  ])
  ac_save_IFS=$IFS; IFS=';'
  set dummy `echo "$ac_cv_func_$1_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
  IFS=$ac_save_IFS
  shift
  AC_DEFINE_UNQUOTED(AS_TR_CPP([$1_TYPE_RETURN]), $[1],
  m4_join([], [Define to the type of return value for `], $1, ['.]))
  AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1_$[1]_TYPE_RETURN]), [1])
  m4_if(m4_cmp(m4_count($@), 3), [1],
  [m4_for([myargn], [4], m4_count($@), [1],
   [m4_define([myargi], m4_eval(myargn - 3))
    AC_DEFINE_UNQUOTED(m4_join([], AS_TR_CPP(m4_join([], $1, [_TYPE_ARG], myargi)), [(name)]), m4_join([], [$], m4_eval(myargi + 1)),
     m4_join([], [Define to the type of arg ], myargi, [ for `], $1, ['.]))
    AC_DEFINE_UNQUOTED(AS_TR_CPP(m4_join([], [HAVE_$1_$], m4_eval(myargi + 1), [_TYPE_ARG], myargi)), [1])
    rm -f conftest*
   ]
  )])
])
