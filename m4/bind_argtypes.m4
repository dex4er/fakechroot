dnl  AC_FUNC_BIND_ARGTYPES
dnl  ---------------------
dnl  Determine the correct type to be passed to each of the `bind'
dnl  function's arguments, and define those types in `BIND_TYPE_RETURN',
dnl  `BIND_TYPE_ARG1', `BIND_TYPE_ARG2', and `BIND_TYPE_ARG3'.
AN_FUNCTION([bind], [AC_FUNC_BIND_ARGTYPES])

m4_define([_AC_CHECK_FUNC_ARGTYPES3],
[m4_foreach([vartype], [$1],
 [m4_define([varname], [BIND_TYPE_RETURN_])
  m4_append([varname], vartype)
  AH_TEMPLATE(AS_TR_CPP([varname]), [Define to 1 if the type of return value for `bind' is `vartype'])
])
m4_foreach([vartype], [m4_split([$2],[;])],
 [m4_define([varname], [BIND_TYPE_ARG1_])
  m4_append([varname], vartype)
  AH_TEMPLATE(AS_TR_CPP([varname]), [Define to 1 if the type of arg 1 for `bind' is `vartype'])
])
m4_foreach([vartype], [$3],
 [m4_define([varname], [BIND_TYPE_ARG2_])
  m4_append([varname], vartype)
  AH_TEMPLATE(AS_TR_CPP([varname]), [Define to 1 if the type of arg 2 for `bind' is `vartype'])
])
m4_foreach([vartype], [$4],
 [m4_define([varname], [BIND_TYPE_ARG3_])
  m4_append([varname], vartype)
  AH_TEMPLATE(AS_TR_CPP([varname]), [Define to 1 if the type of arg 3 for `bind' is `vartype'])
])])

AC_DEFUN([AC_FUNC_BIND_ARGTYPES],
[AC_CHECK_HEADERS([sys/socket.h])
_AC_CHECK_FUNC_ARGTYPES3([[int]], [[int name]], [[__CONST_SOCKADDR_ARG name], [const struct sockaddr *name]], [[socklen_t name]])
AC_CACHE_CHECK([types of arguments for bind],
[ac_cv_func_bind_args],
[for ac_return in 'int'; do
 for ac_arg1 in 'int name'; do
  for ac_arg2 in '__CONST_SOCKADDR_ARG name' 'const struct sockaddr *name'; do
   for ac_arg3 in 'socklen_t name'; do
   AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#include <sys/socket.h>
],
            [
#define ac_arg1(name) $ac_arg1
#define ac_arg2(name) $ac_arg2
#define ac_arg3(name) $ac_arg3
extern $ac_return bind (ac_arg1(fd), ac_arg2(addr), ac_arg3(len));])],
            [ac_cv_func_bind_args="$ac_return;$ac_arg1;$ac_arg2;$ac_arg3"; break 4])
   done
  done
 done
done
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
