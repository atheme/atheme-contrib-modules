# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
# Copyright (C) 2021 Atheme Development Group (https://atheme.github.io/)

AC_PREREQ([2.69])

AC_LANG([C])
AC_INIT([Atheme IRC Services Contrib Modules], [1.0], [https://github.com/atheme/atheme-contrib-modules/issues/], [atheme-contrib-modules], [https://atheme.github.io/])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([buildsys.mk extra.mk])
AC_CONFIG_HEADERS([src/contrib-sysconf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/atheme-compat.h])
AC_PREFIX_DEFAULT([~/atheme])
AS_IF([test "x${prefix}" = "xNONE"], [prefix="${ac_default_prefix}"])

# Programs required by the build system
AC_PROG_CC
AC_PROG_CC_C99
AS_IF([test "x${ac_cv_prog_cc_c99}" = "xno"], [
    AC_MSG_FAILURE([This program requires a C99 compiler])
])
AC_CHECK_TOOL([AR], [ar])
AC_PATH_PROG([CP], [cp])
AC_PROG_INSTALL
AC_PATH_PROG([LN], [ln])
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PATH_PROG([MV], [mv])
AC_PROG_RANLIB
AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([TAR], [tar])

# Does what its name implies. The above block can't be put into this function
# because of autotools black magic voodoo crap that no-one understands ...
ATHEME_CONTRIB_CHECK_BUILD_REQUIREMENTS

# Automake compatibility. --nenolod
AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])
AC_SUBST([VERSION], [AC_PACKAGE_VERSION])
AC_DEFINE_UNQUOTED([PACKAGE], ["${PACKAGE}"], [Name of package])
AC_DEFINE_UNQUOTED([VERSION], ["${VERSION}"], [Version number of package])

# This should be before any library tests so that insufficient header
# inclusions in those tests are caught before they become a runtime
# problem
ATHEME_CONTRIB_TEST_CC_FLAGS([-Werror=implicit])

# Some modules use platform-specific functionality and thus need checks that
# the functionality is available
ATHEME_CONTRIB_LIBTEST_RES_QUERY



AC_OUTPUT
BUILDSYS_TOUCH_DEPS
