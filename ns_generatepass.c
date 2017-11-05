/*
 * Copyright (c) 2005 Greg Feigenson
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a new password, either n digits long (w/ nickserv arg), or 7 digits
 *
 */

#include "atheme-compat.h"

static void ns_cmd_generatepass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_generatepass = { "GENERATEPASS", "Generates a random password.",
                        AC_NONE, 1, ns_cmd_generatepass, { .path = "contrib/generatepass" } };

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("nickserv", &ns_generatepass);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_generatepass);
}

static void ns_cmd_generatepass(sourceinfo_t *si, int parc, char *parv[])
{
	int n = 0;
	char *newpass;

	if (parc >= 1)
		n = atoi(parv[0]);

	if (n <= 0 || n > 127)
		n = 7;

	newpass = random_string(n);

	command_success_string(si, newpass, "Randomly generated password: %s", newpass);
	free(newpass);
	logcommand(si, CMDLOG_GET, "GENERATEPASS");
}

DECLARE_MODULE_V1
(
	"contrib/ns_generatepass", MODULE_UNLOAD_CAPABILITY_OK, mod_init, mod_deinit,
	PACKAGE_STRING,
	"Epiphanic Networks <http://www.epiphanic.org>"
);

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
