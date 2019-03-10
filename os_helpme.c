/* os_helpme.c - set user mode +h
 * elly+atheme@leptoquark.net
 */

#include "atheme-compat.h"

#if (CURRENT_ABI_REVISION < 730000)
#  include "uplink.h"		/* sts() */
#endif

static void
os_cmd_helpme(sourceinfo_t *si, int parc, char *parv[])
{
	service_t *svs;

	svs = service_find("operserv");

	sts(":%s MODE %s :+h", svs->nick, si->su->nick);
	command_success_nodata(si, _("You are now a network helper."));
}

static command_t os_helpme = {
	.name           = "HELPME",
	.desc           = N_("Makes you into a network helper."),
	.access         = PRIV_HELPER,
	.maxparc        = 0,
	.cmd            = &os_cmd_helpme,
	.help           = { .path = "contrib/helpme" },
};

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("operserv", &os_helpme);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_helpme);
}

VENDOR_DECLARE_MODULE_V1("contrib/os_helpme", MODULE_UNLOAD_CAPABILITY_OK, CONTRIB_VENDOR_ELLY)
