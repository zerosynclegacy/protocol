AC_INIT([zerosync], [1.0.0], [mail@zerosync.org])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
])

#
# Libtool -version-info (ABI version)
#
# Currently 1.0.0 ("stable"). Don't change this unless you
# know exactly what you're doing and have read and understand
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
#
# libzsync -version-info
LTVER="1:0:0"
AC_SUBST(LTVER)

# Checks for programs
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CC_C_O
AC_LIBTOOL_WIN32_DLL
AM_PROG_AR
AC_PROG_LIBTOOL


# Checks libraries
AC_CHECK_LIB([zmq], [zmq_init], [], 
             [AC_MSG_ERROR(["Cannot link -lzmq, install zeromq libzmq"])])

AC_CHECK_LIB([czmq], [zctx_new], [],
             [AC_MSG_ERROR(["Cannot link -lczmq, install zeromq czmq"])])

AC_OUTPUT
