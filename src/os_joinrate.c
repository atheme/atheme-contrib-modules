/*
 * Copyright (c) 2021 notsurewhattoputhere
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Watches all channels for abnormally high join rates
 */

#include "atheme-compat.h"

struct joinrate_ {
	char *chname;
	int rate;
	int time;

	time_t last_join_time;
	int tokens;
	time_t last_warn_time;
};
typedef struct joinrate_ joinrate_t;
joinrate_t *joinrate_default;

mowgli_patricia_t *os_joinrates = NULL;
mowgli_heap_t *os_joinrate_heap = NULL;

static void
db_write_jr(database_handle_t *db)
{
	joinrate_t *joinrate;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(joinrate, &state, os_joinrates)
	{
		// don't write to DB if using default rate/time
		if (joinrate->rate != -1)
		{
			db_start_row(db, "JR");
			db_write_word(db, joinrate->chname);
			db_write_int(db, joinrate->rate);
			db_write_int(db, joinrate->time);
			db_commit_row(db);
		}
	}
}

joinrate_t *
os_joinrate_get(const char *chname)
{
	return mowgli_patricia_retrieve(os_joinrates, chname);
}
joinrate_t *
os_joinrate_create(char *chname)
{
	joinrate_t *joinrate = mowgli_heap_alloc(os_joinrate_heap);

	joinrate->chname = chname;
	joinrate->rate = -1;
	joinrate->time = -1;
	joinrate->tokens = -1;
	joinrate->last_join_time = CURRTIME;
	mowgli_patricia_add(os_joinrates, joinrate->chname, joinrate);
	return joinrate;
}
static void
os_joinrate_remove(joinrate_t *joinrate)
{
	mowgli_patricia_delete(os_joinrates, joinrate->chname);
	sfree(joinrate->chname);
	mowgli_heap_free(os_joinrate_heap, joinrate);
}


static void
db_handle_jr(database_handle_t *db, const char *type)
{
	const char *chname = db_sread_word(db);
	const int rate = db_sread_int(db);
	const int time = db_sread_int(db);

	// checking if it exists first is entirely for the benefit of the
	// special "default" channel, which we make in modinit and assign to
	// global joinrate_default
	joinrate_t *joinrate = os_joinrate_get(chname);
	if (joinrate == NULL)
		joinrate = os_joinrate_create(sstrdup(chname));

	joinrate->rate = rate;
	joinrate->time = time;
	joinrate->tokens = -1;
}

static void
watch_user_joins(hook_channel_joinpart_t *hdata)
{
	chanuser_t *cu = hdata->cu;

	// don't count JOINs in a netjoin burst
	if (me.bursting)
		return;

	joinrate_t *joinrate = os_joinrate_get(cu->chan->name);
	if (joinrate == NULL)
		joinrate = os_joinrate_create(cu->chan->name);

	// pick up default rate/time if this channel is set to defaults
	unsigned int ch_rate = (joinrate->rate == -1 ? joinrate_default->rate : joinrate->rate)-1;
	unsigned int ch_time = joinrate->time == -1 ? joinrate_default->time : joinrate->time;
	if (joinrate->tokens == -1)
		joinrate->tokens = ch_rate;

	// do some token bucket magic
	unsigned int elapsed = CURRTIME - joinrate->last_join_time;
	joinrate->tokens += (elapsed * ch_rate) / ch_time;

	// cap tokens at `rate`
	if (joinrate->tokens > ch_rate)
		joinrate->tokens = ch_rate;

	// is there a token left for us to consume?
	if (joinrate->tokens >= 1)
	{
		// yes, consume a token
		joinrate->tokens--;
	}
	else if (CURRTIME - joinrate->last_warn_time >= 30)
	{
		// no, spit out a warning
		joinrate->last_warn_time = CURRTIME;
		slog(LG_INFO, "JOINRATE: \2%s\2 exceeds warning threshold (%d joins in %ds)",
		cu->chan->name, joinrate->rate, joinrate->time);
	}

	joinrate->last_join_time = CURRTIME;
}

static void
os_cmd_joinrate(sourceinfo_t *si, int parc, char *parv[])
{
	char *chname = parv[0];
	char *action = parv[1];
	char *arg_rate = parv[2];
	char *arg_time = parv[3];
	unsigned int rate, time;
	joinrate_t *joinrate;
        mowgli_patricia_iteration_state_t state;

	if (!chname || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINRATE");
		command_fail(si, fault_needmoreparams, _("Syntax: JOINRATE <#channel|default> GET|SET|UNSET [parameters]"));
		return;
	}

	joinrate = os_joinrate_get(chname);

	if (!strcasecmp("GET", action))
	{
		if (joinrate == NULL || joinrate->rate == -1)
		{
			command_success_nodata(si, _("Join rate warning threshold for \2%s\2 is %d joins in %ds (default)"),
				chname, joinrate_default->rate, joinrate_default->time);
		}
		else
		{
			command_success_nodata(si, _("Join rate warning threshold for \2%s\2 is %d joins in %ds"),
				chname, joinrate->rate, joinrate->time);
		}
	}
	else if (!strcasecmp("UNSET", action))
	{

		if (joinrate == NULL || joinrate->rate == -1)
			command_fail(si, fault_nochange, _("\2%s\2 is already set to default join rate warning threshold"), chname);
		else if (!strcasecmp("default", chname))
		{
			joinrate_default->rate = 5;
			joinrate_default->time = 5;

		        MOWGLI_PATRICIA_FOREACH(joinrate, &state, os_joinrates)
		        {
				// reset available token-bucket tokens for
				// channels using default rate/time
				if (joinrate->rate == -1)
					joinrate->tokens = -1;
			}
			command_success_nodata(si, _("Reset default join rate warning threshold (%d joins in %ds)"),
				joinrate_default->rate, joinrate_default->time);
		}
		else
		{
			os_joinrate_remove(joinrate);
			command_success_nodata(si, _("Reset \2%s\2 to default join rate warning threshold (%d joins in %ds)"),
				chname, joinrate_default->rate, joinrate_default->time);
		}
	}
	else if (!strcasecmp("SET", action))
	{
		if (!arg_rate || !arg_time)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINRATE");
			command_fail(si, fault_needmoreparams, _("Syntax: JOINRATE <#channel|default> SET <rate> <time>"));
			return;
		}
		else if (!string_to_uint(arg_rate, &rate))
		{
			command_fail(si, fault_badparams, _("Rate must be an integer"));
			return;
		}
		else if (!string_to_uint(arg_time, &time))
		{
			command_fail(si, fault_badparams, _("Time must be an integer"));
			return;
		}

		if (joinrate == NULL)
			joinrate = os_joinrate_create(sstrdup(chname));

		if (!strcasecmp("default", chname))
		{
			joinrate_default->rate = rate;
			joinrate_default->time = time;

		        MOWGLI_PATRICIA_FOREACH(joinrate, &state, os_joinrates)
		        {
				// reset available token-bucket tokens for
				// channels using default rate/time
				if (joinrate->rate == -1)
					joinrate->tokens = -1;
			}
			command_success_nodata(si, _("Set default join rate warning threshold to %d joins in %ds"),
				joinrate_default->rate, joinrate_default->time);
		}
		else
		{
			joinrate->rate = rate;
			joinrate->time = time;
			// joinrate params changed, reset bucket
			joinrate->tokens = -1;

			command_success_nodata(si, _("Set \2%s\2 join rate warning threshold to %d joins in %ds"),
				chname, joinrate->rate, joinrate->time);
		}
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "JOINRATE");
	}
}

static command_t os_joinrate = {
	.name           = "JOINRATE",
	.desc           = N_("Configure join rate thresholds for channels"),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 4,
	.cmd            = &os_cmd_joinrate,
	.help           = { .path = "contrib/os_joinrate" },
};

static void
mod_init(module_t *const restrict m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	hook_add_event("channel_join");
	hook_add_channel_join(watch_user_joins);

	hook_add_db_write(db_write_jr);
	db_register_type_handler("JR", db_handle_jr);

	service_named_bind_command("operserv", &os_joinrate);

	os_joinrate_heap = mowgli_heap_create(sizeof(joinrate_t), 32, BH_LAZY);
	os_joinrates = mowgli_patricia_create(irccasecanon);

	joinrate_default = os_joinrate_create("default");
	joinrate_default->rate = 5;
	joinrate_default->time = 5;
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	mowgli_patricia_iteration_state_t state;
	joinrate_t *joinrate;

	MOWGLI_PATRICIA_FOREACH(joinrate, &state, os_joinrates)
		os_joinrate_remove(joinrate);
	mowgli_heap_destroy(os_joinrate_heap);
	mowgli_patricia_destroy(os_joinrates, NULL, NULL);

	hook_del_channel_join(watch_user_joins);
	hook_del_db_write(db_write_jr);

	db_unregister_type_handler("JR");

	service_named_unbind_command("operserv", &os_joinrate);
}

SIMPLE_DECLARE_MODULE_V1("contrib/os_joinrate", MODULE_UNLOAD_CAPABILITY_OK)
