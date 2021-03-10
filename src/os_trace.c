/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Looks for users and performs actions on them.
 */

#include "atheme-compat.h"

#include <limits.h>

struct trace_query_constructor
{
       void  *(*prepare)(char **);
       bool   (*exec)(user_t *, void *);
       void   (*cleanup)(void *);
};

struct trace_query_domain
{
	struct trace_query_constructor *cons;
	mowgli_node_t                   node;
};

struct trace_query_regexp_domain
{
	struct trace_query_domain       domain;
	atheme_regex_t *                regex;
	char *                          pattern;
	int                             flags;
};

struct trace_query_server_domain
{
	struct trace_query_domain       domain;
	server_t *                      server;
};

struct trace_query_glob_domain
{
	struct trace_query_domain       domain;
	char *                          pattern;
};

struct trace_query_channel_domain
{
	struct trace_query_domain       domain;
	channel_t *                     channel;
};

struct trace_query_nickage_domain
{
	struct trace_query_domain       domain;
	int                             nickage;
	int                             comparison;
};

struct trace_query_numchan_domain
{
	struct trace_query_domain       domain;
	unsigned int                    numchan;
	int                             comparison;
};

struct trace_query_identified_domain
{
	struct trace_query_domain       domain;
	bool                            identified;
};

struct trace_action
{
	sourceinfo_t *          si;
	bool                    matched;
};

struct trace_action_constructor
{
	struct trace_action * (*prepare)(sourceinfo_t *, char **);
	void                  (*exec)(user_t *, struct trace_action *);
	void                  (*cleanup)(struct trace_action *, bool);
};

struct trace_action_kill
{
	struct trace_action     base;
	char *                  reason;
};

struct trace_action_akill
{
	struct trace_action     base;
	long                    duration;
	char *                  reason;
};

struct trace_action_count
{
	struct trace_action     base;
	unsigned int            matches;
};

/*
 * Add-on interface.
 *
 * This allows third-party module writers to extend the trace API.
 * Just copy the prototypes out of trace.c, and add the trace_cmdtree
 * symbol to your module with MODULE_USE_SYMBOL().
 *
 * Then add your criteria to the tree with mowgli_patricia_add().
 */
mowgli_patricia_t *trace_cmdtree = NULL;
mowgli_patricia_t *trace_acttree = NULL;

static int
read_comparison_operator(char **string, int default_comparison)
{
	if (**string == '<')
	{
		if (*((*string)+1) == '=')
		{
			*string += 2;
			return 2;
		}
		else
		{
			*string += 1;
			return 1;
		}
	}
	else if (**string == '>')
	{
		if (*((*string)+1) == '=')
		{
			*string += 2;
			return 4;
		}
		else
		{
			*string += 1;
			return 3;
		}
	}
	else if (**string == '=')
	{
		*string += 1;
		return 0;
	}
	else
		return default_comparison;
}

static char *
reason_extract(char **args)
{
	char *start = *args;
	bool quotes = false;

	while (**args == ' ')
	{
		(*args)++;
	}
	if (**args == '"')
	{
		start = ++(*args);
		quotes = true;
	}
	else
		start = *args;

	while (**args)
	{
		if (quotes && **args == '"')
		{
			quotes = false;
			**args = 0;
			(*args)++;
			break;
		}
		else if (!quotes && **args == ' ')
		{
			**args = 0;
			(*args)++;
			break;
		}

		(*args)++;
	}

	if (!(**args))
		*args = NULL;

	if (start == *args)
		return NULL;  /* No reason found. */
	if (quotes)
		return NULL;  /* Unclosed quotes. */

	return start;
}

static void *
trace_regexp_prepare(char **args)
{
	struct trace_query_regexp_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	domain = scalloc(sizeof(struct trace_query_regexp_domain), 1);
	domain->pattern = regex_extract(*args, &(*args), &domain->flags);
	domain->regex = regex_create(domain->pattern, domain->flags);

	return domain;
}

static bool
trace_regexp_exec(user_t *u, void *q)
{
	char usermask[512];
	struct trace_query_regexp_domain *domain = (struct trace_query_regexp_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	if (domain->regex == NULL)
		return false;

	snprintf(usermask, 512, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

	return regex_match(domain->regex, usermask);
}

static void
trace_regexp_cleanup(void *q)
{
	struct trace_query_regexp_domain *domain = (struct trace_query_regexp_domain *) q;

	return_if_fail(domain != NULL);

	if (domain->regex != NULL)
		regex_destroy(domain->regex);

	sfree(domain);
}

static void *
trace_server_prepare(char **args)
{
	char *server;
	struct trace_query_server_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	server = strtok(*args, " ");

	domain = scalloc(sizeof(struct trace_query_server_domain), 1);
	domain->server = server_find(server);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool
trace_server_exec(user_t *u, void *q)
{
	struct trace_query_server_domain *domain = (struct trace_query_server_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	return (domain->server == u->server);
}

static void
trace_server_cleanup(void *q)
{
	struct trace_query_server_domain *domain = (struct trace_query_server_domain *) q;

	return_if_fail(domain != NULL);

	sfree(domain);
}

static void *
trace_glob_prepare(char **args)
{
	char *pattern;
	struct trace_query_glob_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	pattern = strtok(*args, " ");

	domain = scalloc(sizeof(struct trace_query_glob_domain), 1);
	domain->pattern = sstrdup(pattern);

	*args = strtok(NULL, "");

	return domain;
}

static bool
trace_glob_exec(user_t *u, void *q)
{
	char usermask[512];
	struct trace_query_glob_domain *domain = (struct trace_query_glob_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	if (domain->pattern == NULL)
		return false;

	snprintf(usermask, 512, "%s!%s@%s", u->nick, u->user, u->host);

	return !match(domain->pattern, usermask);
}

static void
trace_glob_cleanup(void *q)
{
	struct trace_query_glob_domain *domain = (struct trace_query_glob_domain *) q;

	return_if_fail(domain != NULL);

	sfree(domain);
}

static void *
trace_channel_prepare(char **args)
{
	char *channel;
	struct trace_query_channel_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	channel = strtok(*args, " ");

	domain = scalloc(sizeof(struct trace_query_channel_domain), 1);
	domain->channel = channel_find(channel);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool
trace_channel_exec(user_t *u, void *q)
{
	struct trace_query_channel_domain *domain = (struct trace_query_channel_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	if (domain->channel == NULL)
		return false;

	return (chanuser_find(domain->channel, u) != NULL);
}

static void
trace_channel_cleanup(void *q)
{
	struct trace_query_channel_domain *domain = (struct trace_query_channel_domain *) q;

	return_if_fail(domain != NULL);

	sfree(domain);
}

static void *
trace_nickage_prepare(char **args)
{
	char *nickage_string;
	struct trace_query_nickage_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	nickage_string = strtok(*args, " ");

	domain = scalloc(sizeof(struct trace_query_nickage_domain), 1);
	domain->comparison = read_comparison_operator(&nickage_string, 2);
	domain->nickage = atoi(nickage_string);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool
trace_nickage_exec(user_t *u, void *q)
{
	struct trace_query_nickage_domain *domain = (struct trace_query_nickage_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	int nickage = CURRTIME - u->ts;
	if (domain->comparison == 1)
		return (nickage < domain->nickage);
	else if (domain->comparison == 2)
		return (nickage <= domain->nickage);
	else if (domain->comparison == 3)
		return (nickage > domain->nickage);
	else if (domain->comparison == 4)
		return (nickage >= domain->nickage);
	else
		return (nickage == domain->nickage);
}

static void
trace_nickage_cleanup(void *q)
{
	struct trace_query_nickage_domain *domain = (struct trace_query_nickage_domain *) q;

	return_if_fail(domain != NULL);

	sfree(domain);
}

static void *
trace_numchan_prepare(char **args)
{
	char *numchan_string;
	struct trace_query_numchan_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	numchan_string = strtok(*args, " ");

	domain = scalloc(sizeof(struct trace_query_numchan_domain), 1);
	domain->comparison = read_comparison_operator(&numchan_string, 0);
	domain->numchan = atoi(numchan_string);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool
trace_numchan_exec(user_t *u, void *q)
{
	struct trace_query_numchan_domain *domain = (struct trace_query_numchan_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	unsigned int numchan = u->channels.count;
	if (domain->comparison == 1)
		return (numchan < domain->numchan);
	else if (domain->comparison == 2)
		return (numchan <= domain->numchan);
	else if (domain->comparison == 3)
		return (numchan > domain->numchan);
	else if (domain->comparison == 4)
		return (numchan >= domain->numchan);
	else
		return (numchan == domain->numchan);
}

static void
trace_numchan_cleanup(void *q)
{
	struct trace_query_numchan_domain *domain = (struct trace_query_numchan_domain *) q;

	return_if_fail(domain != NULL);

	sfree(domain);
}

static void *
trace_identified_prepare(char **args)
{
	char *yesno;
	bool identified;
	struct trace_query_identified_domain *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	yesno = strtok(*args, " ");

	if (!strcasecmp(yesno, "yes"))
		identified = true;
	else if (!strcasecmp(yesno, "no"))
		identified = false;
	else
		return NULL;

	domain = scalloc(sizeof(struct trace_query_identified_domain), 1);
	domain->identified = identified;

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool
trace_identified_exec(user_t *u, void *q)
{
	struct trace_query_identified_domain *domain = (struct trace_query_identified_domain *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	return (domain->identified == (u->myuser != NULL));
}

static void
trace_identified_cleanup(void *q)
{
	struct trace_query_identified_domain *domain = (struct trace_query_identified_domain *) q;

	return_if_fail(domain != NULL);

	sfree(domain);
}

static void
trace_action_init(struct trace_action *a, sourceinfo_t *si)
{
	return_if_fail(a != NULL);
	return_if_fail(si != NULL);

	a->si = si;
	a->matched = false;
}

static struct trace_action *
trace_print_prepare(sourceinfo_t *si, char **args)
{
	struct trace_action *a;

	return_val_if_fail(si != NULL, NULL);

	a = scalloc(sizeof(struct trace_action), 1);
	trace_action_init(a, si);

	return a;
}

static void
trace_print_exec(user_t *u, struct trace_action *a)
{
	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if(is_internal_client(u))
		return;

	a->matched = true;
	command_success_nodata(a->si, _("\2Match:\2  %s!%s@%s %s {%s}"), u->nick, u->user, u->host, u->gecos, u->server->name);
}

static void
trace_print_cleanup(struct trace_action *a, bool succeeded)
{
	return_if_fail(a != NULL);

	if (!a->matched && succeeded)
		command_success_nodata(a->si, _("No matches."));

	sfree(a);
}

static struct trace_action *
trace_kill_prepare(sourceinfo_t *si, char **args)
{
	struct trace_action_kill *a;
	char *reason;

	return_val_if_fail(si != NULL, NULL);
	return_val_if_fail(args != NULL, NULL);
	if (*args == NULL)
		return NULL;

	reason = reason_extract(args);
	if (reason == NULL)
		return NULL;

	a = scalloc(sizeof(struct trace_action_kill), 1);
	trace_action_init(&a->base, si);
	a->reason = reason;

	return (struct trace_action*) a;
}

static void
trace_kill_exec(user_t *u, struct trace_action *act)
{
	service_t *svs;
	struct trace_action_kill *a = (struct trace_action_kill *) act;

	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if (is_internal_client(u))
		return;
	if (is_ircop(u))
		return;
	if (u->myuser && is_soper(u->myuser))
		return;
	if ((svs = service_find("operserv")) == NULL)
		return;

	act->matched = true;
	kill_user(svs->me, u, "%s", a->reason);
	command_success_nodata(act->si, _("\2%s\2 has been killed."), u->nick);
}

static void
trace_kill_cleanup(struct trace_action *a, bool succeeded)
{
	return_if_fail(a != NULL);

	if (!a->matched && succeeded)
		command_success_nodata(a->si, _("No matches."));

	sfree(a);
}

static struct trace_action *
trace_akill_prepare(sourceinfo_t *si, char **args)
{
	struct trace_action_akill *a;
	char *s, *reason;
	long duration = config_options.kline_time;
	char token;

	return_val_if_fail(si != NULL, NULL);
	return_val_if_fail(args != NULL, NULL);
	if (*args == NULL)
		return NULL;

	while (**args == ' ')
		(*args)++;

	/* Extract a token, but only if there's one to remove.
	 * Otherwise, this would clip a word off the reason. */
	token = 0;
	s = *args;
	if (!strncasecmp(s, "!T", 2) || !strncasecmp(s, "!P", 2))
	{
		if (s[2] == ' ')
		{
			token = tolower(s[1]);
			s[2] = '\0';
			*args += 3;
		}
		else if (s[2] == '\0')
		{
			token = tolower(s[1]);
			*args += 2;
		}
	}

	if (token == 't')
	{
		s = strtok(*args, " ");
		*args = strtok(NULL, "");
		if (*args == NULL)
			return NULL;

		duration = (atol(s) * 60);
		while (isdigit((unsigned char)*s))
		s++;
		if (*s == 'h' || *s == 'H')
			duration *= 60;
		else if (*s == 'd' || *s == 'D')
			duration *= 1440;
		else if (*s == 'w' || *s == 'W')
			duration *= 10080;
		else if (*s == '\0')
			;
		else
			duration = 0;

		if (duration == 0)
			return NULL;
	}
	else if (token == 'p')
		duration = 0;

	reason = reason_extract(args);
	if (reason == NULL)
		return NULL;

	a = scalloc(sizeof(struct trace_action_akill), 1);
	trace_action_init(&a->base, si);
	a->duration = duration;
	a->reason = reason;


	return (struct trace_action*) a;
}

static void
trace_akill_exec(user_t *u, struct trace_action *act)
{
	const char *kuser, *khost;
	struct trace_action_akill *a = (struct trace_action_akill *) act;

	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if (is_internal_client(u))
		return;
	if (is_ircop(u))
		return;
	if (u->myuser && is_soper(u->myuser))
		return;

	kuser = "*";
	khost = u->host;

	if (!match(khost, "127.0.0.1") || !match_ips(khost, "127.0.0.1"))
		return;
	if (me.vhost != NULL && (!match(khost, me.vhost) || !match_ips(khost, me.vhost)))
		return;
	if (kline_find(kuser, khost))
		return;

	act->matched = true;
	kline_add(kuser, khost, a->reason, a->duration, get_storage_oper_name(act->si));
	command_success_nodata(act->si, _("\2%s\2 has been akilled."), u->nick);
}

static void
trace_akill_cleanup(struct trace_action *a, bool succeeded)
{
	return_if_fail(a != NULL);

	if (!a->matched && succeeded)
		command_success_nodata(a->si, _("No matches."));

	sfree(a);
}

static struct trace_action *
trace_count_prepare(sourceinfo_t *si, char **args)
{
	struct trace_action_count *a;

	return_val_if_fail(si != NULL, NULL);

	a = scalloc(sizeof(struct trace_action_count), 1);
	trace_action_init(&a->base, si);

	return (struct trace_action *) a;
}

static void
trace_count_exec(user_t *u, struct trace_action *act)
{
	struct trace_action_count *a = (struct trace_action_count *) act;

	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if (is_internal_client(u))
		return;

	act->matched = true;
	a->matches++;
}

static void
trace_count_cleanup(struct trace_action *act, bool succeeded)
{
	struct trace_action_count *a = (struct trace_action_count *) act;

	return_if_fail(a != NULL);

	if (succeeded)
		command_success_nodata(act->si, _("\2%u\2 matches"), a->matches);

	sfree(a);
}

static struct trace_query_constructor trace_regexp = {
	.prepare        = &trace_regexp_prepare,
	.exec           = &trace_regexp_exec,
	.cleanup        = &trace_regexp_cleanup,
};

static struct trace_query_constructor trace_server = {
	.prepare        = &trace_server_prepare,
	.exec           = &trace_server_exec,
	.cleanup        = &trace_server_cleanup,
};

static struct trace_query_constructor trace_glob = {
	.prepare        = &trace_glob_prepare,
	.exec           = &trace_glob_exec,
	.cleanup        = &trace_glob_cleanup,
};

static struct trace_query_constructor trace_channel = {
	.prepare        = &trace_channel_prepare,
	.exec           = &trace_channel_exec,
	.cleanup        = &trace_channel_cleanup,
};

static struct trace_query_constructor trace_nickage = {
	.prepare        = &trace_nickage_prepare,
	.exec           = &trace_nickage_exec,
	.cleanup        = &trace_nickage_cleanup,
};

static struct trace_query_constructor trace_numchan = {
	.prepare        = &trace_numchan_prepare,
	.exec           = &trace_numchan_exec,
	.cleanup        = &trace_numchan_cleanup,
};

static struct trace_query_constructor trace_identified = {
	.prepare        = &trace_identified_prepare,
	.exec           = &trace_identified_exec,
	.cleanup        = &trace_identified_cleanup,
};

static struct trace_action_constructor trace_print = {
	.prepare        = &trace_print_prepare,
	.exec           = &trace_print_exec,
	.cleanup        = &trace_print_cleanup,
};

static struct trace_action_constructor trace_kill = {
	.prepare        = &trace_kill_prepare,
	.exec           = &trace_kill_exec,
	.cleanup        = &trace_kill_cleanup,
};

static struct trace_action_constructor trace_akill = {
	.prepare        = &trace_akill_prepare,
	.exec           = &trace_akill_exec,
	.cleanup        = &trace_akill_cleanup,
};

static struct trace_action_constructor trace_count = {
	.prepare        = &trace_count_prepare,
	.exec           = &trace_count_exec,
	.cleanup        = &trace_count_cleanup,
};

static bool
os_cmd_trace_run(sourceinfo_t *si, struct trace_action_constructor *actcons, struct trace_action* act, mowgli_list_t *crit, char *args)
{
	user_t *u;
	mowgli_patricia_iteration_state_t state;
	mowgli_node_t *n;

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TRACE");
		command_fail(si, fault_needmoreparams, _("Syntax: TRACE <action> <params>"));
		return false;
	}

	while (true)
	{
		struct trace_query_constructor *cons;
		struct trace_query_domain *q;
		char *cmd = strtok(args, " ");

		if (cmd == NULL)
			break;

		cons = mowgli_patricia_retrieve(trace_cmdtree, cmd);
		if (cons == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Invalid criteria specified."));
			return false;
		}

		args = strtok(NULL, "");
		if (args == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Invalid criteria specified."));
			return false;
		}

		q = cons->prepare(&args);
		slog(LG_DEBUG, "operserv/trace: adding criteria %p(%s) to list [remain: %s]", q, cmd, args);
		if (q == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Invalid criteria specified."));
			return false;
		}
		slog(LG_DEBUG, "operserv/trace: new args position [%s]", args);

		q->cons = cons;
		mowgli_node_add(q, &q->node, crit);
	}

	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		bool doit = true;

		MOWGLI_ITER_FOREACH(n, crit->head)
		{
			struct trace_query_domain *q = (struct trace_query_domain *) n->data;

			if (!q->cons->exec(u, q))
			{
				doit = false;
				break;
			}
		}

		if (doit)
			actcons->exec(u, act);
	}

	return true;
}

static void
os_cmd_trace(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_list_t crit = { NULL, NULL, 0 };
	struct trace_action_constructor *actcons;
	struct trace_action* act;
	char *args = parv[1];
	mowgli_node_t *n, *tn;
	char *params;
	bool succeeded;

	if (!parv[0])
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "TRACE");
		command_fail(si, fault_badparams, _("Syntax: TRACE <action> <params>"));
		return;
	}

	actcons = mowgli_patricia_retrieve(trace_acttree, parv[0]);
	if (actcons == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "TRACE");
		command_fail(si, fault_badparams, _("Syntax: TRACE <action> <params>"));
		return;
	}

	act = actcons->prepare(si, &args);
	if (act == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Action compilation failed."));
		return;
	}

	params = sstrdup(args);
	succeeded = os_cmd_trace_run(si, actcons, act, &crit, args);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, crit.head)
	{
		struct trace_query_domain *q = (struct trace_query_domain *) n->data;
		q->cons->cleanup(q);
	}
	actcons->cleanup(act, succeeded);

	if (succeeded)
		logcommand(si, CMDLOG_ADMIN, "TRACE: \2%s\2 \2%s\2", parv[0], params);

	sfree(params);
}

static command_t os_trace = {
	.name           = "TRACE",
	.desc           = N_("Looks for users and performs actions on them."),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 2,
	.cmd            = &os_cmd_trace,
	.help           = { .path = "contrib/trace" },
};

static void
mod_init(module_t *const restrict m)
{
	if (! (trace_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (! (trace_acttree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		(void) mowgli_patricia_destroy(trace_cmdtree, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	mowgli_patricia_add(trace_cmdtree, "REGEXP", &trace_regexp);
	mowgli_patricia_add(trace_cmdtree, "SERVER", &trace_server);
	mowgli_patricia_add(trace_cmdtree, "GLOB", &trace_glob);
	mowgli_patricia_add(trace_cmdtree, "CHANNEL", &trace_channel);
	mowgli_patricia_add(trace_cmdtree, "NICKAGE", &trace_nickage);
	mowgli_patricia_add(trace_cmdtree, "NUMCHAN", &trace_numchan);
	mowgli_patricia_add(trace_cmdtree, "IDENTIFIED", &trace_identified);

	mowgli_patricia_add(trace_acttree, "PRINT", &trace_print);
	mowgli_patricia_add(trace_acttree, "KILL", &trace_kill);
	mowgli_patricia_add(trace_acttree, "AKILL", &trace_akill);
	mowgli_patricia_add(trace_acttree, "COUNT", &trace_count);

	service_named_bind_command("operserv", &os_trace);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_trace);

	mowgli_patricia_delete(trace_cmdtree, "REGEXP");
	mowgli_patricia_delete(trace_cmdtree, "SERVER");
	mowgli_patricia_delete(trace_cmdtree, "GLOB");
	mowgli_patricia_delete(trace_cmdtree, "CHANNEL");
	mowgli_patricia_delete(trace_cmdtree, "NICKAGE");
	mowgli_patricia_delete(trace_cmdtree, "NUMCHAN");
	mowgli_patricia_delete(trace_cmdtree, "IDENTIFIED");

	mowgli_patricia_delete(trace_acttree, "PRINT");
	mowgli_patricia_delete(trace_acttree, "KILL");
	mowgli_patricia_delete(trace_acttree, "AKILL");
	mowgli_patricia_delete(trace_acttree, "COUNT");

	mowgli_patricia_destroy(trace_cmdtree, NULL, NULL);
	mowgli_patricia_destroy(trace_acttree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("contrib/os_trace", MODULE_UNLOAD_CAPABILITY_OK)
