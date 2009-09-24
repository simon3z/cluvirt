#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([cluvirtd], [0.1.0], [federico.simoncelli@gmail.com])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([cluvirtd/main.c], [cluvadm/main.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
LDFLAGS="$LDFLAGS -L${libdir}/openais"
AC_CHECK_LIB([cpg], [cpg_initialize],,
    [AC_MSG_ERROR([libcpg is missing])]
)
AC_CHECK_LIB([pthread], [pthread_create],,
    [AC_MSG_ERROR([libpthread is missing])]
)

# Checks for pkg-config modules.
PKG_CHECK_MODULES([xml], [libxml-2.0])
PKG_CHECK_MODULES([virt], [libvirt])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h syslog.h unistd.h])
AC_CHECK_HEADERS([openais/cpg.h],,
    [AC_MSG_ERROR([openais/cpg.h header is missing])]
)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([memset select strdup strtol])

AC_CONFIG_FILES([Makefile cluvirtd/Makefile cluvadm/Makefile])
AC_OUTPUT
