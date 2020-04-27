dnl @synopsis AC_CXX_NAMESPACES
dnl
dnl If the compiler can prevent names clashes using namespaces, define
dnl HAVE_NAMESPACES.
dnl
dnl @version $Id: ac_cxx_namespaces.m4,v 1.1.1.1 2005/07/17 23:22:40 shini Exp $
dnl @author Luc Maisonobe
dnl
AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK([whether the compiler implements namespaces],
[ac_cv_cxx_namespaces],
[AC_LANG_PUSH([C++])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                    [namespace Outer { namespace Inner { int i = 0; }}],
                    [using namespace Outer::Inner; return i;])],
 [ac_cv_cxx_namespaces=yes], [ac_cv_cxx_namespaces=no])
 AC_LANG_POP([C++])
])
AS_IF([test "$ac_cv_cxx_namespaces" = yes],
  [AC_DEFINE(HAVE_NAMESPACES,1,
        [define if the compiler implements namespaces])])])
