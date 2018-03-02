/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Aggressively resyncs channels on mode change.
 */

#include "atheme-compat.h"

/* contract provided by chanserv/sync */
static void (*do_channel_sync)(mychan_t *mc, chanacs_t *ca) = NULL;

static void
on_channel_mode(hook_channel_mode_t *data)
{
	mychan_t *mc;

	return_if_fail(data != NULL);
	return_if_fail(data->c != NULL);

	mc = mychan_from(data->c);

	if (mc == NULL || mc->flags & MC_NOSYNC)
		return;

	if (do_channel_sync != NULL)
		do_channel_sync(mc, NULL);
}

static void
mod_init(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, do_channel_sync, "chanserv/sync", "do_channel_sync");

	hook_add_event("channel_mode");
	hook_add_channel_mode(on_channel_mode);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	hook_del_channel_mode(on_channel_mode);
}

VENDOR_DECLARE_MODULE_V1("contrib/cs_modesync", MODULE_UNLOAD_CAPABILITY_OK, CONTRIB_VENDOR_NENOLOD)
