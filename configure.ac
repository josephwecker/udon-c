AC_INIT([udon], [0.0.1], [joseph.wecker@gmail.com])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.build-aux])
AM_INIT_AUTOMAKE([gnu -Wall -Werror])
LT_INIT
AC_PROG_CC
AC_CONFIG_SRCDIR([lib/udon.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile lib/Makefile src/Makefile])
AC_FUNC_MALLOC
CFLAGS='-O3 -Wall -W -Winline -Wno-unused-label'
AC_OUTPUT
