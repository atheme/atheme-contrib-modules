/*
 * Copyright (c) 2011 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Set/unset DALnet channel mode +r on registration/recreation/deregistration.
 */

#include "atheme-compat.h"

static void register_hook(hook_channel_req_t *hdata)
{
	mychan_t *mc = hdata->mc;

	if (mc == NULL || mc->chan == NULL)
		return;

	modestack_mode_simple(chansvs.nick, mc->chan, MTYPE_ADD, CMODE_CHANREG);
}

static void add_hook(channel_t *c)
{
	mychan_t *mc;

	if (!(mc = mychan_find(c->name)))
		return;

	modestack_mode_simple(chansvs.nick, mc->chan, MTYPE_ADD, CMODE_CHANREG);
}

static void drop_hook(mychan_t *mc)
{
	if (mc == NULL || mc->chan == NULL)
		return;

	modestack_mode_simple(chansvs.nick, mc->chan, MTYPE_DEL, CMODE_CHANREG);
}

void
mod_init(module_t *m)
{
	hook_add_event("channel_register");
	hook_add_channel_register(register_hook);

	hook_add_event("channel_add");
	hook_add_channel_add(add_hook);

	hook_add_event("channel_drop");
	hook_add_channel_drop(drop_hook);
}

void
mod_deinit(module_unload_intent_t intent)
{
	hook_del_channel_register(register_hook);
	hook_del_channel_add(add_hook);
	hook_del_channel_drop(drop_hook);
}

DECLARE_MODULE_V1
(
	"contrib/cs_regmode", MODULE_UNLOAD_CAPABILITY_OK, mod_init, mod_deinit,
        PACKAGE_STRING,
        VENDOR_STRING
);
