#include "atheme-compat.h"

#ifdef HAVE_RES_QUERY

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
#  include <arpa/nameser.h>
#endif
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif
#ifdef HAVE_RESOLV_H
#  include <resolv.h>
#endif

#ifndef C_IN
#  define C_IN ns_c_in
#endif
#ifndef T_MX
#  define T_MX ns_t_mx
#endif
#ifndef S_AN
#  define S_AN ns_s_an
#endif

static int
count_mx(const char *host)
{
	unsigned char nsbuf[4096];
	ns_msg amsg;

	if (! host || ! *host)
		return 0;

	const int len = res_query(host, C_IN, T_MX, nsbuf, sizeof nsbuf);

	if (len < 0)
		return 0;

	(void) ns_initparse(nsbuf, len, &amsg);

	return ns_msg_count(amsg, S_AN);
}

static void
check_registration(hook_user_register_check_t *hdata)
{
	char buf[1024];
	const char *user;
	const char *domain;
	int count;

	if (hdata->approved)
		return;

	mowgli_strlcpy(buf, hdata->email, sizeof buf);
	user = strtok(buf, "@");
	domain = strtok(NULL, "@");
	count = count_mx(domain);

	if (count > 0)
	{
		/* there are MX records for this domain */
		slog(LG_INFO, "REGISTER: mxcheck: %d MX records for %s", count, domain);
	}
	else
	{
		/* no MX records or error */
		struct hostent *host;

		/* attempt to resolve host (fallback to A) */
		if ((host = gethostbyname(domain)) == NULL)
		{
			slog(LG_INFO, "REGISTER: mxcheck: no A/MX records for %s - REGISTER failed", domain);
			command_fail(hdata->si, fault_noprivs, "Sorry, \2%s\2 does not exist, I can't send mail "
			                                       "there. Please check and try again.", domain);
			hdata->approved = 1;
			return;
		}
	}
}

static void
mod_init(module_t *const restrict m)
{
	hook_add_event("user_can_register");
	hook_add_user_can_register(check_registration);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	hook_del_user_can_register(check_registration);
}

#else /* HAVE_RES_QUERY */

static void
mod_init(module_t *const restrict m)
{
	(void) slog(LG_ERROR, "%s: this module requires a functioning res_query(3); refusing to load", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const module_unload_intent_t intent)
{

}

#endif /* !HAVE_RES_QUERY */

VENDOR_DECLARE_MODULE_V1("contrib/ns_mxcheck", MODULE_UNLOAD_CAPABILITY_OK, CONTRIB_VENDOR_JAMIE_PENMAN)
