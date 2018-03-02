/* os_helpme.c - set user mode +h
 * elly+atheme@leptoquark.net
 */

#include "atheme-compat.h"
#include "uplink.h"		/* sts() */

static void os_cmd_helpme(sourceinfo_t *si, int parc, char *parv[]);

command_t os_helpme = { "HELPME", N_("Makes you into a network helper."),
                        PRIV_HELPER, 0, os_cmd_helpme, { .path = "contrib/helpme" } };

static void
os_cmd_helpme(sourceinfo_t *si, int parc, char *parv[])
{
	service_t *svs;

	svs = service_find("operserv");

	sts(":%s MODE %s :+h", svs->nick, si->su->nick);
	command_success_nodata(si, _("You are now a network helper."));
}

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
