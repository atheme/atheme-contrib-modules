#include "atheme-compat.h"

static void
cs_cmd_ping(sourceinfo_t *si, int parc, char *parv[])
{
	command_success_nodata(si, "Pong!");
}

static command_t cs_ping = {
	.name           = "PING",
	.desc           = N_("Verifies network connectivity by responding with pong."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &cs_cmd_ping,
	.help           = { .path = "contrib/cs_ping" },
};

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

SIMPLE_DECLARE_MODULE_V1("contrib/cs_ping", MODULE_UNLOAD_CAPABILITY_OK)
