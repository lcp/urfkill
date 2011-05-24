/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@novell.com>
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
#include <dbus/dbus-glib.h>

#include "urf-consolekit.h"

typedef struct {
	guint		 cookie;
	char		*session_id;
	char		*bus_name;
	char		*reason;
} UrfInhibitor;

struct UrfConsolekitPrivate {
	DBusGConnection	*connection;
	DBusGProxy	*proxy;
	DBusGProxy	*bus_proxy;
	GList		*seats;
	GList		*inhibitors;
	gboolean	 inhibit;
};

G_DEFINE_TYPE (UrfConsolekit, urf_consolekit, G_TYPE_OBJECT)

#define URF_CONSOLEKIT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_CONSOLEKIT, UrfConsolekitPrivate))

/**
 * urf_consolekit_is_inhibited:
 **/
gboolean
urf_consolekit_is_inhibited (UrfConsolekit *consolekit)
{
	return consolekit->priv->inhibit;
}

/**
 * urf_consolekit_find_seat:
 **/
static UrfSeat *
urf_consolekit_find_seat (UrfConsolekit *consolekit,
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
find_inhibitor_by_sid (UrfConsolekit *consolekit,
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
find_inhibitor_by_bus_name (UrfConsolekit *consolekit,
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
find_inhibitor_by_cookie (UrfConsolekit *consolekit,
			  const guint    cookie)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;
	GList *item;

	for (item = priv->inhibitors; item; item = item->next) {
		inhibitor = (UrfInhibitor *)item->data;
		if (inhibitor->cookie == cookie)
			return inhibitor;
	}
	return NULL;
}

static gboolean
is_inhibited (UrfConsolekit *consolekit)
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
 * urf_consolekit_seat_active_changed:
 **/
static void
urf_consolekit_seat_active_changed (UrfSeat       *seat,
				    const char    *session_id,
				    UrfConsolekit *consolekit)
{
	consolekit->priv->inhibit = is_inhibited (consolekit);
	g_debug ("Active Session changed: %s", session_id);
}

/**
 * urf_consolekit_seat_active_changed:
 **/
static void
urf_consolekit_seat_session_removed (UrfSeat       *seat,
				     const char    *session_id,
				     UrfConsolekit *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;

	inhibitor = find_inhibitor_by_sid (consolekit, session_id);
	if (inhibitor == NULL)
		return;

	priv->inhibitors = g_list_remove (priv->inhibitors, inhibitor);
	free_inhibitor (inhibitor);
	g_debug ("Session removed: %s", session_id);
}

static char *
get_session_id (UrfConsolekit *consolekit,
		const char    *bus_name)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	pid_t calling_pid;
	char *session_id = NULL;
	GError *error;

        error = NULL;
	if (!dbus_g_proxy_call (priv->bus_proxy, "GetConnectionUnixProcessID", &error,
				G_TYPE_STRING, bus_name,
				G_TYPE_INVALID,
				G_TYPE_UINT, &calling_pid,
				G_TYPE_INVALID)) {
		g_warning("GetConnectionUnixProcessID() failed: %s", error->message);
		g_error_free (error);
		goto out;
	}

        error = NULL;
	if (!dbus_g_proxy_call (priv->proxy, "GetSessionForUnixProcess", &error,
				G_TYPE_UINT, calling_pid,
				G_TYPE_INVALID,
				DBUS_TYPE_G_OBJECT_PATH, &session_id,
				G_TYPE_INVALID)) {
		g_warning ("Couldn't sent GetSessionForUnixProcess: %s", error->message);
		g_error_free (error);
		session_id = NULL;
		goto out;
	}

	session_id = g_strdup (session_id);
out:
	return session_id;
}

static guint
generate_unique_cookie (UrfConsolekit *consolekit)
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
 * urf_consolekit_inhibit:
 **/
guint
urf_consolekit_inhibit (UrfConsolekit *consolekit,
			const char    *bus_name,
			const char    *reason)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfInhibitor *inhibitor;

	inhibitor = find_inhibitor_by_bus_name (consolekit, bus_name);
	if (inhibitor)
		return inhibitor->cookie;

	inhibitor = g_new0 (UrfInhibitor, 1);
	inhibitor->reason = g_strdup (reason);
	inhibitor->bus_name = g_strdup (bus_name);
	inhibitor->session_id = get_session_id (consolekit, bus_name);
	inhibitor->cookie = generate_unique_cookie (consolekit);

	priv->inhibitors = g_list_prepend (priv->inhibitors, inhibitor);

	consolekit->priv->inhibit = is_inhibited (consolekit);
	g_debug ("Inhibit: %s for %s", bus_name, reason);

	return inhibitor->cookie;
}

static void
remove_inhibitor (UrfConsolekit *consolekit,
		  UrfInhibitor  *inhibitor)
{
	UrfConsolekitPrivate *priv = consolekit->priv;

	priv->inhibitors = g_list_remove (priv->inhibitors, inhibitor);
	consolekit->priv->inhibit = is_inhibited (consolekit);
	g_debug ("Remove inhibitor: %s", inhibitor->bus_name);
	free_inhibitor (inhibitor);
}

/**
 * urf_consolekit_uninhibit:
 **/
void
urf_consolekit_uninhibit (UrfConsolekit *consolekit,
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
 * urf_consolekit_add_seat:
 **/
static void
urf_consolekit_add_seat (UrfConsolekit *consolekit,
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
			  G_CALLBACK (urf_consolekit_seat_active_changed),
			  consolekit);
	g_signal_connect (seat, "session_removed",
			  G_CALLBACK (urf_consolekit_seat_session_removed),
			  consolekit);
}

/**
 * urf_consolekit_seat_added_cb:
 **/
static void
urf_consolekit_seat_added_cb (DBusGProxy    *proxy,
			      const char    *object_path,
			      UrfConsolekit *consolekit)
{
	if (urf_consolekit_find_seat (consolekit, object_path) != NULL) {
		g_debug ("Already added seat: %s", object_path);
		return;
	}

	urf_consolekit_add_seat (consolekit, object_path);
	g_debug ("Monitor seat: %s", object_path);
}

/**
 * urf_consolekit_seat_removed_cb:
 **/
static void
urf_consolekit_seat_removed_cb (DBusGProxy    *proxy,
				const char    *object_path,
				UrfConsolekit *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	UrfSeat *seat;

	seat = urf_consolekit_find_seat (consolekit, object_path);
	if (seat == NULL)
		return;

	priv->seats = g_list_remove (priv->seats, seat);

	g_object_unref (seat);
	g_debug ("Removed seat: %s", object_path);
}

/**
 * urf_consolekit_bus_owner_changed_cb:
 **/
static void
urf_consolekit_bus_owner_changed_cb (DBusGProxy    *bus_proxy,
				     const char    *name,
				     const char    *old_owner,
				     const char    *new_owner,
				     UrfConsolekit *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
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
 * urf_consolekit_get_seats:
 **/
static gboolean
urf_consolekit_get_seats (UrfConsolekit *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	GType g_type_array;
	GPtrArray *seats = NULL;
	GError *error = NULL;
	const char *object_path;
	gboolean ret;
	int i;

	g_type_array = dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH);
	ret = dbus_g_proxy_call (priv->proxy, "GetSeats", &error,
				 G_TYPE_INVALID,
				 g_type_array, &seats,
				 G_TYPE_INVALID);

	if (!ret) {
		g_warning ("GetSeats Failed: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	if (seats == NULL) {
		g_debug ("No Seat exists");
		return FALSE;
	}

	for (i = 0; i < seats->len; i++) {
		object_path = (const char *) g_ptr_array_index (seats, i);
		urf_consolekit_add_seat (consolekit, object_path);
		g_debug ("Added seat: %s", object_path);
	}

	return TRUE;
}

/**
 * urf_consolekit_startup:
 **/
gboolean
urf_consolekit_startup (UrfConsolekit *consolekit)
{
	UrfConsolekitPrivate *priv = consolekit->priv;
	GError *error = NULL;
	gboolean ret;

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_warning ("Failed to get bus: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
						 "org.freedesktop.ConsoleKit",
						 "/org/freedesktop/ConsoleKit/Manager",
						 "org.freedesktop.ConsoleKit.Manager");

	priv->bus_proxy = dbus_g_proxy_new_for_name (priv->connection,
						     DBUS_SERVICE_DBUS,
						     DBUS_PATH_DBUS,
						     DBUS_INTERFACE_DBUS);
	/* Get seats */
	ret = urf_consolekit_get_seats (consolekit);
	if (!ret)
		return FALSE;

	/* connect signals */
	dbus_g_proxy_add_signal (priv->proxy, "SeatAdded",
				 G_TYPE_STRING,
				 G_TYPE_INVALID);
	dbus_g_proxy_add_signal (priv->proxy, "SeatRemoved",
				 G_TYPE_STRING,
				 G_TYPE_INVALID);

	dbus_g_proxy_add_signal (priv->bus_proxy, "NameOwnerChanged",
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_INVALID);
	/* callbacks */
	dbus_g_proxy_connect_signal (priv->proxy, "SeatAdded",
				     G_CALLBACK (urf_consolekit_seat_added_cb), consolekit, NULL);
	dbus_g_proxy_connect_signal (priv->proxy, "SeatRemoved",
				     G_CALLBACK (urf_consolekit_seat_removed_cb), consolekit, NULL);

	dbus_g_proxy_connect_signal (priv->bus_proxy, "NameOwnerChanged",
				     G_CALLBACK (urf_consolekit_bus_owner_changed_cb), consolekit, NULL);

	return TRUE;
}

/**
 * urf_consolekit_dispose:
 **/
static void
urf_consolekit_dispose (GObject *object)
{
	UrfConsolekit *consolekit = URF_CONSOLEKIT(object);

	if (consolekit->priv->connection) {
		dbus_g_connection_unref (consolekit->priv->connection);
		consolekit->priv->connection = NULL;
	}
	if (consolekit->priv->proxy) {
		g_object_unref (consolekit->priv->proxy);
		consolekit->priv->proxy = NULL;
	}
	if (consolekit->priv->bus_proxy) {
		g_object_unref (consolekit->priv->bus_proxy);
		consolekit->priv->bus_proxy = NULL;
	}

	G_OBJECT_CLASS (urf_consolekit_parent_class)->dispose (object);
}

/**
 * urf_consolekit_finalize:
 **/
static void
urf_consolekit_finalize (GObject *object)
{
	UrfConsolekit *consolekit = URF_CONSOLEKIT (object);

	if (consolekit->priv->seats)
		g_list_free_full (consolekit->priv->seats, g_object_unref);
	if (consolekit->priv->inhibitors) {
		g_list_free_full (consolekit->priv->seats,
				  (GDestroyNotify)free_inhibitor);
	}

	G_OBJECT_CLASS (urf_consolekit_parent_class)->finalize (object);
}

/**
 * urf_consolekit_class_init:
 **/
static void
urf_consolekit_class_init (UrfConsolekitClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = urf_consolekit_dispose;
	object_class->finalize = urf_consolekit_finalize;

	g_type_class_add_private (klass, sizeof (UrfConsolekitPrivate));
}

/**
 * urf_consolekit_init:
 **/
static void
urf_consolekit_init (UrfConsolekit *consolekit)
{
	consolekit->priv = URF_CONSOLEKIT_GET_PRIVATE (consolekit);
	consolekit->priv->seats = NULL;
	consolekit->priv->inhibitors = NULL;
	consolekit->priv->inhibit = FALSE;
	consolekit->priv->connection = NULL;
	consolekit->priv->proxy = NULL;
	consolekit->priv->bus_proxy = NULL;
}

/**
 * urf_consolekit_new:
 **/
UrfConsolekit *
urf_consolekit_new (void)
{
	UrfConsolekit *consolekit;
	consolekit = URF_CONSOLEKIT (g_object_new (URF_TYPE_CONSOLEKIT, NULL));
	return consolekit;
}
