/*
 * Copyright (c) 2013 Brandon Turner
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Restrict registration to certain email addresses.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ns_goodmail", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.net>"
);

static void check_registration(hook_user_register_check_t *hdata);
static void ns_cmd_goodmail(sourceinfo_t *si, int parc, char *parv[]);

static void write_gedb(database_handle_t *db);
static void db_h_ge(database_handle_t *db, const char *type);

command_t ns_goodmail = { "GOODMAIL", N_("Restrict registration to certain email addresses."), PRIV_USER_ADMIN, 3, ns_cmd_goodmail, { .path = "contrib/ns_goodmail" } };

struct goodmail_ {
	char *mail;
	time_t mail_ts;
	char *creator;
	char *reason;
};

typedef struct goodmail_ goodmail_t;

mowgli_list_t ns_maillist;

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	hook_add_event("user_can_register");
	hook_add_user_can_register(check_registration);
	hook_add_db_write(write_gedb);

	db_register_type_handler("GE", db_h_ge);

	service_named_bind_command("nickserv", &ns_goodmail);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_user_can_register(check_registration);
	hook_del_db_write(write_gedb);

	db_unregister_type_handler("GE");

	service_named_unbind_command("nickserv", &ns_goodmail);
}

static void write_gedb(database_handle_t *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, ns_maillist.head)
	{
		goodmail_t *l = n->data;

		db_start_row(db, "GE");
		db_write_word(db, l->mail);
		db_write_time(db, l->mail_ts);
		db_write_word(db, l->creator);
		db_write_str(db, l->reason);
		db_commit_row(db);
	}
}

static void db_h_ge(database_handle_t *db, const char *type)
{
	const char *mail = db_sread_word(db);
	time_t mail_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *reason = db_sread_str(db);

	goodmail_t *l = smalloc(sizeof(goodmail_t));
	l->mail = sstrdup(mail);
	l->mail_ts = mail_ts;
	l->creator = sstrdup(creator);
	l->reason = sstrdup(reason);
	mowgli_node_add(l, mowgli_node_create(), &ns_maillist);
}
static void check_registration(hook_user_register_check_t *hdata)
{
	mowgli_node_t *n;
	goodmail_t *l;

	if (hdata->approved)
		return;

	MOWGLI_ITER_FOREACH(n, ns_maillist.head)
	{
		l = n->data;

		if (!match(l->mail, hdata->email))
		{
			return;
		}
	}

	command_fail(hdata->si, fault_noprivs, "Sorry, we do not accept registrations with email addresses from that domain.  Use another address.");
	hdata->approved = 1;
	slog(LG_INFO, "REGISTER:GOODMAIL: %s to \2%s\2 by \2%s\2",
			hdata->account, hdata->email,
			hdata->si->su != NULL ? hdata->si->su->nick : get_source_name(hdata->si));
}

static void ns_cmd_goodmail(sourceinfo_t *si, int parc, char *parv[])
{
	char *action = parv[0];
	char *email = parv[1];
	char *reason = parv[2];
	mowgli_node_t *n, *tn;
	goodmail_t *l;

	if (!action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "goodmail");
		command_fail(si, fault_needmoreparams, _("Syntax: goodmail ADD|DEL|LIST [parameters]"));
		return;
	}

	if (!strcasecmp("ADD", action))
	{
		if (!email || !reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "goodmail");
			command_fail(si, fault_needmoreparams, _("Syntax: goodmail ADD <email> <reason>"));
			return;
		}

		if (si->smu == NULL)
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}

		/* search for it */
		MOWGLI_ITER_FOREACH(n, ns_maillist.head)
		{
			l = n->data;

			if (!irccasecmp(l->mail, email))
			{
				command_success_nodata(si, _("Email \2%s\2 has already been whitelisted."), email);
				return;
			}
		}

		l = smalloc(sizeof(goodmail_t));
		l->mail = sstrdup(email);
		l->mail_ts = CURRTIME;;
		l->creator = sstrdup(get_source_name(si));
		l->reason = sstrdup(reason);

		logcommand(si, CMDLOG_ADMIN, "goodmail:ADD: \2%s\2 (Reason: \2%s\2)", email, reason);

		n = mowgli_node_create();
		mowgli_node_add(l, n, &ns_maillist);

		command_success_nodata(si, _("You have whitelisted email address \2%s\2."), email);
		return;
	}
	else if (!strcasecmp("DEL", action))
	{
		if (!email)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "goodmail");
			command_fail(si, fault_needmoreparams, _("Syntax: goodmail DEL <email>"));
			return;
		}

		MOWGLI_ITER_FOREACH_SAFE(n, tn, ns_maillist.head)
		{
			l = n->data;

			if (!irccasecmp(l->mail, email))
			{
				logcommand(si, CMDLOG_ADMIN, "goodmail:DEL: \2%s\2", l->mail);

				mowgli_node_delete(n, &ns_maillist);

				free(l->mail);
				free(l->creator);
				free(l->reason);
				free(l);

				return;
			}
		}
		command_success_nodata(si, _("Email pattern \2%s\2 not found in goodmail database."), email);
		return;
	}
	else if (!strcasecmp("LIST", action))
	{
		char buf[BUFSIZE];
		struct tm tm;

		MOWGLI_ITER_FOREACH(n, ns_maillist.head)
		{
			l = n->data;

			tm = *localtime(&l->mail_ts);
			strftime(buf, BUFSIZE, TIME_FORMAT, &tm);
			command_success_nodata(si, "Email: \2%s\2, Reason: \2%s\2 (%s - %s)",
				l->mail, l->reason, l->creator, buf);
		}
		command_success_nodata(si, "End of list.");
		logcommand(si, CMDLOG_GET, "goodmail:LIST");
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "goodmail");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
