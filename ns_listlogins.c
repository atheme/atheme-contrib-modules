/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTLOGINS function.
 */

#include "atheme-compat.h"

static void
ns_cmd_listlogins(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	mowgli_node_t *n;
	unsigned int matches = 0;

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	command_success_nodata(si, "Clients identified to account \2%s\2", entity(si->smu)->name);

	MOWGLI_ITER_FOREACH(n, si->smu->logins.head)
	{
		u = n->data;
		command_success_nodata(si, "- %s!%s@%s (real host: %s)", u->nick, u->user, u->vhost, u->host);
		matches++;
	}

	command_success_nodata(si, ngettext(N_("\2%u\2 client found"), N_("\2%u\2 clients found"), matches), matches);
	logcommand(si, CMDLOG_GET, "LISTLOGINS: (\2%u\2 matches)", matches);
}

static command_t ns_listlogins = {
	.name           = "LISTLOGINS",
	.desc           = N_("Lists details of clients authenticated as you."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ns_cmd_listlogins,
	.help           = { .path = "contrib/listlogins" },
};

static void
mod_init(module_t *const restrict m)
{
	MODULE_CONFLICT(m, "nickserv/listlogins")

	service_named_bind_command("nickserv", &ns_listlogins);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_listlogins);
}

SIMPLE_DECLARE_MODULE_V1("contrib/ns_listlogins", MODULE_UNLOAD_CAPABILITY_OK)
