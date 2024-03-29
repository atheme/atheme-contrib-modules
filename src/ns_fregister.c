/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ FREGISTER function.
 *
 * Remember to give the user:fregister priv to any soper you want
 * to be able to use this command.
 */

#include "atheme-compat.h"

static void
ns_cmd_fregister(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mynick_t *mn = NULL;
	char *account;
	char *pass;
	char *email;
	int i, uflags = 0;
	hook_user_req_t req;

	account = parv[0], pass = parv[1], email = parv[2];

	if (!account || !pass || !email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREGISTER");
		command_fail(si, fault_needmoreparams, _("Syntax: FREGISTER <account> <password> <email> "
		                                         "[CRYPTPASS] [HIDEMAIL] [NOOP] [NEVEROP]"));
		return;
	}

	for (i = 3; i < parc; i++)
	{
		if (!strcasecmp(parv[i], "CRYPTPASS"))
			uflags |= MU_CRYPTPASS;
		else if (!strcasecmp(parv[i], "HIDEMAIL"))
			uflags |= MU_HIDEMAIL;
		else if (!strcasecmp(parv[i], "NOOP"))
			uflags |= MU_NOOP;
		else if (!strcasecmp(parv[i], "NEVEROP"))
			uflags |= MU_NEVEROP;
	}

	if (strlen(pass) > COMPAT_PASSLEN)
	{
		if (uflags & MU_CRYPTPASS)
			command_fail(si, fault_badparams, "The provided password hash is too long");
		else
			command_fail(si, fault_badparams, "The provided password is too long");
		return;
	}

	if (!nicksvs.no_nick_ownership && IsDigit(*account))
	{
		command_fail(si, fault_badparams, "For security reasons, you can't register your UID.");
		return;
	}

	if (! is_valid_nick(account))
	{
		command_fail(si, fault_badparams, "The account name \2%s\2 is invalid.", account);
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	if (nicksvs.no_nick_ownership ? myuser_find(account) != NULL : mynick_find(account) != NULL)
	{
		command_fail(si, fault_alreadyexists, "\2%s\2 is already registered.", account);
		return;
	}

	mu = myuser_add(account, pass, email, uflags | config_options.defuflags | MU_NOBURSTLOGIN);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_add(mu, entity(mu)->name);
		mn->registered = CURRTIME;
		mn->lastseen = CURRTIME;
	}

	logcommand(si, CMDLOG_REGISTER, "FREGISTER: \2%s\2 to \2%s\2", account, email);
	if (is_soper(mu))
	{
		wallops("%s used FREGISTER on account \2%s\2 with services operator privileges.", get_oper_name(si), entity(mu)->name);
		slog(LG_INFO, "SOPER: \2%s\2", entity(mu)->name);
	}

	command_success_nodata(si, "\2%s\2 is now registered to \2%s\2.", entity(mu)->name, mu->email);
	hook_call_user_register(mu);
	req.si = si;
	req.mu = mu;
	req.mn = mn;
	hook_call_user_verify_register(&req);
}

static command_t ns_fregister = {
	.name           = "FREGISTER",
	.desc           = N_("Registers a nickname on behalf of another user."),
	.access         = PRIV_USER_FREGISTER,
	.maxparc        = 20,
	.cmd            = &ns_cmd_fregister,
	.help           = { .path = "contrib/fregister" },
};

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("nickserv", &ns_fregister);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_fregister);
}

SIMPLE_DECLARE_MODULE_V1("contrib/ns_fregister", MODULE_UNLOAD_CAPABILITY_OK)
