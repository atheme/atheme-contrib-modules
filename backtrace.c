#include "atheme-compat.h"

#if defined( __linux__) || defined(__Linux__)
#include <execinfo.h>

static void __segv_hdl(int whocares)
{
	void *array[256];
	char **strings;
	size_t sz, i;

	sz = backtrace(array, 256);
	strings = backtrace_symbols(array, sz);

	slog(LG_INFO, "---------------- [ CRASH ] -----------------");
	slog(LG_INFO, "%zu stack frames, flags %s", sz, get_conf_opts());
	for (i = 0; i < sz; i++)
		slog(LG_INFO, "#%zu --> %p (%s)", i, array[i], strings[i]);
	slog(LG_INFO, "Report to http://jira.atheme.org/");
	slog(LG_INFO, "--------------------------------------------");
}

static void
mod_init(module_t *const restrict m)
{
	signal(SIGSEGV, __segv_hdl);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	signal(SIGSEGV, NULL);
}

DECLARE_MODULE_V1
(
	"contrib/backtrace", MODULE_UNLOAD_CAPABILITY_OK, mod_init, mod_deinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
