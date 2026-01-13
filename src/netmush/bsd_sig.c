/**
 * @file bsd_sig.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Signal handling and server control
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

/**
 * @brief Log a signal reception event to the server log
 */
static inline void log_signal(const char *signame)
{
	log_write(LOG_PROBLEMS, "SIG", "CATCH", "Caught signal %s", signame);
}

/**
 * @brief Check and handle panic state to prevent recursive signal processing
 */
static inline void check_panicking(int sig)
{
	/* If we are panicking, turn off signal catching and resignal */
	if (mushstate.panicking)
	{
		for (int i = 0; i < NSIG; i++)
		{
			signal(i, SIG_DFL);
		}

		kill(getpid(), sig);
	}

	mushstate.panicking = 1;
}

/**
 * @brief Reset all signal handlers to system default behavior
 */
static void unset_signals(void)
{
	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	for (int i = 0; i < NSIG; i++)
	{
		sigaction(i, &sa, NULL);
	}
}

/**
 * @brief Handle system signals and perform appropriate server actions
 */
static void sighandler(int sig)
{
	const char *signames[] = {"SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT", "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2"};
	int i = 0;
	pid_t child = 0;
	char *s = NULL;
	int stat = 0;

	switch (sig)
	{
	case SIGUSR1: /* Normal restart now */
		log_signal(signames[sig]);
		do_restart(GOD, GOD, 0);
		break;

	case SIGUSR2: /* Dump a flatfile soon */
		mushstate.flatfile_flag = 1;
		break;

	case SIGALRM: /* Timer */
		mushstate.alarm_triggered = 1;
		break;

	case SIGCHLD: /* Change in child status */
		while ((child = waitpid(0, &stat, WNOHANG)) > 0)
		{
			if (mushconf.fork_dump && mushstate.dumping && child == mushstate.dumper && (WIFEXITED(stat) || WIFSIGNALED(stat)))
			{
				mushstate.dumping = 0;
				mushstate.dumper = 0;
			}
		}

		break;

	case SIGHUP: /* Dump database soon */
		log_signal(signames[sig]);
		mushstate.dump_counter = 0;
		break;

	case SIGINT: /* Force a live backup */
		mushstate.backup_flag = 1;
		break;

	case SIGQUIT: /* Normal shutdown soon */
		mushstate.shutdown_flag = 1;
		break;

	case SIGTERM: /* Graceful shutdown with full database dump */
#ifdef SIGXCPU
	case SIGXCPU:
#endif
		check_panicking(sig);
		log_signal(signames[sig]);
		raw_broadcast(0, "GAME: Caught signal %s, shutting down gracefully.", signames[sig]);
		al_store();								/* Persist any in-memory attribute list before exit */
		dump_database_internal(DUMP_DB_NORMAL); /* Use normal dump for graceful shutdown */
		s = XASPRINTF("s", "Caught signal %s", signames[sig]);
		write_status_file(NOTHING, s);
		XFREE(s);
		exit(EXIT_SUCCESS);
		break;

	case SIGILL: /* Panic save + restart now, or coredump now */
	case SIGFPE:
	case SIGSEGV:
	case SIGTRAP:
#ifdef SIGXFSZ
	case SIGXFSZ:
#endif
#ifdef SIGEMT
	case SIGEMT:
#endif
#ifdef SIGBUS
	case SIGBUS:
#endif
#ifdef SIGSYS
	case SIGSYS:
#endif
		check_panicking(sig);
		log_signal(signames[sig]);
		report();

		if (mushconf.sig_action != SA_EXIT)
		{
			raw_broadcast(0, "GAME: Fatal signal %s caught, restarting with previous database.", signames[sig]);
			/* Not good, Don't sync, restart using older db. */
			al_store(); /* Persist any in-memory attribute list before crash dump */
			dump_database_internal(DUMP_DB_CRASH);
			db_sync_attributes();
			dddb_close();

			/* Try our best to dump a usable core by generating a second signal with the SIG_DFL action. */
			if (fork() > 0)
			{
				unset_signals();
				/* In the parent process (easier to follow with gdb), we're about to return from this signal handler
				 * and hope that a second signal is delivered. Meanwhile let's close all our files to avoid corrupting
				 * the child process. */
				for (i = 0; i < maxd; i++)
				{
					close(i);
				}

				return;
			}

			alarm(0);
			dump_restart_db();
			execl(mushconf.game_exec, mushconf.game_exec, mushconf.config_file, (char *)NULL);
			break;
		}
		else
		{
			unset_signals();
			log_write_raw(1, "ABORT! bsd.c, SA_EXIT requested.\n");
			write_status_file(NOTHING, "ABORT! bsd.c, SA_EXIT requested.");
			abort();
		}

	case SIGABRT: /* Coredump now */
		check_panicking(sig);
		log_signal(signames[sig]);
		report();
		unset_signals();
		log_write_raw(1, "ABORT! bsd.c, SIGABRT received.\n");
		write_status_file(NOTHING, "ABORT! bsd.c, SIGABRT received.");
		abort();
	}

	/* Signal handler is persistent with sigaction(), no need to re-register */
	mushstate.panicking = 0;
	return;
}

/**
 * @brief Install and configure signal handlers for the server
 */
void set_signals(void)
{
	sigset_t sigs;
	struct sigaction sa;
	/* We have to reset our signal mask, because of the possibility that we triggered a restart on a SIGUSR1.
	 * If we did so, then the signal became blocked, and stays blocked, since control never returns to the
	 * caller; i.e., further attempts to send a SIGUSR1 would fail. */
	sigfillset(&sigs);
	sigprocmask(SIG_UNBLOCK, &sigs, NULL);

	/* Setup sigaction structure for our handler */
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);

	/* Restart interrupted syscalls automatically */
	sa.sa_flags = SA_RESTART;

	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/* Setup for SIG_IGN */
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	/* Restore handler for remaining signals */
	sa.sa_handler = sighandler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
#ifdef SIGXCPU
	sigaction(SIGXCPU, &sa, NULL);
#endif

	/* Setup for SIG_IGN */
	sa.sa_handler = SIG_IGN;
	sigaction(SIGFPE, &sa, NULL);

	/* Restore handler for remaining signals */
	sa.sa_handler = sighandler;
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
#ifdef SIGFSZ
	sigaction(SIGXFSZ, &sa, NULL);
#endif
#ifdef SIGEMT
	sigaction(SIGEMT, &sa, NULL);
#endif
#ifdef SIGBUS
	sigaction(SIGBUS, &sa, NULL);
#endif
#ifdef SIGSYS
	sigaction(SIGSYS, &sa, NULL);
#endif
}
