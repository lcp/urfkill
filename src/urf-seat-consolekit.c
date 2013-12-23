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
#include <gio/gio.h>

#include "urf-seat-consolekit.h"

enum {
	SIGNAL_ACTIVE_CHANGED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };


struct UrfSeatPrivate {
	GDBusConnection	*connection;
	GDBusProxy	*proxy;
	char		*object_path;
	char		*active;
};

G_DEFINE_TYPE (UrfSeat, urf_seat, G_TYPE_OBJECT)

#define URF_SEAT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_SEAT, UrfSeatPrivate))

/**
 * urf_seat_get_object_path:
 **/
const char *
urf_seat_get_object_path (UrfSeat *seat)
{
	return seat->priv->object_path;
}

/**
 * urf_seat_get_active:
 **/
const char *
urf_seat_get_active (UrfSeat *seat)
{
	return seat->priv->active;
}

/**
 * urf_seat_proxy_signal_cb:
 **/
static void
urf_seat_proxy_signal_cb (GDBusProxy *proxy,
                          gchar      *sender_name,
                          gchar      *signal_name,
                          GVariant   *parameters,
                          gpointer    user_data)
{
	UrfSeat *seat = URF_SEAT (user_data);
	char *session_id;

	if (g_strcmp0 (signal_name, "ActiveSessionChanged") == 0) {
		g_variant_get (parameters, "(s)", &session_id);

		g_free (seat->priv->active);
		seat->priv->active = g_strdup (session_id);

		g_signal_emit (seat, signals[SIGNAL_ACTIVE_CHANGED], 0, session_id);
	}
}

/**
 * urf_seat_object_path_sync:
 **/
gboolean
urf_seat_object_path_sync (UrfSeat    *seat,
			   const char *object_path)
{
	UrfSeatPrivate *priv = seat->priv;
	GVariant *retval;
	char *session;
	GError *error;

	priv->object_path = g_strdup (object_path);

	error = NULL;
	priv->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                             G_DBUS_PROXY_FLAGS_NONE,
	                                             NULL,
	                                             "org.freedesktop.ConsoleKit",
	                                             priv->object_path,
	                                             "org.freedesktop.ConsoleKit.Seat",
	                                             NULL,
	                                             &error);
	if (error) {
		g_error ("failed to setup proxy for consolekit seat: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	error = NULL;
	retval = g_dbus_proxy_call_sync (priv->proxy, "GetActiveSession",
	                                 NULL,
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error);
	if (error) {
		g_warning ("Failed to get Active Session: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	g_variant_get (retval, "(o)", &session);
	priv->active = g_strdup (session);
	g_variant_unref (retval);

	/* connect signals */
	g_signal_connect (G_OBJECT (priv->proxy), "g-signal",
	                  G_CALLBACK (urf_seat_proxy_signal_cb), seat);

	return TRUE;
}

/**
 * urf_seat_dispose:
 **/
static void
urf_seat_dispose (GObject *object)
{
	UrfSeat *seat = URF_SEAT (object);

	if (seat->priv->proxy) {
		g_object_unref (seat->priv->proxy);
		seat->priv->proxy = NULL;
	}

	G_OBJECT_CLASS (urf_seat_parent_class)->dispose (object);
}

/**
 * urf_seat_finalize:
 **/
static void
urf_seat_finalize (GObject *object)
{
	UrfSeat *seat = URF_SEAT (object);

	g_free (seat->priv->object_path);
	g_free (seat->priv->active);

	G_OBJECT_CLASS (urf_seat_parent_class)->finalize (object);
}

/**
 * urf_seat_class_init:
 **/
static void
urf_seat_class_init (UrfSeatClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = urf_seat_dispose;
	object_class->finalize = urf_seat_finalize;

	signals[SIGNAL_ACTIVE_CHANGED] =
		g_signal_new ("active-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfSeatClass, active_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	g_type_class_add_private (klass, sizeof (UrfSeatPrivate));
}

/**
 * urf_seat_init:
 **/
static void
urf_seat_init (UrfSeat *seat)
{
	seat->priv = URF_SEAT_GET_PRIVATE (seat);
	seat->priv->object_path = NULL;
	seat->priv->active = NULL;
}

/**
 * urf_seat_new:
 **/
UrfSeat *
urf_seat_new (void)
{
	UrfSeat *seat;
	seat = URF_SEAT (g_object_new (URF_TYPE_SEAT, NULL));
	return seat;
}
