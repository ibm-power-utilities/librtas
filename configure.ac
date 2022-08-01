AC_INIT([librtas],[2.0.3],[See README])
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

LT_INIT
LT_LANG([C])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile librtas.spec])
AC_OUTPUT
