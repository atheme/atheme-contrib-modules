/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Russian Roulette game. Will actually /KILL the user that gets "shot".
 *
 */

#include "atheme-compat.h"

static void command_roulette(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_roulette = { "ROULETTE", N_("A game of Russian Roulette."), AC_NONE, 2, command_roulette, { .path = "contrib/roulette" } };

void mod_init(module_t * m)
{
	service_named_bind_command("gameserv", &cmd_roulette);

	service_named_bind_command("chanserv", &cmd_roulette);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("gameserv", &cmd_roulette);

	service_named_unbind_command("chanserv", &cmd_roulette);
}

/*
 * Handle reporting for both fantasy commands and normal commands in GameServ
 * quickly and easily. Of course, sourceinfo has a vtable that can be manipulated,
 * but this is quicker and easier...                                  -- nenolod
 */
static void gs_command_report(sourceinfo_t *si, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->c != NULL)
		msg(chansvs.nick, si->c->name, "%s", buf);
	else
		command_success_nodata(si, "%s", buf);

	if (!strcasecmp(buf, "*BANG*"))
		kill_user(si->service->me, si->su, "Lost at Russian Roulette.");
}

static void command_roulette(sourceinfo_t *si, int parc, char *parv[])
{
	static const char *roulette_responses[2] = {
		N_("*CLICK*"),
		N_("*BANG*")
	};

	gs_command_report(si, "%s", roulette_responses[rand() % 6 == 0]);
}

SIMPLE_DECLARE_MODULE_V1("contrib/gs_roulette", MODULE_UNLOAD_CAPABILITY_OK)
