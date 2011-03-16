/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@novell.com>
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

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "urf-config.h"
#include "urf-daemon.h"

#define URFKILL_SERVICE_NAME "org.freedesktop.URfkill"
static GMainLoop *loop = NULL;

/**
 * urf_main_acquire_name_on_proxy:
 **/
static gboolean
urf_main_acquire_name_on_proxy (DBusGProxy  *bus_proxy,
				const gchar *name)
{
	GError *error = NULL;
	guint result;
	gboolean ret = FALSE;

	if (bus_proxy == NULL)
		goto out;

	ret = dbus_g_proxy_call (bus_proxy, "RequestName", &error,
        			 G_TYPE_STRING, name,
				 G_TYPE_UINT, 0,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, &result,
				 G_TYPE_INVALID);
	if (!ret) {
		if (error != NULL) {
			g_warning ("Failed to acquire %s: %s", name, error->message);
			g_error_free (error);
		} else {
			g_warning ("Failed to acquire %s", name);
		}
		goto out;
	}

	/* already taken */
	if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		if (error != NULL) {
			g_warning ("Failed to acquire %s: %s", name, error->message);
			g_error_free (error);
		} else {
			g_warning ("Failed to acquire %s", name);
		}
		ret = FALSE;
		goto out;
	}
out:
	return ret;
}

/**
 * urf_main_sigint_handler:
 **/
static void
urf_main_sigint_handler (gint sig)
{
	g_debug ("Handling SIGINT");

	/* restore default */
	signal (SIGINT, SIG_DFL);

	/* cleanup */
	g_main_loop_quit (loop);
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

/**
 * main:
 **/
gint
main (gint argc, gchar **argv)
{
	GError *error = NULL;
	UrfConfig *config = NULL;
	UrfDaemon *daemon = NULL;
	GOptionContext *context;
	DBusGProxy *bus_proxy;
	DBusGConnection *bus;
	gboolean ret;
	gint retval = 1;
	gboolean timed_exit = FALSE;
	gboolean immediate_exit = FALSE;
	guint timer_id = 0;
	struct passwd *user;
	const char *username = NULL;
	const char *conf_file = NULL;

	const GOptionEntry options[] = {
		{ "timed-exit", '\0', 0, G_OPTION_ARG_NONE, &timed_exit,
		  /* TRANSLATORS: exit after we've started up, used for user profiling */
		  _("Exit after a small delay"), NULL },
		{ "immediate-exit", '\0', 0, G_OPTION_ARG_NONE, &immediate_exit,
		  /* TRANSLATORS: exit straight away, used for automatic profiling */
		  _("Exit after the engine has loaded"), NULL },
		{ "user", 'u', 0, G_OPTION_ARG_STRING, &username,
		  /* TRANSLATORS: change to another user and drop the privilege */
		  _("Use a specific user instead of root"), NULL },
		{ "config", 'c', 0, G_OPTION_ARG_STRING, &conf_file,
		  /* TRANSLATORS: use another config file instead of the default one */
		  _("Use a specific config file"), NULL },
		{ NULL }
	};

	g_type_init ();

	context = g_option_context_new ("urfkill daemon");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	if (conf_file == NULL)
		conf_file = PACKAGE_SYSCONF_DIR"/urfkill.conf";

	config = urf_config_new ();
	urf_config_load_from_file (config, conf_file);

	/* get bus connection */
	bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (bus == NULL) {
		g_warning ("Couldn't connect to system bus: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* get proxy */
	bus_proxy = dbus_g_proxy_new_for_name (bus, DBUS_SERVICE_DBUS,
					       DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	if (bus_proxy == NULL) {
		g_warning ("Could not construct bus_proxy object; bailing out");
		goto out;
	}

	/* aquire name */
        ret = urf_main_acquire_name_on_proxy (bus_proxy, URFKILL_SERVICE_NAME);
	if (!ret) {
		g_warning ("Could not acquire name; bailing out");
		goto out;
	}

	/* do stuff on ctrl-c */
	signal (SIGINT, urf_main_sigint_handler);

	g_debug ("Starting urfkilld version %s", PACKAGE_VERSION);

	/* start the daemon */
	daemon = urf_daemon_new ();
	loop = g_main_loop_new (NULL, FALSE);
	ret = urf_daemon_startup (daemon, config);
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
#if GLIB_CHECK_VERSION(2,25,8)
		g_source_set_name_by_id (timer_id, "[UrfMain] idle");
#endif
	}

	/* immediatly exit */
	if (immediate_exit)
		g_timeout_add (50, (GSourceFunc) urf_main_timed_exit_cb, loop);

	/* wait for input or timeout */
	g_main_loop_run (loop);
	retval = 0;
out:
	if (daemon != NULL)
		g_object_unref (daemon);
	if (config != NULL)
		g_object_unref (config);
	if (loop != NULL)
		g_main_loop_unref (loop);
	return retval;
}
