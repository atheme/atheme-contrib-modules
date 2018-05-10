#include "atheme-compat.h"

mowgli_list_t restricted_hosts;

static int c_restricted_hosts(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *subce;
	mowgli_node_t *n, *tn;

	if(!ce->entries)
		return 0;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, restricted_hosts.head)
	{
		free(n->data);
		mowgli_node_delete(n, &restricted_hosts);
		mowgli_node_free(n);
	}

	MOWGLI_ITER_FOREACH(subce, ce->entries)
	{
		if (subce->entries != NULL)
			conf_report_warning(ce, "Invalid restricted_hosts entry");
		else
			mowgli_node_add(sstrdup(subce->varname), mowgli_node_create(), &restricted_hosts);
	}

	return 0;
}

mowgli_list_t permitted_mechanisms;

static int c_permitted_mechanisms(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *subce;
	mowgli_node_t *n, *tn;

	if(!ce->entries)
		return 0;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, permitted_mechanisms.head)
	{
		free(n->data);
		mowgli_node_delete(n, &permitted_mechanisms);
		mowgli_node_free(n);
	}

	MOWGLI_ITER_FOREACH(subce, ce->entries)
	{
		if (subce->entries != NULL)
			conf_report_warning(ce, "Invalid permitted_mechanisms entry");
		else
			mowgli_node_add(sstrdup(subce->varname), mowgli_node_create(), &permitted_mechanisms);
	}

	return 0;
}

static bool is_restricted_host(char const *host)
{
	mowgli_node_t *n;
	char *entry;

	MOWGLI_ITER_FOREACH(n, restricted_hosts.head)
	{
		entry = n->data;
		if(!match(entry, host) || !match_ips(entry, host))
			return 1;
	}
	return 0;
}

static bool is_permitted_mechanism(char const *mech)
{
	mowgli_node_t *n;
	MOWGLI_ITER_FOREACH(n, permitted_mechanisms.head)
		if(!strcmp(n->data, mech))
			return 1;
	return 0;
}

static void can_login(hook_user_login_check_t *c)
{
	struct sasl_sourceinfo *ssi;
	char const *log_target = service_get_log_target(c->si->service);

	if(c->si->service == service_find("saslserv"))
	{
		ssi = (struct sasl_sourceinfo *)c->si;
		if((ssi->sess->host && is_restricted_host(ssi->sess->host)) || (ssi->sess->ip && is_restricted_host(ssi->sess->ip)))
			if(!is_permitted_mechanism(ssi->sess->mechptr->name))
			{
				slog(CMDLOG_LOGIN, "%s %s:%s failed LOGIN to \2%s\2 (%s not allowed)", log_target, entity(c->mu)->name, ssi->sess->uid, entity(c->mu)->name, ssi->sess->mechptr->name);
				c->allowed = false;
				return;
			}
	}
	else if(c->si->su)
	{
		if(is_restricted_host(c->si->su->host) || is_restricted_host(c->si->su->chost) || is_restricted_host(c->si->su->vhost) || is_restricted_host(c->si->su->ip))
		{
			logcommand(c->si, CMDLOG_LOGIN, "failed IDENTIFY to \2%s\2 (restricted address)", entity(c->mu)->name);
			c->allowed = false;
			return;
		}
	}
}

static void can_register(hook_user_register_check_t *c)
{
	if(is_restricted_host(c->si->su->host) || is_restricted_host(c->si->su->chost) || is_restricted_host(c->si->su->vhost) || is_restricted_host(c->si->su->ip))
	{
		logcommand(c->si, CMDLOG_LOGIN, "denied REGISTER of \2%s\2 (restricted address)", c->account);
		c->approved = 1;
	}
}

static void mod_init(module_t *m)
{
	hook_add_user_can_login(can_login);
	hook_add_user_can_register(can_register);

	add_conf_item("RESTRICTED_HOSTS", &service_find("saslserv")->conf_table, c_restricted_hosts);
	add_conf_item("PERMITTED_MECHANISMS", &service_find("saslserv")->conf_table, c_permitted_mechanisms);
}

static void mod_deinit(module_unload_intent_t intent)
{
	mowgli_node_t *n, *tn;

	hook_del_user_can_login(can_login);
	hook_del_user_can_register(can_register);

	del_conf_item("RESTRICTED_HOSTS", &service_find("saslserv")->conf_table);
	MOWGLI_ITER_FOREACH_SAFE(n, tn, restricted_hosts.head)
	{
		free(n->data);
		mowgli_node_delete(n, &restricted_hosts);
		mowgli_node_free(n);
	}
	del_conf_item("PERMITTED_MECHANISMS", &service_find("saslserv")->conf_table);
	MOWGLI_ITER_FOREACH_SAFE(n, tn, permitted_mechanisms.head)
	{
		free(n->data);
		mowgli_node_delete(n, &permitted_mechanisms);
		mowgli_node_free(n);
	}
}

VENDOR_DECLARE_MODULE_V1("contrib/sasl_blacklist", MODULE_UNLOAD_CAPABILITY_OK, CONTRIB_VENDOR_FREENODE)
