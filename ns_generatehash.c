/*
 * Copyright (c) 2010 Atheme development group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a hash for use as a operserv "password".
 *
 */

#include "atheme-compat.h"

static void ns_cmd_generatehash(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_generatehash = { "GENERATEHASH", "Generates a hash for SOPER.",
                        AC_NONE, 1, ns_cmd_generatehash, { .path = "contrib/generatehash" } };

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("nickserv", &ns_generatehash);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_generatehash);
}

static void ns_cmd_generatehash(sourceinfo_t *si, int parc, char *parv[])
{
	char *pass = parv[0];

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GENERATEHASH");
		command_fail(si, fault_needmoreparams, _("Syntax: GENERATEHASH <password>"));
		return;
	}

	const char *const hash = crypt_password(pass);

	if (hash)
		command_success_string(si, hash, "Hash is: %s", hash);
	else
		command_fail(si, fault_internalerror, _("Hash generation failure -- is a crypto module loaded?"));

	logcommand(si, CMDLOG_GET, "GENERATEHASH");
}

SIMPLE_DECLARE_MODULE_V1("contrib/ns_generatehash", MODULE_UNLOAD_CAPABILITY_OK)
