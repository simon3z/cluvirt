#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([cluvirt], [0.0.90], [federico.simoncelli@nethesis.it])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Configure default CFLAGS.
CFLAGS="-g -O2 -Wall -Wshadow -Wconversion"

# Checks for x86_64 lib path.
if test "$libdir" = "\${exec_prefix}/lib"; then
  if test -e "/usr/lib64"; then
	 libdir="/usr/lib64"
  else
	 libdir="/usr/lib"
  fi   
fi

# Checks for libraries.
LDFLAGS="$LDFLAGS -L${libdir}/openais"
AC_CHECK_LIB([pthread], [pthread_create],,
    [AC_MSG_ERROR([libpthread is missing])]
)
AC_CHECK_LIB([cpg], [cpg_initialize],,
    [AC_MSG_ERROR([libcpg is missing])]
)
AC_CHECK_LIB([cman], [cman_init],,
    [AC_MSG_ERROR([libcman is missing])]
)

# Checks for pkg-config modules.
PKG_CHECK_MODULES([xml], [libxml-2.0])
PKG_CHECK_MODULES([virt], [libvirt])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h \
                  sys/socket.h sys/time.h syslog.h unistd.h])

AC_CHECK_HEADERS([corosync/cpg.h],
    [AC_SUBST([CPG_HEADER_PATH], [corosync/cpg.h])],
    [AC_CHECK_HEADERS([openais/cpg.h],
	[AC_SUBST([CPG_HEADER_PATH], [openais/cpg.h])],
        [AC_MSG_ERROR([cpg.h header is missing])])])

AC_CHECK_HEADERS([libcman.h],,
    [AC_MSG_ERROR([libcman.h header is missing])]
)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([dup2 gettimeofday memset select socket strdup])
AC_FUNC_FORK

# Output files.
AC_CONFIG_FILES([Makefile
                 utils/Makefile
                 lib/Makefile 
                 lib/cluvirt.h
                 daemon/Makefile 
                 client/Makefile 
                 man/Makefile
                 tests/Makefile
                 cluvirt.spec
                 cluvirtd.init
])

AC_OUTPUT
