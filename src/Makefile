# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (c) 2003-2004 E. Will et al.
# Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2021 Atheme Development Group <https://atheme.github.io/>

include ../../../extra.mk
include ../extra.mk

DISTCLEAN   = contrib-sysconf.h
MODULE      = contrib
SRCS        = $(wildcard *.c)

include ../buildsys.mk
include ../buildsys.module.mk

CFLAGS      += ${LIBPCRE_CFLAGS} ${LIBMOWGLI_CFLAGS}
LIBS        += ${LIBPCRE_LIBS} ${LIBMOWGLI_LIBS}

CPPFLAGS    += -I../../../include
LDFLAGS     += -L../../../libathemecore
LIBS        += -lathemecore
