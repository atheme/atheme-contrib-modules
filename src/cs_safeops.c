/*
 * Copyright (C) 2025 David Schultz <me@zpld.me>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Makes sure users with operator flags in a channel are
 * opped before a services shutdown.
 */

#include "atheme-compat.h"

static void
on_shutdown(void *unused)
{
	const char *log_target = service_get_log_target(chansvs.me);
	struct mychan *mc;
	mowgli_patricia_iteration_state_t state;

	slog(LG_INFO, "%s cs_safeops: Granting channel operator status to channel operators prior to shutdown", log_target);
	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (mc->chan == NULL)
			continue;

		const mowgli_node_t *n;

		MOWGLI_ITER_FOREACH(n, mc->chan->members.head)
		{
			struct chanuser *cu = n->data;
			struct user *u = cu->user;
			const int flags = chanacs_user_flags(mc, u);
			const bool noop = (mc->flags & MC_NOOP) || (u->myuser != NULL && u->myuser->flags & MU_NOOP);

			if (noop)
				continue;

			if (ircd->uses_owner)
			{
				if (flags & CA_USEOWNER && (cu->modes & ircd->owner_mode) == 0)
				{
					modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(u));
					cu->modes |= ircd->owner_mode;
				}
			}

			if (ircd->uses_protect)
			{
				if (flags & CA_USEPROTECT)
				{
					if (!(cu->modes & ircd->protect_mode || (ircd->uses_owner && cu->modes & ircd->owner_mode)))
					{
						modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(u));
						cu->modes |= ircd->protect_mode;
					}
				}
			}

			if (flags & CA_OP)
			{
				if ((cu->modes & CSTATUS_OP) == 0)
				{
					modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'o', CLIENT_NAME(u));
					cu->modes |= CSTATUS_OP;
				}
			}

			if (ircd->uses_halfops)
			{
				if (flags & CA_HALFOP)
				{
					if ((cu->modes & (CSTATUS_OP | ircd->halfops_mode)) == 0)
					{
						modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'h', CLIENT_NAME(u));
						cu->modes |= ircd->halfops_mode;
					}
				}
			}
		}
		modestack_flush_channel(mc->chan);
	}
	slog(LG_INFO, "%s cs_safeops: Channel operator statuses granted. Shutdown will proceed.", log_target);
}

static void
mod_init(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")
	/* We need hook_add_first() so ChanServ won't
	 * quit on us before we've done what we need to do
	 */
	hook_add_first_shutdown(on_shutdown);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	hook_del_shutdown(on_shutdown);
}

SIMPLE_DECLARE_MODULE_V1("contrib/cs_safeops", MODULE_UNLOAD_CAPABILITY_OK)
