AC_INIT([xbee-comm], [2011.11.07], [roysjosh@gmail.com])
AC_PREREQ([2.61])
AM_INIT_AUTOMAKE([-Wall foreign dist-bzip2])
#LT_INIT([disable-static])
AC_CONFIG_SRCDIR([src/lib/xb_ctx.c])
AC_CONFIG_HEADERS([config.h])
#AC_SUBST([LIBTOOL_DEPS])

# Checks for programs.
#AC_PROG_CXX
AC_PROG_CC

AC_LANG([C])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_SELECT_ARGTYPES

AC_CONFIG_FILES([
  Makefile
  src/bin/Makefile
])
AC_OUTPUT
