/*
 * Copyright (c) 2006 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Kicks people saying "..." on channels with "kickdots" metadata set.
 *
 */

#include "atheme-compat.h"

static void
on_channel_message(hook_cmessage_data_t *data)
{
	if (data != NULL && data->msg != NULL && !strncmp(data->msg, "...", 3))
	{
		mychan_t *mc = mychan_from(data->c);

		if (mc == NULL)
			return;

		if (metadata_find(mc, "kickdots"))
		{
			kick(chansvs.me->me, data->c, data->u, data->msg);
		}
	}
}

static void
mod_init(module_t *const restrict m)
{
	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	hook_del_channel_message(on_channel_message);
}

VENDOR_DECLARE_MODULE_V1("contrib/cs_kickdots", MODULE_UNLOAD_CAPABILITY_OK, CONTRIB_VENDOR_NENOLOD)

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
