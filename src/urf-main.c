/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@suse.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>

#include <glib-unix.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include "urf-config.h"
#include "urf-daemon.h"

#define URFKILL_SERVICE_NAME "org.freedesktop.URfkill"
#define URFKILL_CONFIG_FILE URFKILL_CONFIG_DIR"urfkill.conf"

#define URFKILL_DEFAULT_LOG_LEVEL \
	(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO)

static GMainLoop *loop = NULL;

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
	g_warning ("Lost the name %s on the system bus\n", name);
	g_main_loop_quit (loop);
}

/**
 * urf_main_sigint_cb:
 **/
static gboolean
urf_main_signal_cb (gpointer user_data)
{
	g_message ("Shutting down...");
	g_main_loop_quit (loop);
	return FALSE;
}

/**
 * urf_main_timed_exit_cb:
 *
 * Exits the main loop, which is helpful for valgrinding.
 **/
static gboolean
urf_main_timed_exit_cb (GMainLoop *loop)
{
	g_main_loop_quit (loop);
	return FALSE;
}

static void
urf_log_handler (const gchar *log_domain,
                GLogLevelFlags level,
                const gchar *message,
                gpointer ignored)
{
	const char *level_string;
	int syslog_priority;

	switch (level) {
	case G_LOG_LEVEL_ERROR:
		syslog_priority = LOG_CRIT;
		level_string = "<error> ";
		break;
	case G_LOG_LEVEL_CRITICAL:
		syslog_priority = LOG_ERR;
		level_string = "<critical> ";
		break;
	case G_LOG_LEVEL_WARNING:
		syslog_priority = LOG_WARNING;
		level_string = "<warning> ";
		break;
	case G_LOG_LEVEL_MESSAGE:
		syslog_priority = LOG_NOTICE;
		level_string = "";
		break;
	case G_LOG_LEVEL_DEBUG:
		syslog_priority = LOG_DEBUG;
		level_string = "<debug> ";
		break;
	case G_LOG_LEVEL_INFO:
	default:
		syslog_priority = LOG_INFO;
		level_string = "";
		break;
	}

	syslog (syslog_priority, "%s%s", level_string, message);
}

/**
 * main:
 **/
gint
main (gint argc, gchar **argv)
{
	UrfConfig *config = NULL;
	UrfDaemon *daemon = NULL;
	GOptionContext *context;
	gboolean ret;
	gint retval = 1;
	gboolean timed_exit = FALSE;
	gboolean immediate_exit = FALSE;
	gboolean fork_daemon = FALSE;
	gboolean debug = FALSE;
	guint owner_id;
	guint timer_id = 0;
	guint log_level = URFKILL_DEFAULT_LOG_LEVEL;
	struct passwd *user;
	const char *username = NULL;
	const char *conf_file = NULL;
	pid_t pid;

	const GOptionEntry options[] = {
		{ "timed-exit", '\0', 0, G_OPTION_ARG_NONE, &timed_exit,
		  /* TRANSLATORS: exit after we've started up, used for user profiling */
		  _("Exit after a small delay"), NULL },
		{ "immediate-exit", '\0', 0, G_OPTION_ARG_NONE, &immediate_exit,
		  /* TRANSLATORS: exit straight away, used for automatic profiling */
		  _("Exit after the engine has loaded"), NULL },
		{ "fork", 'f', 0, G_OPTION_ARG_NONE, &fork_daemon,
		  /* TRANSLATORS: fork to background */
		  _("Fork on startup"), NULL },
		{ "user", 'u', 0, G_OPTION_ARG_STRING, &username,
		  /* TRANSLATORS: change to another user and drop the privilege */
		  _("Use a specific user instead of root"), NULL },
		{ "config", 'c', 0, G_OPTION_ARG_STRING, &conf_file,
		  /* TRANSLATORS: use another config file instead of the default one */
		  _("Use a specific config file"), NULL },
		{ "debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
		  /* TRANSLATORS: enable debug logging */
		  _("Enable debug logging"), NULL },
		{ NULL }
	};

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init ();
#endif

	context = g_option_context_new ("urfkill daemon");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	openlog (G_LOG_DOMAIN, LOG_PID | LOG_CONS, LOG_DAEMON);

	if (debug)
		log_level |= G_LOG_LEVEL_DEBUG;

	g_log_set_handler (G_LOG_DOMAIN,
	                   log_level | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
	                   (GLogFunc) urf_log_handler,
	                   NULL);

	if (conf_file == NULL)
		conf_file = URFKILL_CONFIG_FILE;

	config = urf_config_new ();
	urf_config_load_from_file (config, conf_file);

	loop = g_main_loop_new (NULL, FALSE);

	/* acquire name */
	owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
	                           URFKILL_SERVICE_NAME,
	                           G_BUS_NAME_OWNER_FLAGS_NONE,
	                           NULL,
	                           NULL,
	                           on_name_lost,
	                           NULL,
	                           NULL);

	/* do stuff on ctrl-c and for SIGTERM */
	g_unix_signal_add_full (G_PRIORITY_DEFAULT,
				SIGINT,
				urf_main_signal_cb,
				loop,
				NULL);
	g_unix_signal_add_full (G_PRIORITY_DEFAULT,
				SIGTERM,
				urf_main_signal_cb,
				loop,
				NULL);

	g_message ("Starting urfkilld version %s", PACKAGE_VERSION);

	/* start the daemon */
	daemon = urf_daemon_new (config);
	ret = urf_daemon_startup (daemon);
	if (!ret) {
		g_warning ("Could not startup; bailing out");
		goto out;
	}

	if (!username)
		username = urf_config_get_user (config);

	if (username != NULL && g_strcmp0 (username, "root") != 0) {
		/* Change uid/gid to a specific user and drop privilege */
		if (!(user = getpwnam (username))) {
			g_warning ("Can't get %s's uid and gid", username);
			goto out;
		}
		if (initgroups (username, user->pw_gid) != 0) {
			g_warning ("initgroups failed");
			goto out;
		}
		if (setgid (user->pw_gid) != 0 || setuid (user->pw_uid) != 0) {
			g_warning ("Can't drop privilege");
			goto out;
		}
	}

	/* only timeout and close the mainloop if we have specified it on the command line */
	if (timed_exit) {
		timer_id = g_timeout_add_seconds (30, (GSourceFunc) urf_main_timed_exit_cb, loop);
		g_source_set_name_by_id (timer_id, "[UrfMain] idle");
	}

	/* immediately exit */
	if (immediate_exit)
		g_timeout_add (50, (GSourceFunc) urf_main_timed_exit_cb, loop);

	/* fork as daemon Clone ourselves to make a child */
	if(fork_daemon){
		pid = fork();

		/* If the pid is less than zero,
		   something went wrong when forking */
		if (pid < 0) {
			return pid;
		}

		/* If the pid we got back was greater
		   than zero, then the clone was
		   successful and we are the parent. */
		if (pid > 0) {
			return 0;
		}

		/* If execution reaches this point we are the child */
	}

	/* wait for input or timeout */
	g_main_loop_run (loop);
	retval = 0;
	g_bus_unown_name (owner_id);
out:
	if (daemon != NULL)
		g_object_unref (daemon);
	if (config != NULL)
		g_object_unref (config);
	if (loop != NULL)
		g_main_loop_unref (loop);

	closelog();

	return retval;
}
