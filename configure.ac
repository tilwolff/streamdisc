#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(streamdisc, 0.1.0,)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/streamdisc_cgi.c])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB(dvdread,DVDOpen)
AC_CHECK_LIB(bluray,bd_open)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([socket strdup strtoull])

AC_OUTPUT(Makefile src/Makefile)
