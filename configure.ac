AC_INIT([zerosync], [1.0.0], [mail@zerosync.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
])

# Checks for programs
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CC_C_O

# Checks libraries
AC_CHECK_LIB([zmq], [zmq_init], [], 
             [AC_MSG_ERROR(["Cannot link -lzmq, install zeromq libzmq"])])w

AC_CHECK_LIB([czmq], [zctx_new], [],
             [AC_MSG_ERROR(["Cannot link -lczmq, install zeromq czmq"])])

AC_OUTPUT
