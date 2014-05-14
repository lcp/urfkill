/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Mathieu Trudel-Lapierre <mathieu.trudel-lapierre@canonical.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include "urf-arbitrator.h"
#include "urf-ofono-manager.h"
#include "urf-device.h"
#include "urf-device-ofono.h"

struct _UrfOfonoManager {
	GObject parent_instance;

	UrfArbitrator *arbitrator;

	GDBusProxy *proxy;
	GCancellable *cancellable;
	int watch_id;

	GSList *devices;
};

typedef GObjectClass UrfOfonoManagerClass;

G_DEFINE_TYPE (UrfOfonoManager, urf_ofono_manager, G_TYPE_OBJECT)

static gint modem_idx = 100;

static void
urf_ofono_manager_finalize (GObject *object)
{
	UrfOfonoManager *ofono;

	g_return_if_fail (URF_IS_OFONO_MANAGER (object));
	ofono = URF_OFONO_MANAGER (object);

	if (ofono->proxy) {
		g_object_unref (ofono->proxy);
		ofono->proxy = NULL;
	}

	if (ofono->arbitrator) {
		g_object_unref (ofono->arbitrator);
		ofono->arbitrator = NULL;
	}

	if (ofono->devices) {
		g_slist_free_full (ofono->devices, g_object_unref);
		ofono->devices = NULL;
	}

	G_OBJECT_CLASS (urf_ofono_manager_parent_class)->finalize (object);
}

static void
urf_ofono_manager_add_modem (UrfOfonoManager *ofono,
                             const char *object_path)
{
	UrfDevice *device;

	device = urf_device_ofono_new (modem_idx, object_path);
	modem_idx++;

	/* Keep our own list to avoid the overhead of iterating through all
	 * devices when we need to remove a modem
	 */
	ofono->devices = g_slist_append (ofono->devices, device);

	urf_arbitrator_add_device (ofono->arbitrator, device);
}

static void
urf_ofono_manager_remove_modem (UrfOfonoManager *ofono,
                                const char *object_path)
{
	UrfDeviceOfono *device = NULL;
	GSList *dev = NULL;
	gchar *path;

	for (dev = ofono->devices; dev; dev = dev->next) {
		path = urf_device_ofono_get_path (dev->data);

		if (g_strcmp0 (path, object_path) == 0) {
			device = dev->data;
			break;
		}
	}

	if (device) {
		ofono->devices = g_slist_remove (ofono->devices, device);
		urf_arbitrator_remove_device (ofono->arbitrator, URF_DEVICE (device));
	}
}

static void
urf_ofono_manager_remove_all_modems (UrfOfonoManager *ofono)
{
	g_debug ("Remove all modems.");
}

static void
ofono_get_modems_cb (GObject *source_object,
                     GAsyncResult *res,
                     gpointer user_data)
{
	UrfOfonoManager *ofono = user_data;
	GVariant *value, *modems;
	GVariantIter iter, dict_iter;
	gchar *modem_path;
	GError *error = NULL;

	value = g_dbus_proxy_call_finish (ofono->proxy, res, &error);

	if (!error) {
		g_debug ("variant %p: %s", value, g_variant_get_type_string (value));

		modems = g_variant_get_child_value (value, 0);
		g_debug ("found %zd modems", g_variant_n_children (modems));

		g_variant_iter_init (&iter, modems);
		while (g_variant_iter_next (&iter, "(oa{sv})", &modem_path, &dict_iter)) {
			g_message ("Modem found: '%s'", modem_path);

			urf_ofono_manager_add_modem (ofono, modem_path);

			g_free (modem_path);
		}
	} else {
		g_warning ("Could not get list of modems.");
	}
}

static void
ofono_signal_cb (GDBusProxy *proxy,
                 gchar *sender_name,
                 gchar *signal_name,
                 GVariant *parameters,
                 gpointer user_data)
{
	UrfOfonoManager *ofono = user_data;
	const char *object_path;

	if (g_strcmp0 (signal_name, "ModemAdded") == 0) {
		g_variant_get (parameters, "(oa{sv})", &object_path, NULL);
		urf_ofono_manager_add_modem (ofono, object_path);
	} else if (g_strcmp0 (signal_name, "ModemRemoved") == 0) {
		g_variant_get (parameters, "(o)", &object_path);
		urf_ofono_manager_remove_modem (ofono, object_path);
	}
}

static void
ofono_proxy_ready_cb (GObject *source_object,
                      GAsyncResult *res,
                      gpointer user_data)
{
	UrfOfonoManager *ofono = user_data;
	GError *error = NULL;

	ofono->proxy = g_dbus_proxy_new_finish (res, &error);

	if (!error) {
		g_cancellable_reset (ofono->cancellable);
		g_dbus_proxy_call (ofono->proxy,
		                   "GetModems",
		                   NULL,
		                   G_DBUS_CALL_FLAGS_NONE,
		                   -1,
		                   ofono->cancellable,
		                   ofono_get_modems_cb,
		                   ofono);
		g_signal_connect (ofono->proxy, "g-signal",
		                  G_CALLBACK (ofono_signal_cb), ofono);
	} else {
		g_warning("Could not get oFono Modem proxy.");
	}
}

static void
on_ofono_appeared (GDBusConnection *connection,
                   const gchar *name,
                   const gchar *name_owner,
                   gpointer user_data)
{
	UrfOfonoManager *ofono = user_data;

	g_debug("oFono appeared on the bus");

	g_cancellable_reset (ofono->cancellable);
	g_dbus_proxy_new (connection,
	                  G_DBUS_PROXY_FLAGS_NONE,
	                  NULL,
	                  "org.ofono",
	                  "/",
	                  "org.ofono.Manager",
	                  ofono->cancellable,
	                  ofono_proxy_ready_cb,
	                  ofono);

}

static void
on_ofono_vanished (GDBusConnection *connection,
                   const gchar *name,
                   gpointer user_data)
{
	UrfOfonoManager *ofono = user_data;

	g_debug("oFono vanished from the bus");

	g_cancellable_cancel (ofono->cancellable);

	if (ofono->proxy) {
		g_object_unref (ofono->proxy);
		ofono->proxy = NULL;
	}

	if (ofono->devices) {
		modem_idx = 100;
		urf_ofono_manager_remove_all_modems (ofono);
	}
}

/**
 * urf_ofono_manager_class_init:
 **/
static void
urf_ofono_manager_class_init (GObjectClass *object_class)
{
	object_class->finalize = urf_ofono_manager_finalize;
}

gboolean
urf_ofono_manager_startup (UrfOfonoManager *ofono,
                           UrfArbitrator *arbitrator)
{
	ofono->arbitrator = g_object_ref (arbitrator);

	ofono->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
	                                    "org.ofono",
	                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
	                                    on_ofono_appeared,
	                                    on_ofono_vanished,
	                                    ofono,
	                                    NULL);

	return TRUE;
}

/**
 * urf_ofono_manager_init:
 **/
static void
urf_ofono_manager_init (UrfOfonoManager *ofono)
{
	ofono->arbitrator = NULL;
	ofono->cancellable = g_cancellable_new ();
	ofono->proxy = NULL;
	ofono->watch_id = 0;

	return;
}

/**
 * urf_ofono_manager_new:
 **/
UrfOfonoManager *
urf_ofono_manager_new (void)
{
	return g_object_new (URF_TYPE_OFONO_MANAGER, NULL);
}

