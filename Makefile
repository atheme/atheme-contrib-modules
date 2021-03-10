# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2021 Atheme Development Group (https://atheme.github.io/)

-include extra.mk

SUBDIRS     = src
DISTCLEAN   = buildsys.mk config.log config.status extra.mk

-include buildsys.mk

buildsys.mk:
	@echo "This repository does not compile standalone."
	@echo "Please build Atheme IRC Services with './configure --enable-contrib' instead."
	@exit 1
