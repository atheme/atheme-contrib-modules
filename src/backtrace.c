/*
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 * Rights to this code are documented in doc/LICENSE.
 *
 * Log crash reason with backtrace on fatal abnormal signals.
 */

#include "atheme-compat.h"

#if (CURRENT_ABI_REVISION < 730000)
#  include "serno.h"
#endif

#if (defined(__linux__) || defined(__Linux__)) && defined(__GLIBC__)
#  if (__GLIBC__ == 2) && defined(__GLIBC_MINOR__) && (__GLIBC_MINOR__ >= 1)
#    define HAVE_BACKTRACE_SUPPORT      1
#  else /* (__GLIBC__ == 2) && __GLIBC_MINOR__ && (__GLIBC_MINOR >= 1) */
#    if (__GLIBC__ > 2)
#      define HAVE_BACKTRACE_SUPPORT    1
#    endif /* (__GLIBC__ > 2) */
#  endif /* !((__GLIBC__ == 2) && __GLIBC_MINOR__ && (__GLIBC_MINOR__ >= 1)) */
#endif /* (__linux__ || __Linux__) && __GLIBC__ */

#ifdef HAVE_BACKTRACE_SUPPORT

#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>

#define MIN_STACK_FRAMES        3
#define MAX_STACK_FRAMES        64

#ifdef DATADIR
#define CRASHLOG_FILE           DATADIR "/crash.log"
#endif /* DATADIR */

#ifdef CRASHLOG_FILE
static FILE *crashfp            = NULL;
#else /* CRASHLOG_FILE */
static FILE *const crashfp      = stderr;
#endif /* !CRASHLOG_FILE */

static struct sigaction oldbusaction;
static struct sigaction oldfpeaction;
static struct sigaction oldillaction;
static struct sigaction oldsegvaction;

static bool
contrib_backtrace_signal_map(const int signum, const siginfo_t *const restrict info,
                             const char **const restrict fault_type,
                             const char **const restrict fault_code)
{
	if (signum == SIGBUS)
	{
		*fault_type = "SIGBUS";

		switch (info->si_code)
		{
#ifdef BUS_ADRALN
			case BUS_ADRALN:
				*fault_code = "BUS_ADRALN";
				break;
#endif /* BUS_ADRALN */
#ifdef BUS_ADDRERR
			case BUS_ADRERR:
				*fault_code = "BUS_ADRERR";
				break;
#endif /* BUS_ADRERR */
#ifdef BUS_OBJERR
			case BUS_OBJERR:
				*fault_code = "BUS_OBJERR";
				break;
#endif /* BUS_OBJERR */
#ifdef BUS_MCEERR_AR
			case BUS_MCEERR_AR:
				*fault_code = "BUS_MCEERR_AR";
				break;
#endif /* BUS_MCEERR_AR */
#ifdef BUS_MCEERR_AO
			case BUS_MCEERR_AO:
				*fault_code = "BUS_MCEERR_AO";
				break;
#endif /* BUS_MCEERR_AO */
#ifdef SI_KERNEL
			case SI_KERNEL:
				*fault_code = "<unknown>";
				break;
#endif /* SI_KERNEL */
			default:
				return false;
		}

		return true;
	}

	if (signum == SIGFPE)
	{
		*fault_type = "SIGFPE";

		switch (info->si_code)
		{
#ifdef FPE_INTDIV
			case FPE_INTDIV:
				*fault_code = "FPE_INTDIV";
				break;
#endif /* FPE_INTDIV */
#ifdef FPE_INTOVF
			case FPE_INTOVF:
				*fault_code = "FPE_INTOVF";
				break;
#endif /* FPE_INTOVF */
#ifdef FPE_FLTDIV
			case FPE_FLTDIV:
				*fault_code = "FPE_FLTDIV";
				break;
#endif /* FPE_FLTDIV */
#ifdef FPE_FLTOVF
			case FPE_FLTOVF:
				*fault_code = "FPE_FLTOVF";
				break;
#endif /* FPE_FLTOVF */
#ifdef FPE_FLTUND
			case FPE_FLTUND:
				*fault_code = "FPE_FLTUND";
				break;
#endif /* FPE_FLTUND */
#ifdef FPE_FLTRES
			case FPE_FLTRES:
				*fault_code = "FPE_FLTRES";
				break;
#endif /* FPE_FLTRES */
#ifdef FPE_FLTINV
			case FPE_FLTINV:
				*fault_code = "FPE_FLTINV";
				break;
#endif /* FPE_FLTINV */
#ifdef FPE_FLTSUB
			case FPE_FLTSUB:
				*fault_code = "FPE_FLTSUB";
				break;
#endif /* FPE_FLTSUB */
#ifdef SI_KERNEL
			case SI_KERNEL:
				*fault_code = "<unknown>";
				break;
#endif /* SI_KERNEL */
			default:
				return false;
		}

		return true;
	}

	if (signum == SIGILL)
	{
		*fault_type = "SIGILL";

		switch (info->si_code)
		{
#ifdef ILL_ILLOPC
			case ILL_ILLOPC:
				*fault_code = "ILL_ILLOPC";
				break;
#endif /* ILL_ILLOPC */
#ifdef ILL_ILLOPN
			case ILL_ILLOPN:
				*fault_code = "ILL_ILLOPN";
				break;
#endif /* ILL_ILLOPN */
#ifdef ILL_ILLADR
			case ILL_ILLADR:
				*fault_code = "ILL_ILLADR";
				break;
#endif /* ILL_ILLADR */
#ifdef ILL_ILLTRP
			case ILL_ILLTRP:
				*fault_code = "ILL_ILLTRP";
				break;
#endif /* ILL_ILLTRP */
#ifdef ILL_PRVOPC
			case ILL_PRVOPC:
				*fault_code = "ILL_PRVOPC";
				break;
#endif /* ILL_PRVOPC */
#ifdef ILL_PRVREG
			case ILL_PRVREG:
				*fault_code = "ILL_PRVREG";
				break;
#endif /* ILL_PRVREG */
#ifdef ILL_COPROC
			case ILL_COPROC:
				*fault_code = "ILL_COPROC";
				break;
#endif /* ILL_COPROC */
#ifdef ILL_BADSTK
			case ILL_BADSTK:
				*fault_code = "ILL_BADSTK";
				break;
#endif /* ILL_BADSTK */
#ifdef SI_KERNEL
			case SI_KERNEL:
				*fault_code = "<unknown>";
				break;
#endif /* SI_KERNEL */
			default:
				return false;
		}

		return true;
	}

	if (signum == SIGSEGV)
	{
		*fault_type = "SIGSEGV";

		switch (info->si_code)
		{
#ifdef SEGV_MAPERR
			case SEGV_MAPERR:
				*fault_code = "SEGV_MAPERR";
				break;
#endif /* SEGV_MAPERR */
#ifdef SEGV_ACCERR
			case SEGV_ACCERR:
				*fault_code = "SEGV_ACCERR";
				break;
#endif /* SEGV_ACCERR */
#ifdef SI_KERNEL
			case SI_KERNEL:
				*fault_code = "<unknown>";
				break;
#endif /* SI_KERNEL */
			default:
				return false;
		}

		return true;
	}

	(void) slog(LG_ERROR, "%s: unrecognised signal %d", __func__, signum);
	return false;
}

static void ATHEME_FATTR_NORETURN
contrib_backtrace_signal_handler(const int signum, siginfo_t *const restrict info,
                                 void ATHEME_VATTR_UNUSED *const restrict ucontext)
{
	const char *fault_type = "<unknown>";
	const char *fault_code = "<unknown>";

	if (! contrib_backtrace_signal_map(signum, info, &fault_type, &fault_code))
		goto end;

	char *fault_null = "<NULL>";
	char **fault_addr = (info->si_addr) ? backtrace_symbols(&info->si_addr, 1) : &fault_null;

	(void) fprintf(crashfp, "\n");
	(void) fprintf(crashfp, "===8<===8<=== [ BEGIN CRASH REPORT ] ===>8===>8===\n");
	(void) fprintf(crashfp, "\n");
	(void) fprintf(crashfp, "Program Version ...: %s (%s)\n", PACKAGE_STRING, SERNO);
	(void) fprintf(crashfp, "Config Flags ......: %s\n", get_conf_opts());
	(void) fprintf(crashfp, "Fault Type ........: %s (%d)\n", fault_type, signum);
	(void) fprintf(crashfp, "Fault Code ........: %s\n", fault_code);
	(void) fprintf(crashfp, "Fault Address .....: %s\n", (fault_addr) ? *fault_addr : fault_null);
	(void) fprintf(crashfp, "\n");

	void *frames[MAX_STACK_FRAMES];
	const int framecount = backtrace(frames, MAX_STACK_FRAMES);

	if (framecount >= MIN_STACK_FRAMES)
	{
		(void) fprintf(crashfp, "Backtrace:\n");
		(void) fprintf(crashfp, "\n");

		char **symbols = backtrace_symbols(frames, framecount);

		if (symbols)
		{
			for (int i = 0; i < framecount; i++)
				(void) fprintf(crashfp, "  #%02d %s\n", i, symbols[i]);
		}
		else
		{
			for (int i = 0; i < framecount; i++)
				(void) fprintf(crashfp, "  #%02d %p\n", i, frames[i]);

			(void) fprintf(crashfp, "\n");
			(void) fprintf(crashfp, "No symbols available.\n");
		}
	}
	else
		(void) fprintf(crashfp, "No backtrace available.\n");

	(void) fprintf(crashfp, "\n");

	if (IS_TAINTED)
	{
		(void) fprintf(crashfp, "Your installation is tainted; support is unavailable.\n");
	}
	else
	{
#ifdef PACKAGE_BUGREPORT
		(void) fprintf(crashfp, "Please file a bug report for this crash:\n");
		(void) fprintf(crashfp, "  <%s>\n", PACKAGE_BUGREPORT);
#else /* PACKAGE_BUGREPORT */
		(void) fprintf(crashfp, "Please file a bug report for this crash.\n");
#endif /* !PACKAGE_BUGREPORT */
	}

	(void) fprintf(crashfp, "\n");
	(void) fprintf(crashfp, "===8<===8<==== [ END CRASH REPORT ] ====>8===>8===\n");
	(void) fprintf(crashfp, "\n");
	(void) fflush(crashfp);

#ifdef CRASHLOG_FILE
	(void) fclose(crashfp);
	(void) fprintf(stderr, "Aborting; please see '%s'\n", CRASHLOG_FILE);
	(void) fflush(stderr);
#endif /* CRASHLOG_FILE */

end:
	// This will cause a core dump (if enabled) and will result in the program exiting
	abort();
}

static void
mod_init(module_t *const restrict m)
{
	struct rlimit coredumps;

	if (getrlimit(RLIMIT_CORE, &coredumps) != 0)
	{
		(void) slog(LG_ERROR, "%s: getrlimit(2) failed: %s", m->name, strerror(errno));
	}
	else if (coredumps.rlim_max > 0)
	{
		if (coredumps.rlim_cur < coredumps.rlim_max)
		{
			coredumps.rlim_cur = coredumps.rlim_max;

			if (setrlimit(RLIMIT_CORE, &coredumps) != 0)
				(void) slog(LG_ERROR, "%s: setrlimit(2) failed: %s", m->name, strerror(errno));
			else
				(void) slog(LG_INFO, "%s: coredumps enabled", m->name);
		}
		else
			(void) slog(LG_INFO, "%s: coredumps enabled", m->name);
	}
	else
		(void) slog(LG_INFO, "%s: coredumps cannot be enabled", m->name);

	sigset_t masked_signals;

	if (sigfillset(&masked_signals) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigfillset(3) failed: %s", m->name, strerror(errno));

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	const struct sigaction newsigaction = {
		.sa_sigaction   = &contrib_backtrace_signal_handler,
		.sa_mask        = masked_signals,
		.sa_flags       = SA_SIGINFO,
	};

	if (sigaction(SIGBUS, &newsigaction, &oldbusaction) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigaction(2) for SIGBUS failed: %s", m->name, strerror(errno));

		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (sigaction(SIGFPE, &newsigaction, &oldfpeaction) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigaction(2) for SIGFPE failed: %s", m->name, strerror(errno));

		(void) sigaction(SIGBUS, &oldbusaction, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (sigaction(SIGILL, &newsigaction, &oldillaction) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigaction(2) for SIGILL failed: %s", m->name, strerror(errno));

		(void) sigaction(SIGBUS, &oldbusaction, NULL);
		(void) sigaction(SIGFPE, &oldfpeaction, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (sigaction(SIGSEGV, &newsigaction, &oldsegvaction) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigaction(2) for SIGSEGV failed: %s", m->name, strerror(errno));

		(void) sigaction(SIGBUS, &oldbusaction, NULL);
		(void) sigaction(SIGFPE, &oldfpeaction, NULL);
		(void) sigaction(SIGILL, &oldillaction, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

#ifdef CRASHLOG_FILE
	if (! (crashfp = fopen(CRASHLOG_FILE, "w")))
	{
		(void) slog(LG_ERROR, "%s: fopen('%s') failed: %s", m->name, CRASHLOG_FILE, strerror(errno));

		(void) sigaction(SIGBUS, &oldbusaction, NULL);
		(void) sigaction(SIGFPE, &oldfpeaction, NULL);
		(void) sigaction(SIGILL, &oldillaction, NULL);
		(void) sigaction(SIGSEGV, &oldsegvaction, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) slog(LG_INFO, "%s: logging to '%s'", m->name, CRASHLOG_FILE);
#else /* CRASHLOG_FILE */
	(void) slog(LG_INFO, "%s: logging to stderr", m->name);
#endif /* !CRASHLOG_FILE */
}

static void
mod_deinit(const module_unload_intent_t ATHEME_VATTR_UNUSED intent)
{
	(void) sigaction(SIGBUS, &oldbusaction, NULL);
	(void) sigaction(SIGFPE, &oldfpeaction, NULL);
	(void) sigaction(SIGILL, &oldillaction, NULL);
	(void) sigaction(SIGSEGV, &oldsegvaction, NULL);

#ifdef CRASHLOG_FILE
	(void) fclose(crashfp);
#endif /* CRASHLOG_FILE */
}

#else /* HAVE_BACKTRACE_SUPPORT */

static void
mod_init(module_t *const restrict m)
{
	(void) slog(LG_ERROR, "%s: this module only supports Linux systems with GNU libc >= 2.1", m->name);
	(void) slog(LG_ERROR, "%s: please check your configuration and build environment", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const module_unload_intent_t ATHEME_VATTR_UNUSED intent)
{

}

#endif /* !HAVE_BACKTRACE_SUPPORT */

SIMPLE_DECLARE_MODULE_V1("contrib/backtrace", MODULE_UNLOAD_CAPABILITY_OK)
