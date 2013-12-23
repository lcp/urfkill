/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
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

#include <glib.h>
#include <string.h>
#include <gio/gio.h>

#include "urf-session-checker-consolekit.h"

typedef struct {
	guint		 cookie;
	char		*session_id;
	char		*bus_name;
	char		*reason;
} UrfInhibitor;

struct UrfConsolekitPrivate {
	GDBusProxy	*proxy;
	GDBusProxy	*bus_proxy;
	GList		*seats;
	GList		*inhibitors;
	gboolean	 inhibit;
};

G_DEFINE_TYPE (UrfSessionChecker, urf_session_checker, G_TYPE_OBJECT)

#define URF_SESSION_CHECKER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_SESSION_CHECKER, UrfConsolekitPrivate))

/**
 * urf_session_checker_is_inhibited:
 **/
gboolean
urf_session_checker_is_inhibited (UrfSessionChecker *consolekit)
{
	return consolekit->priv->inhibit;
}

/**
 * urf_session_checker_find_seat:
 **/
static UrfSeat *
urf_session_checker_find_seat (UrfSessionChecker *consolekit,
			  const char    *object_path)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfSeat *seat;
	GList *item;

	for (item = priv->seats; item; item = item->next) {
		seat = URF_SEAT (item->data);
		if (g_strcmp0 (urf_seat_get_object_path (seat), object_path) == 0)
			return seat;
	}

	return NULL;
}

static UrfInhibitor *
find_inhibitor_by_sid (UrfSessionChecker *consolekit,
		       const char    *session_id)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;
	GList *item;

	for (item = priv->inhibitors; item; item = item->next) {
		inhibitor = (UrfInhibitor *)item->data;
		if (g_strcmp0 (inhibitor->session_id, session_id) == 0)
			return inhibitor;
	}
	return NULL;
}

static UrfInhibitor *
find_inhibitor_by_bus_name (UrfSessionChecker *consolekit,
			    const char    *bus_name)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;
	GList *item;

	for (item = priv->inhibitors; item; item = item->next) {
		inhibitor = (UrfInhibitor *)item->data;
		if (g_strcmp0 (inhibitor->bus_name, bus_name) == 0)
			return inhibitor;
	}
	return NULL;
}

static UrfInhibitor *
find_inhibitor_by_cookie (UrfSessionChecker *consolekit,
			  const guint    cookie)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;
	GList *item;

	if (cookie == 0)
		return NULL;

	for (item = priv->inhibitors; item; item = item->next) {
		inhibitor = (UrfInhibitor *)item->data;
		if (inhibitor->cookie == cookie)
			return inhibitor;
	}
	return NULL;
}

static gboolean
is_inhibited (UrfSessionChecker *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfSeat *seat;
	const char *active_id;
	GList *item;

	for (item = priv->seats; item; item = item->next) {
		seat = URF_SEAT (item->data);
		active_id = urf_seat_get_active (seat);
		if (find_inhibitor_by_sid (consolekit, active_id))
			return TRUE;
	}

	return FALSE;
}

static void
free_inhibitor (UrfInhibitor *inhibitor)
{
	g_free (inhibitor->session_id);
	g_free (inhibitor->bus_name);
	g_free (inhibitor->reason);
	g_free (inhibitor);
}

/**
 * urf_session_checker_seat_active_changed:
 **/
static void
urf_session_checker_seat_active_changed (UrfSeat       *seat,
				    const char    *session_id,
				    UrfSessionChecker *consolekit)
{
	consolekit->priv->inhibit = is_inhibited (consolekit);
	g_debug ("Active Session changed: %s", session_id);
}

static char *
get_session_id (UrfSessionChecker *consolekit,
		const char    *bus_name)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	pid_t calling_pid;
	char *session_id = NULL;
	GVariant *retval;
	GError *error;

        error = NULL;
	retval = g_dbus_proxy_call_sync (priv->bus_proxy, "GetConnectionUnixProcessID",
	                                 g_variant_new ("(s)", bus_name),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error);
	if (error) {
		g_warning("GetConnectionUnixProcessID() failed: %s", error->message);
		g_error_free (error);
		goto out;
	}
	g_variant_get (retval, "(u)", &calling_pid);
	g_variant_unref (retval);

        error = NULL;
	retval = g_dbus_proxy_call_sync (priv->proxy, "GetSessionForUnixProcess",
	                                 g_variant_new ("(u)", (guint)calling_pid),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error);
	if (error) {
		g_warning ("Couldn't sent GetSessionForUnixProcess: %s", error->message);
		g_error_free (error);
		session_id = NULL;
		goto out;
	}

	g_variant_get (retval, "(s)", &session_id);
	session_id = g_strdup (session_id);
	g_variant_unref (retval);
out:
	return session_id;
}

static guint
generate_unique_cookie (UrfSessionChecker *consolekit)
{
	UrfInhibitor *inhibitor;
	guint cookie;

	do {
		cookie = g_random_int_range (1, G_MAXINT);
		inhibitor = find_inhibitor_by_cookie (consolekit, cookie);
	} while (inhibitor != NULL);

	return cookie;
}

/**
 * urf_session_checker_inhibit:
 **/
guint
urf_session_checker_inhibit (UrfSessionChecker *consolekit,
			const char    *bus_name,
			const char    *reason)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;

	g_return_val_if_fail (priv->proxy != NULL, 0);

	inhibitor = find_inhibitor_by_bus_name (consolekit, bus_name);
	if (inhibitor)
		return inhibitor->cookie;

	inhibitor = g_new0 (UrfInhibitor, 1);
	inhibitor->session_id = get_session_id (consolekit, bus_name);
	if (inhibitor->session_id == NULL) {
		g_free (inhibitor);
		return 0;
	}

	inhibitor->reason = g_strdup (reason);
	inhibitor->bus_name = g_strdup (bus_name);
	inhibitor->cookie = generate_unique_cookie (consolekit);

	priv->inhibitors = g_list_prepend (priv->inhibitors, inhibitor);

	consolekit->priv->inhibit = is_inhibited (consolekit);
	g_debug ("Inhibit: %s for %s", bus_name, reason);

	return inhibitor->cookie;
}

static void
remove_inhibitor (UrfSessionChecker *consolekit,
		  UrfInhibitor  *inhibitor)
{
	UrfConsolekitPrivate *priv = consolekit->priv;

	g_return_if_fail (priv->proxy != NULL);

	priv->inhibitors = g_list_remove (priv->inhibitors, inhibitor);
	consolekit->priv->inhibit = is_inhibited (consolekit);
	g_debug ("Remove inhibitor: %s", inhibitor->bus_name);
	free_inhibitor (inhibitor);
}

/**
 * urf_session_checker_uninhibit:
 **/
void
urf_session_checker_uninhibit (UrfSessionChecker *consolekit,
			  const guint    cookie)
{
	UrfInhibitor *inhibitor;

	inhibitor = find_inhibitor_by_cookie (consolekit, cookie);
	if (inhibitor == NULL) {
		g_debug ("Cookie outdated");
		return;
	}
	remove_inhibitor (consolekit, inhibitor);
}

/**
 * urf_session_checker_add_seat:
 **/
static void
urf_session_checker_add_seat (UrfSessionChecker *consolekit,
			 const char    *object_path)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfSeat *seat = urf_seat_new ();
	gboolean ret;

	ret = urf_seat_object_path_sync (seat, object_path);

	if (!ret) {
		g_warning ("Failed to sync %s", object_path);
		return;
	}

	priv->seats = g_list_prepend (priv->seats, seat);

	/* connect signal */
	g_signal_connect (seat, "active-changed",
			  G_CALLBACK (urf_session_checker_seat_active_changed),
			  consolekit);
}

/**
 * urf_session_checker_seat_added:
 **/
static void
urf_session_checker_seat_added (UrfSessionChecker *consolekit,
                           const char    *object_path)
{
	if (urf_session_checker_find_seat (consolekit, object_path) != NULL) {
		g_debug ("Already added seat: %s", object_path);
		return;
	}

	urf_session_checker_add_seat (consolekit, object_path);
	g_debug ("Monitor seat: %s", object_path);
}

/**
 * urf_session_checker_seat_removed:
 **/
static void
urf_session_checker_seat_removed (UrfSessionChecker *consolekit,
                             const char    *object_path)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfSeat *seat;

	seat = urf_session_checker_find_seat (consolekit, object_path);
	if (seat == NULL)
		return;

	priv->seats = g_list_remove (priv->seats, seat);

	g_object_unref (seat);
	g_debug ("Removed seat: %s", object_path);
}

/**
 * urf_session_checker_proxy_signal_cb
 **/
static void
urf_session_checker_proxy_signal_cb (GDBusProxy *proxy,
                                gchar      *sender_name,
                                gchar      *signal_name,
                                GVariant   *parameters,
                                gpointer    user_data)
{
	UrfSessionChecker *consolekit = URF_SESSION_CHECKER (user_data);
	char *seat_path;

	if (g_strcmp0 (signal_name, "SeatAdded") == 0) {
		g_variant_get (parameters, "(o)", &seat_path);
		urf_session_checker_seat_added (consolekit, seat_path);
	} else if (g_strcmp0 (signal_name, "SeatRemoved") == 0) {
		g_variant_get (parameters, "(o)", &seat_path);
		urf_session_checker_seat_removed (consolekit, seat_path);
	}
}

/**
 * urf_session_checker_bus_owner_changed:
 **/
static void
urf_session_checker_bus_owner_changed (UrfSessionChecker *consolekit,
                                  const char    *old_owner,
                                  const char    *new_owner)
{
	UrfInhibitor *inhibitor;

	if (strlen (new_owner) == 0 &&
	    strlen (old_owner) > 0) {
		/* A process disconnected from the bus */
		inhibitor = find_inhibitor_by_bus_name (consolekit, old_owner);
		if (inhibitor == NULL)
			return;
		remove_inhibitor (consolekit, inhibitor);
	}
}

/**
 * urf_session_checker_bus_proxy_signal_cb
 **/
static void
urf_session_checker_bus_proxy_signal_cb (GDBusProxy *proxy,
                                    gchar      *sender_name,
                                    gchar      *signal_name,
                                    GVariant   *parameters,
                                    gpointer    user_data)
{
	UrfSessionChecker *consolekit = URF_SESSION_CHECKER (user_data);
	char *name;
	char *old_owner;
	char *new_owner;

	if (g_strcmp0 (signal_name, "NameOwnerChanged") == 0) {
		g_variant_get (parameters, "(sss)", &name, &old_owner, &new_owner);
		urf_session_checker_bus_owner_changed (consolekit, old_owner, new_owner);
	}
}

/**
 * urf_session_checker_get_seats:
 **/
static gboolean
urf_session_checker_get_seats (UrfSessionChecker *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	GError *error = NULL;
	const char *seat_name;
	GVariant *retval;
	GVariantIter *iter;

	retval = g_dbus_proxy_call_sync (priv->proxy, "GetSeats",
	                                 NULL,
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error);
	if (error) {
		g_warning ("GetSeats Failed: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	if (retval == NULL) {
		g_debug ("No Seat exists");
		return FALSE;
	}

	g_variant_get (retval, "(ao)", &iter);
	while (g_variant_iter_loop (iter, "o", &seat_name)) {
		urf_session_checker_add_seat (consolekit, seat_name);
		g_debug ("Added seat: %s", seat_name);
	}
	g_variant_iter_free (iter);
	g_variant_unref (retval);

	return TRUE;
}

/**
 * urf_session_checker_startup:
 **/
gboolean
urf_session_checker_startup (UrfSessionChecker *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	GError *error;
	gboolean ret;

	error = NULL;
	priv->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                             G_DBUS_PROXY_FLAGS_NONE,
	                                             NULL,
	                                             "org.freedesktop.ConsoleKit",
	                                             "/org/freedesktop/ConsoleKit/Manager",
	                                             "org.freedesktop.ConsoleKit.Manager",
	                                             NULL,
	                                             &error);
	if (error) {
		g_error ("failed to setup proxy for consolekit: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	error = NULL;
	priv->bus_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                                 G_DBUS_PROXY_FLAGS_NONE,
	                                                 NULL,
	                                                 "org.freedesktop.DBus",
	                                                 "/org/freedesktop/DBus",
	                                                 "org.freedesktop.DBus",
	                                                 NULL,
	                                                 &error);
	if (error) {
		g_error ("failed to setup proxy for consolekit: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	/* Get seats */
	ret = urf_session_checker_get_seats (consolekit);
	if (!ret)
		return FALSE;

	/* connect signals */
	g_signal_connect (G_OBJECT (priv->proxy), "g-signal",
	                  G_CALLBACK (urf_session_checker_proxy_signal_cb), consolekit);
	g_signal_connect (G_CALLBACK (priv->bus_proxy), "g-signal",
	                  G_CALLBACK (urf_session_checker_bus_proxy_signal_cb), consolekit);

	return TRUE;
}

/**
 * urf_session_checker_dispose:
 **/
static void
urf_session_checker_dispose (GObject *object)
{
	UrfSessionChecker *consolekit = URF_SESSION_CHECKER(object);

	if (consolekit->priv->proxy) {
		g_object_unref (consolekit->priv->proxy);
		consolekit->priv->proxy = NULL;
	}
	if (consolekit->priv->bus_proxy) {
		g_object_unref (consolekit->priv->bus_proxy);
		consolekit->priv->bus_proxy = NULL;
	}

	G_OBJECT_CLASS (urf_session_checker_parent_class)->dispose (object);
}

/**
 * urf_session_checker_finalize:
 **/
static void
urf_session_checker_finalize (GObject *object)
{
	UrfSessionChecker *consolekit = URF_SESSION_CHECKER (object);
	GList *item;

	if (consolekit->priv->seats) {
		for (item = consolekit->priv->seats; item; item = item->next)
			g_object_unref (item->data);
		g_list_free (consolekit->priv->seats);
		consolekit->priv->seats = NULL;
	}
	if (consolekit->priv->inhibitors) {
		for (item = consolekit->priv->inhibitors; item; item = item->next)
			free_inhibitor (item->data);
		g_list_free (consolekit->priv->inhibitors);
		consolekit->priv->inhibitors = NULL;
	}

	G_OBJECT_CLASS (urf_session_checker_parent_class)->finalize (object);
}

/**
 * urf_session_checker_class_init:
 **/
static void
urf_session_checker_class_init (UrfSessionCheckerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = urf_session_checker_dispose;
	object_class->finalize = urf_session_checker_finalize;

	g_type_class_add_private (klass, sizeof (UrfConsolekitPrivate));
}

/**
 * urf_session_checker_init:
 **/
static void
urf_session_checker_init (UrfSessionChecker *consolekit)
{
	consolekit->priv = URF_SESSION_CHECKER_GET_PRIVATE (consolekit);
	consolekit->priv->seats = NULL;
	consolekit->priv->inhibitors = NULL;
	consolekit->priv->inhibit = FALSE;
	consolekit->priv->proxy = NULL;
	consolekit->priv->bus_proxy = NULL;
}

/**
 * urf_session_checker_new:
 **/
UrfSessionChecker *
urf_session_checker_new (void)
{
	UrfSessionChecker *consolekit;
	consolekit = URF_SESSION_CHECKER (g_object_new (URF_TYPE_SESSION_CHECKER, NULL));
	return consolekit;
}
