/*
 * Copyright (c) 2007 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * mlocktweaker.c: A module which tweaks mlock on registration.
 *
 */

#include "atheme-compat.h"

/* Changed to allow for dynamic configuration, to configure this, set 
 * chanserv::mlocktweak to the mode string you want set on channels as 
 * they are registered. Keep in mind that +nt will be applied as well, 
 * so if you do not want +n or +t, negate them in the configuration.
 * - Quora
 */
char * mlocktweak;

DECLARE_MODULE_V1
(
	"contrib/mlocktweaker", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Pitcock <nenolod@atheme.org>"
);

static void handle_channel_register(hook_channel_req_t *hdata);

void _modinit(module_t *m)
{
	add_dupstr_conf_item("MLOCKTWEAK", &chansvs.me->conf_table, 0, &mlocktweak, "-t+c");
	hook_add_event("channel_register");
	hook_add_first_channel_register(handle_channel_register);
}

void _moddeinit(module_unload_intent_t intent)
{
	del_conf_item("MLOCKTWEAK", &chansvs.me->conf_table);
	hook_del_channel_register(handle_channel_register);
}

static void handle_channel_register(hook_channel_req_t *hdata)
{
	mychan_t *mc = hdata->mc;
	unsigned int *target;
	char *it = mlocktweak;

	if (mc == NULL)
		return;

	target = &mc->mlock_on;

	while (*it != '\0')
	{
		switch (*it)
		{
			case '+':
				target = &mc->mlock_on;
				break;
			case '-':
				target = &mc->mlock_off;
				break;
			default:
				*target |= mode_to_flag(*it);
				break;
		}
		++it;
	}

	mc->mlock_off &= ~mc->mlock_on;
}
