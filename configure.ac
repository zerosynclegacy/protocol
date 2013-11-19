AC_INIT([zerosync], [1.0.0], [mail@zerosync.org])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

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

# Checks for typedefs, structures, and compiler characteristics.


# Determine whether or not documentation should be built and installed.
zerosync_build_doc="yes"
zerosync_install_man="yes"

# Check for asciidoc and xmlto and don't build the docs if these are not installed.
AC_CHECK_PROG(zerosync_have_asciidoc, asciidoc, yes, no)
AC_CHECK_PROG(zerosync_have_xmlto, xmlto, yes, no)
if test "x$zerosync_have_asciidoc" = "xno" -o "x$zerosync_have_xmlto" = "xno"; then
    zerosync_build_doc="no"
    # Tarballs built with 'make dist' ship with prebuilt documentation.
    if ! test -f doc/zerosync.7; then
        zerosync_install_man="no"
        AC_MSG_WARN([You are building an unreleased version of libzyre and asciidoc or xmlto are not installed.])
        AC_MSG_WARN([Documentation will not be built and manual pages will not be installed.])
    fi
fi

AC_MSG_CHECKING([whether to build documentation])
AC_MSG_RESULT([$zerosync_build_doc])
AC_MSG_CHECKING([whether to install manpages])
AC_MSG_RESULT([$zerosync_install_man])

AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AM_CONDITIONAL(INSTALL_MAN, test "x$zerosync_install_man" = "xyes")
AM_CONDITIONAL(BUILD_DOC, test "x$zerosync_build_doc" = "xyes")

AC_OUTPUT(Makefile src/Makefile doc/Makefile src/libzsync.pc)
