# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE = contrib

SRCS =				\
	backtrace.c		\
	cs_access_alias.c	\
	cs_babbler.c		\
	cs_badwords.c		\
	cs_fregister.c		\
	cs_kickdots.c		\
	cs_modesync.c		\
	cs_ping.c		\
	cs_regmode.c		\
	cs_regnotice.c		\
	cs_updown.c		\
	cs_userinfo.c		\
	dnsbl.c			\
	gen_echoserver.c	\
	gen_listenerdemo.c	\
	gen_vhostonreg.c	\
	graphtastical.c		\
	gs_roulette.c		\
	ircd_announceserv.c	\
	ircd_catserv.c		\
	ircd_loveserv.c		\
	mlocktweaker.c		\
	ms_fsend.c		\
	ns_ajoin.c		\
	ns_cleannick.c		\
	ns_fenforce.c		\
	ns_forbid.c		\
	ns_fregister.c		\
	ns_generatehash.c	\
	ns_generatepass.c	\
	ns_goodmail.c		\
	ns_guestnoreg.c		\
	ns_listlogins.c		\
	ns_mxcheck_async.c	\
	ns_mxcheck.c		\
	ns_regnotice.c		\
	ns_sendpassmail.c	\
	ns_waitreg.c		\
	on_db_save.c		\
	os_akillnicklist.c	\
	os_defcon.c		\
	os_helpme.c		\
	os_joinmon.c		\
	os_kill.c		\
	os_klinechan.c		\
	os_modeall.c		\
	os_pingspam.c		\
	os_procwatch.c		\
	os_resolve.c		\
	os_savechanmodes.c	\
	os_tabletest.c		\
	os_testcmd.c		\
	os_testproc.c		\
	os_trace.c		\
	sasl_blacklist.c	\
	wumpus.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
CFLAGS		+= ${PLUGIN_CFLAGS}
LIBS		+= -L../../libathemecore -lathemecore ${CONTRIB_LIBS} ${LDFLAGS_RPATH}
