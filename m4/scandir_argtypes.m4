dnl  AC_FUNC_SCANDIR_ARGTYPES
dnl  -------------------------
dnl  Determine the correct type to be passed to each of the `scandir'
dnl  function's arguments, and define those types in `SCANDIR_TYPE_ARG1',
dnl  `SCANDIR_TYPE_ARG2', `SCANDIR_TYPE_ARG3' and `SCANDIR_TYPE_ARG4'.
AN_FUNCTION([scandir], [AC_FUNC_SCANDIR_ARGTYPES])
AC_DEFUN([AC_FUNC_SCANDIR_ARGTYPES],
[AC_CHECK_HEADERS(dirent.h)
AC_CACHE_CHECK([types of arguments for scandir],
[ac_cv_func_scandir_args],
[for ac_arg1 in 'const char *dir'; do
 for ac_arg2 in 'struct dirent ***namelist'; do
  for ac_arg3 in 'int(*filter)(const struct dirent *)' 'int(*filter)(struct dirent *)'; do
   for ac_arg4 in 'int(*compar)(const void *,const void *)'; do
    AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#ifdef HAVE_UNISTD_H
#include <dirent.h>
#endif
],
			[extern int scandir ($ac_arg1,
					    $ac_arg2,
					    $ac_arg3,
					    $ac_arg4);])],
	      [ac_cv_func_scandir_args="$ac_arg1;$ac_arg2;$ac_arg3;$ac_arg4"; break 3])
   done
  done
 done
done
# Provide a safe default value.
: ${ac_cv_func_scandir_args='(default) const char *dir;struct dirent ***namelist;int(*filter)(const struct dirent *);int(*compar)(const void *,const void *)'}
])
ac_save_IFS=$IFS; IFS=';'
set dummy `echo "$ac_cv_func_scandir_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
IFS=$ac_save_IFS
shift
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG1, $[1],
		   [Define to the type of arg 1 for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG2, $[2],
		   [Define to the type of arg 2 for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG3, $[3],
		   [Define to the type of arg 3 for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG4, $[4],
		   [Define to the type of arg 4 for `scandir'.])
rm -f conftest*
])
