AC_PREREQ(2.61)
AC_INIT([quic-teec], 1.0.0)
AM_INIT_AUTOMAKE([-Wall gnu foreign])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CXX
AM_PROG_AR

PKG_PROG_PKG_CONFIG

AC_ARG_WITH([uapi-headers],
    AC_HELP_STRING([--with-uapi-headers=DIR],
                   [Specify the location of the installed kernel UAPI headers for build]),
    [uapidir=$withval],
    with_uapi=no)

if (test "x$with_uapi" != "xno"); then
    UAPI_INC_PATH="-I${uapidir}"
    AC_SUBST(UAPI_INC_PATH)
fi

AC_ARG_WITH([qcbor-package],
    AC_HELP_STRING([--with-qcbor-package=DIR],
                   [Specify the location of the installed QCBOR package for build]),
    [qcborpkgdir=$withval],
    with_qcbor_package=no)

if (test "x$with_qcbor_package" != "xno"); then
    QCBOR_INC_PATH="-I${qcborpkgdir}/include"
    QCBOR_LIB_PATH="-L${qcborpkgdir}/lib"
    AC_SUBST(QCBOR_INC_PATH)
    AC_SUBST(QCBOR_LIB_PATH)
else
    AC_MSG_ERROR([No location provided for installed QCBOR package via --with-qcbor-package])
fi

AC_CONFIG_FILES([Makefile \
                 libqcomtee/Makefile \
                 test/qcomtee_clients/Makefile])

AC_OUTPUT
