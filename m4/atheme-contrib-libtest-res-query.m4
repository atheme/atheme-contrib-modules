# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

ATHEME_CONTRIB_LIBTEST_RES_QUERY_RESULT=""

AC_DEFUN([ATHEME_CONTRIB_LIBTEST_RES_QUERY_DRIVER], [

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            #ifdef HAVE_SYS_TYPES_H
            #  include <sys/types.h>
            #endif
            #ifdef HAVE_NETINET_IN_H
            #  include <netinet/in.h>
            #endif
            #ifdef HAVE_ARPA_NAMESER_H
            #  include <arpa/nameser.h>
            #endif
            #ifdef HAVE_NETDB_H
            #  include <netdb.h>
            #endif
            #ifdef HAVE_RESOLV_H
            #  include <resolv.h>
            #endif
            #ifndef C_ANY
            #  define C_ANY ns_c_any
            #endif
            #ifndef T_MX
            #  define T_MX ns_t_mx
            #endif
            #ifndef S_AN
            #  define S_AN ns_s_an
            #endif
        ]], [[
            ns_msg amsg;
            const int ret = res_query(NULL, C_ANY, T_MX, NULL, 0);
            (void) ns_initparse(NULL, ret, &amsg);
            const int cnt = ns_msg_count(amsg, S_AN);
        ]])
    ], [
        ATHEME_CONTRIB_LIBTEST_RES_QUERY_RESULT="yes"
    ], [
        ATHEME_CONTRIB_LIBTEST_RES_QUERY_RESULT="no"
    ])
])

AC_DEFUN([ATHEME_CONTRIB_LIBTEST_RES_QUERY_FAIL], [

    AC_MSG_WARN([the ns_mxcheck and ns_mxcheck_async modules will not be available])
])

AC_DEFUN([ATHEME_CONTRIB_LIBTEST_RES_QUERY_SUCCESS], [

    AC_DEFINE([HAVE_RES_QUERY], [1], [Define to 1 if res_query(3) appears to be usable])
])

AC_DEFUN([ATHEME_CONTRIB_LIBTEST_RES_QUERY], [

    LIBS_SAVED="${LIBS}"

    AC_SEARCH_LIBS([gethostbyname], [nsl], [

        AC_HEADER_RESOLV
        AC_MSG_CHECKING([whether compiling and linking a program using res_query(3) works])

        ATHEME_CONTRIB_LIBTEST_RES_QUERY_DRIVER
        AS_IF([test "${ATHEME_CONTRIB_LIBTEST_RES_QUERY_RESULT}" = "yes"], [
            AC_MSG_RESULT([yes])
            ATHEME_CONTRIB_LIBTEST_RES_QUERY_SUCCESS
        ], [
            LIBS="-lresolv ${LIBS}"
            ATHEME_CONTRIB_LIBTEST_RES_QUERY_DRIVER
            AS_IF([test "${ATHEME_CONTRIB_LIBTEST_RES_QUERY_RESULT}" = "yes"], [
                AC_MSG_RESULT([yes])
                ATHEME_CONTRIB_LIBTEST_RES_QUERY_SUCCESS
            ], [
                AC_MSG_RESULT([no])
                ATHEME_CONTRIB_LIBTEST_RES_QUERY_FAIL
                LIBS="${LIBS_SAVED}"
            ])
        ])
    ], [
        ATHEME_CONTRIB_LIBTEST_RES_QUERY_FAIL
    ], [])

    unset LIBS_SAVED
])
