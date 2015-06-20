/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Copyright (c) 2015 Max Teufel <max@teufelsnetz.com>
 * Copyright (c) 2015 Mantas Mikulėnas <grawity@gmail.com>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows you to opt-in to authentication via Tor.
 *
 * This assumes freenode-style configuration, where specific servers
 * are dedicated to Tor access. It does not use Tor DNSBL, though it could.
 *
 * general {
 *     torservers {
 *         "hobana.freenode.net";
 *     };
 * };
 *
 */

#include "atheme-compat.h"
#include "conf.h"
#include "../../modules/nickserv/list_common.h"
#include "../../modules/nickserv/list.h"

#define MD_KEY_TOR "private:tor"

DECLARE_MODULE_V1
(
	"nickserv/set_tor", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group"
);

mowgli_patricia_t **ns_set_cmdtree;

static mowgli_list_t torservers = { NULL, NULL, 0 };

static void ns_cmd_set_tor(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_tor = { "TOR", N_("Allows logging in to your account via Tor."), AC_NONE, 1, ns_cmd_set_tor, { .path = "nickserv/set_tor" } };

static inline bool user_allows_tor(myuser_t *mu)
{
	return metadata_find(mu, MD_KEY_TOR) != NULL;
}

static inline void user_set_tor(myuser_t *mu, bool arg)
{
	if (arg)
		metadata_add(mu, MD_KEY_TOR, "allow");
	else
		metadata_delete(mu, MD_KEY_TOR);
}

static bool nick_allows_tor(const mynick_t *mn, const void *arg)
{
	return user_allows_tor(mn->owner);
}

static bool source_is_tor(sourceinfo_t *si)
{
	mowgli_node_t *n;
	server_t *s = si->su ? si->su->server : si->s;

	/*
	 * for SaslServ, only si->s and si->sourcedesc are present
	 *  - TODO: add si->sourceip if possible
	 * for NickServ, only si->su (and si->su->host, etc) are present
	 */

	if (!s || !s->name) {
		slog(LG_ERROR, "source_is_tor(): got sourceinfo %p with no server", si);
		return false;
	}

	slog(LG_DEBUG, "source_is_tor(): checking '%s'", s->name);

	MOWGLI_ITER_FOREACH(n, torservers.head)
	{
		slog(LG_DEBUG, "source_is_tor(): ... against '%s'", n->data);
		if (!strcasecmp(s->name, n->data))
		{
			slog(LG_DEBUG, "source_is_tor(): found match");
			return true;
		}
	}

	return false;
}

static void can_login_hook(hook_user_login_check_t *req)
{
	if (req->allowed && source_is_tor(req->si))
	{
		req->allowed = user_allows_tor(req->mu);
	}
}

static int tor_config_handler(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *cce;
	slog(LG_DEBUG, "tor_config_handler(%p)", ce);

	MOWGLI_ITER_FOREACH(cce, ce->entries)
	{
		slog(LG_DEBUG, "tor_config_handler(): varname '%s'", cce->varname);
		mowgli_node_add(sstrdup(cce->varname),
				mowgli_node_create(),
				&torservers);
	}
	return 0;
}

static void tor_config_purge(void *unused)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, torservers.head)
	{
		free(n->data);
		mowgli_node_delete(n, &torservers);
		mowgli_node_free(n);
	}
}

void _modinit(module_t *m)
{
	static list_param_t tor = {
		.opttype = OPT_BOOL,
		.is_match = nick_allows_tor,
	};

	hook_add_event("config_purge");
	hook_add_config_purge(tor_config_purge);

	hook_add_event("user_can_login");
	hook_add_user_can_login(can_login_hook);

	add_conf_item("TORSERVERS", &conf_gi_table, tor_config_handler);

	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");
	command_add(&ns_set_tor, *ns_set_cmdtree);

	use_nslist_main_symbols(m);
	list_register("tor", &tor);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_config_purge(tor_config_purge);

	hook_del_user_can_login(can_login_hook);

	del_conf_item("TORSERVERS", &conf_gi_table);

	command_delete(&ns_set_tor, *ns_set_cmdtree);

	list_unregister("tor");
}

/* SET TOR [ON|OFF] */
static void ns_cmd_set_tor(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOR");
		return;
	}

	if (!strcasecmp("ON", setting))
	{
		if (user_allows_tor(si->smu))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "TOR", entity(si->smu)->name);
			return;
		}
		user_set_tor(si->smu, true);
		logcommand(si, CMDLOG_SET, "SET:TOR:ON");
		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "TOR" ,entity(si->smu)->name);
		return;
	}
	else if (!strcasecmp("OFF", setting))
	{
		if (!user_allows_tor(si->smu))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "TOR", entity(si->smu)->name);
			return;
		}
		user_set_tor(si->smu, false);
		logcommand(si, CMDLOG_SET, "SET:TOR:OFF");
		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "TOR", entity(si->smu)->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "TOR");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

