#include "atheme-compat.h"

static void cs_cmd_ping(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_ping = { "PING", "Verifies network connectivity by responding with pong.",
			AC_NONE, 0, cs_cmd_ping, { .path = "contrib/cs_ping" } };

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("chanserv", &cs_ping);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_ping);
}

static void cs_cmd_ping(sourceinfo_t *si, int parc, char *parv[])
{
	command_success_nodata(si, "Pong!");
	return;
}

DECLARE_MODULE_V1
(
	"contrib/cs_ping", MODULE_UNLOAD_CAPABILITY_OK, mod_init, mod_deinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
