# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2021 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_CONTRIB_CHECK_BUILD_REQUIREMENTS], [

    # Initialise our build system
    BUILDSYS_INIT
    BUILDSYS_SHARED_LIB
    BUILDSYS_PROG_IMPLIB
])
