/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ping spammer thingy
 */

#include "atheme-compat.h"

static const char *notices[] =
{
	"Scanning for proxies.",
	"Killing off bottlers.",
	"LOL ok so like we are teh SKANZ0RZING j00 becuz well like OMG deze bots r h3r3 an liek they are FL00DING!!#@! ignore plz",
	"gaben",
	"Please ignore this notice.",
	"Scanning for warez.",
	"All your pr0n are belong to us!",
	"Move over! This is the police!",
	"This notice brought to you by Burma-Shave.",
	"They're coming...",
	":)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(:",
	"lolz!",
	"<Hikaru> your a pagan",
	"* Ads needs to shower soon",
	"<Hik`Coding> Don't make me get Yakuza on you",
	"beu fails it",
	"BAN KAI~!$"
};

static const char *phrases[] =
{
	"",
	" please-ignore",
	" proxy scan",
	" ignore me",
	" <3 neostats",
};

static bool spamming = false;

static void
pingspam(user_t *u)
{
	user_t *sptr;
	mowgli_node_t *n;
	int i;
	service_t *svs;

	if((svs = service_find("global")) != NULL)
		for(i = 0;i < 6;i++)
			notice(svs->me->nick, u->nick, "%s", notices[rand() % sizeof(notices) / sizeof(char*)]);

	MOWGLI_ITER_FOREACH(n, me.me->userlist.head)
	{
		sptr = n->data;
		msg(sptr->nick, u->nick, "\001PING %ld%s\001",
				time(NULL),
				phrases[rand() % sizeof(phrases) / sizeof(char*)]
		   );
	}
}

static void
user_add_hook(hook_user_nick_t *data)
{
	user_t *u;

	u = data->u;
	if (u == NULL)
		return;
	if (is_internal_client(u))
		return;
	if (spamming)
		pingspam(u);
}

static void
os_cmd_pingspam(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	user_t *u;

	if(!target)
	{
		command_fail(si, fault_badparams, "Usage: \2PINGSPAM\2 <target>");
		return;
	}

	if(!(u = user_find_named(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on the network", target);
		return;
	}

	pingspam(u);
	command_success_nodata(si, "\2%s\2 has been pwned.", target);
	logcommand(si, CMDLOG_ADMIN, "PINGSPAM: \2%s\2", target);
}

static void
os_cmd_autopingspam(sourceinfo_t *si, int parc, char *parv[])
{
	char *mode = parv[0];

	if(!mode)
	{
		command_success_nodata(si, "Auto-pingspam is currently \2%s\2", spamming ? "ON" : "OFF");
		return;
	}

	if(strcasecmp(mode, "on") == 0 || atoi(mode))
	{
		spamming = true;
		command_success_nodata(si, "Auto-pingspam is now \2ON\2");
	}
	else
	{
		spamming = false;
		command_success_nodata(si, "Auto-pingspam is now \2OFF\2");
	}
}

static command_t os_pingspam = {
	.name           = "PINGSPAM",
	.desc           = N_("Spam a user with pings from every service, plus some bonus notices."),
	.access         = PRIV_OMODE,
	.maxparc        = 1,
	.cmd            = &os_cmd_pingspam,
	.help           = { .path = "contrib/pingspam" },
};

static command_t os_autopingspam = {
	.name           = "AUTOPINGSPAM",
	.desc           = N_("Spam connecting users with pings from every service, plus some bonus notices (setting)."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_autopingspam,
	.help           = { .path = "contrib/autopingspam" },
};

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("operserv", &os_pingspam);
	service_named_bind_command("operserv", &os_autopingspam);

	hook_add_event("user_add");
	hook_add_user_add(user_add_hook);
}

static void
mod_deinit(const module_unload_intent_t ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_pingspam);
	service_named_unbind_command("operserv", &os_autopingspam);

	hook_del_user_add(user_add_hook);
}

SIMPLE_DECLARE_MODULE_V1("contrib/os_pingspam", MODULE_UNLOAD_CAPABILITY_OK)
