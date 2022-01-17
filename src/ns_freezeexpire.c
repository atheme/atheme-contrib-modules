/*
 * Copyright (c) 2021 Jess Porter
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Auto-drop frozen accounts after a given duration
 */

#include "atheme-compat.h"

static unsigned int freeze_expire;

static void
user_expiry_hook(struct hook_expiry_req *req)
{
	struct myuser *mu = req->data.mu;
	struct metadata *md;
	unsigned int freeze_ts;

	if (
		// don't undo an already-requested expire
		!req->do_expire
		// we have a configured freeze expire duration
		&& freeze_expire
		// this is a frozen account
		&& (md = metadata_find(mu, "private:freeze:timestamp"))
		&& string_to_uint(md->value, &freeze_ts)
	)
		req->do_expire = (CURRTIME - freeze_ts) >= freeze_expire;
}

static void
mod_init(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")
	add_duration_conf_item("FREEZE_EXPIRE", &nicksvs.me->conf_table, 0, &freeze_expire, "d", 0);
	hook_add_user_check_expire(user_expiry_hook);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	hook_del_user_check_expire(user_expiry_hook);
	del_conf_item("FREEZE_EXPIRE", &nicksvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("contrib/ns_freezeexpire", MODULE_UNLOAD_CAPABILITY_OK)
