/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Does an A/AAAA record lookup.
 */

#include "atheme-compat.h"

struct resolve_request
{
	mowgli_dns_query_t dns_query;
	sourceinfo_t *si;
};

static mowgli_heap_t *request_heap = NULL;
static mowgli_dns_t *dns_base = NULL;

static void
os_cmd_resolve_cb(mowgli_dns_reply_t *reply, int result, void *vptr)
{
	char buf[BUFSIZE];

	return_if_fail(reply != NULL);
	return_if_fail(vptr != NULL);

	const struct sockaddr *const sa = (const struct sockaddr *) &reply->addr.addr;
	struct resolve_request *const req = vptr;

	if (sa->sa_family == AF_INET)
	{
		const struct sockaddr_in *const sa4 = (const struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sa4->sin_addr, buf, sizeof buf))
			(void) command_success_nodata(req->si, _("Result: %s"), buf);
		else
			(void) command_fail(req->si, fault_internalerror, _("Error: inet_ntop(3) failed: %s"),
			                    strerror(errno));
	}
	else if (sa->sa_family == AF_INET6)
	{
		const struct sockaddr_in6 *const sa6 = (const struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sa6->sin6_addr, buf, sizeof buf))
			(void) command_success_nodata(req->si, _("Result: %s"), buf);
		else
			(void) command_fail(req->si, fault_internalerror, _("Error: inet_ntop(3) failed: %s"),
			                    strerror(errno));
	}
	else
		(void) command_fail(req->si, fault_internalerror, _("Error: Unrecognised address family %d"),
		                    (int) sa->sa_family);

	(void) atheme_object_unref(req->si);
	(void) mowgli_heap_free(request_heap, req);
}

static void
os_cmd_resolve_func(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RESOLVE");
		return;
	}

	struct resolve_request *const req4 = mowgli_heap_alloc(request_heap);
	struct resolve_request *const req6 = mowgli_heap_alloc(request_heap);

	if (!req4 || !req6)
	{
		(void) command_fail(si, fault_internalerror, _("mowgli_heap_alloc: memory allocation failure"));

		if (req4)
			(void) mowgli_heap_free(request_heap, req4);

		if (req6)
			(void) mowgli_heap_free(request_heap, req6);

		return;
	}

	req4->si = si;
	req4->dns_query.ptr = req4;
	req4->dns_query.callback = &os_cmd_resolve_cb;

	req6->si = si;
	req6->dns_query.ptr = req6;
	req6->dns_query.callback = &os_cmd_resolve_cb;

	(void) atheme_object_ref(si);
	(void) atheme_object_ref(si);

	(void) mowgli_dns_gethost_byname(dns_base, parv[0], &req4->dns_query, MOWGLI_DNS_T_A);
	(void) mowgli_dns_gethost_byname(dns_base, parv[0], &req6->dns_query, MOWGLI_DNS_T_AAAA);
}

static command_t os_cmd_resolve = {
	"RESOLVE",
	N_("Perform DNS lookup on hostname"),
	PRIV_ADMIN,
	1,
	&os_cmd_resolve_func,
	{
		.path = "contrib/os_resolve",
	},
};

static void
mod_init(module_t *const restrict m)
{
	if (! (request_heap = mowgli_heap_create(sizeof(struct resolve_request), 32, BH_NOW)))
	{
		(void) slog(LG_ERROR, _("%s: failed to create Mowgli heap object"), m->name);
		m->mflags |= MODTYPE_FAIL;
		return;
	}

	if (! (dns_base = mowgli_dns_create(base_eventloop, MOWGLI_DNS_TYPE_ASYNC)))
	{
		(void) slog(LG_ERROR, _("%s: failed to create Mowgli DNS resolver object"), m->name);
		m->mflags |= MODTYPE_FAIL;
		return;
	}

	(void) service_named_bind_command("operserv", &os_cmd_resolve);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	(void) service_named_unbind_command("operserv", &os_cmd_resolve);
	(void) mowgli_heap_destroy(request_heap);
	(void) mowgli_dns_destroy(dns_base);
}

SIMPLE_DECLARE_MODULE_V1("contrib/os_resolve", MODULE_UNLOAD_CAPABILITY_OK)
