/*
 * Copyright (c) 2015 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NicksServ SENDPASSMAIL function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ns_sendpassmail", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

static void ns_cmd_sendpassmail(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_sendpassmail = { "SENDPASSMAIL", N_("Email registration passwords."), AC_NONE, 2, ns_cmd_sendpassmail, { .path = "contrib/ns_sendpassmail" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_sendpassmail);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_sendpassmail);
}

struct sendpassmail_state
{
	sourceinfo_t *origin;
	stringref email_canonical;
};

static bool can_sendpass(sourceinfo_t *si, myuser_t *mu)
{
	if (is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASSMAIL \2%s\2 (\2%s\2) (is SOPER)", entity(mu)->name, mu->email_canonical);
		return false;
	}

	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASSMAIL to the logged in account \2%s\2 (\2%s\2)", entity(mu)->name, mu->email_canonical);
		return false;
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASSMAIL to the frozen account %s (%s)", entity(mu)->name, mu->email_canonical);
		return false;
	}

	if (metadata_find(mu, "private:setpass:key"))
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASSMAIL to the account %s (%s) because there is already a key outstanding.", entity(mu)->name, mu->email_canonical);
		return false;
	}

	return true;
}

static int sendpassmail_foreach_cb(myentity_t *mt, void *privdata)
{
	struct sendpassmail_state *state = (struct sendpassmail_state *) privdata;

	myuser_t *mu = user(mt);
	sourceinfo_t *si = state->origin;
	bool ismarked = false;
	char *key;

	hook_user_needforce_t needforce_hdata;

	needforce_hdata.si = si;
	needforce_hdata.mu = mu;
	needforce_hdata.allowed = 1;

	if (state->email_canonical != mu->email_canonical)
	{
		return 0;
	}

	hook_call_user_needforce(&needforce_hdata);

	if (!needforce_hdata.allowed || metadata_find(mu, "private:mark:setter"))
	{
		ismarked = true;
	}

	if (!can_sendpass(si, mu))
	{
		return 0;
	}

	key = random_string(12);
	if (sendemail(si->su != NULL ? si->su : si->service->me, mu, EMAIL_SETPASS, mu->email, key))
	{
		if (ismarked)
			wallops("%s used SENDPASSMAIL for the \2MARKED\2 account %s (%s).", get_oper_name(si), entity(mu)->name, mu->email_canonical);

		logcommand(si, CMDLOG_ADMIN, "SENDPASSMAIL: \2%s\2 (\2%s\2) (change key)", entity(mu)->name, mu->email_canonical);
		metadata_add(mu, "private:setpass:key", crypt_string(key, gen_salt()));
		metadata_add(mu, "private:sendpass:sender", get_oper_name(si));
		metadata_add(mu, "private:sendpass:timestamp", number_to_string(time(NULL)));
	}
	else
		logcommand(si, CMDLOG_ADMIN, "SENDPASSMAIL failed sending email to  %s", mu->email_canonical);

	free(key);
	return 0;
}

static void ns_cmd_sendpassmail(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *email = parv[0];
	char cmdtext[NICKLEN + 20];

	struct sendpassmail_state state;

	if (!email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SENDPASSMAIL");
		command_fail(si, fault_needmoreparams, _("Syntax: SENDPASSMAIL <email>"));
		return;
	}

	state.email_canonical = canonicalize_email(email);
	state.origin = si;

	if (!validemail(email)) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid email address."), email);
		return;
	}

	myentity_foreach_t(ENT_USER, sendpassmail_foreach_cb, &state);
	strshare_unref(state.email_canonical);

	command_success_nodata(si, _("A password reset email has been sent for all accounts matching address \2%s\2, if any."), email);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
