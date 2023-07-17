AC_INIT([librtas],[2.0.4],[See README])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR([librtas_src/librtas.h])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability subdir-objects foreign tar-ustar])

AC_CONFIG_HEADERS([config.h])

AC_CHECK_FUNCS([memset munmap strrchr])
AC_CHECK_HEADERS([fcntl.h])
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_CHECK_PROGS([cmd_abidiff], [abidiff])

LT_INIT
LT_LANG([C])

# Detection of missing pkg-config macros, per Autoconf manual.
m4_pattern_forbid([^PKG_])

PKG_PROG_PKG_CONFIG

AC_MSG_CHECKING([whether to enable tests])
AC_ARG_ENABLE([tests],
	      [AS_HELP_STRING([--enable-tests],
	                      [enable tests (default is no)])],
	      [enable_tests=$enableval],
	      [enable_tests="no"])
AC_MSG_RESULT([$enable_tests])
AM_CONDITIONAL([ENABLE_TESTS], [test "$enable_tests" = "yes"])
if test "$enable_tests" = "yes"; then
    cmocka_min_ver=1.1.7
    PKG_CHECK_MODULES([CMOCKA],
                      [cmocka >= $cmocka_min_ver],
                      [cmocka_available="yes"],
                      [cmocka_available="no"])
    if test "$cmocka_available" = "no"; then
        AC_MSG_ERROR([--enable-tests requires cmocka >= $cmocka_min_ver])
    fi
fi

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile librtas.spec])
AC_OUTPUT
