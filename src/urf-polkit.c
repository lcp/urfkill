/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@suse.com>
 * Copyright (C) 2008 David Zeuthen <davidz@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#include <polkit/polkit.h>

#include "urf-polkit.h"
#include "urf-daemon.h"

#define URF_POLKIT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URF_TYPE_POLKIT, UrfPolkitPrivate))

struct UrfPolkitPrivate
{
	PolkitAuthority	*authority;
};

G_DEFINE_TYPE (UrfPolkit, urf_polkit, G_TYPE_OBJECT)
static gpointer urf_polkit_object = NULL;

/**
 * urf_polkit_get_subject:
 **/
PolkitSubject *
urf_polkit_get_subject (UrfPolkit             *polkit,
                        GDBusMethodInvocation *invocation)
{
	const gchar *sender;
	PolkitSubject *subject;

	sender = g_dbus_method_invocation_get_sender (invocation);
	subject = polkit_system_bus_name_new (sender);

	if (subject == NULL) {
		g_dbus_method_invocation_return_error (invocation,
		                                       URF_DAEMON_ERROR,
		                                       URF_DAEMON_ERROR_GENERAL,
		                                       "failed to get PolicyKit subject");
	}

	return subject;
}

/**
 * urf_polkit_check_auth:
 **/
gboolean
urf_polkit_check_auth (UrfPolkit             *polkit,
		       PolkitSubject         *subject,
		       const gchar           *action_id,
		       GDBusMethodInvocation *invocation)
{
	gboolean ret = FALSE;
	GError *error = NULL;
	PolkitAuthorizationResult *result;

	/* check auth */
	result = polkit_authority_check_authorization_sync (polkit->priv->authority,
							    subject, action_id, NULL,
							    POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
							    NULL, &error);
	if (result == NULL) {
		g_dbus_method_invocation_return_error (invocation,
		                                       URF_DAEMON_ERROR,
		                                       URF_DAEMON_ERROR_GENERAL,
		                                       "failed to check authorisation: %s",
		                                       error->message);
		g_error_free (error);
		goto out;
	}

	/* okay? */
	if (polkit_authorization_result_get_is_authorized (result)) {
		ret = TRUE;
	} else {
		g_dbus_method_invocation_return_error (invocation,
		                                       URF_DAEMON_ERROR,
		                                       URF_DAEMON_ERROR_GENERAL,
		                                       "not authorized");
	}
out:
	if (result != NULL)
		g_object_unref (result);
	return ret;
}

/**
 * urf_polkit_finalize:
 **/
static void
urf_polkit_finalize (GObject *object)
{
	UrfPolkit *polkit;
	g_return_if_fail (URF_IS_POLKIT (object));
	polkit = URF_POLKIT (object);

	g_object_unref (polkit->priv->authority);

	G_OBJECT_CLASS (urf_polkit_parent_class)->finalize (object);
}

/**
 * urf_polkit_class_init:
 **/
static void
urf_polkit_class_init (UrfPolkitClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = urf_polkit_finalize;
	g_type_class_add_private (klass, sizeof (UrfPolkitPrivate));
}

/**
 * urf_polkit_init:
 *
 * initializes the polkit class. NOTE: We expect polkit objects
 * to *NOT* be removed or added during the session.
 * We only control the first polkit object if there are more than one.
 **/
static void
urf_polkit_init (UrfPolkit *polkit)
{
	GError *error = NULL;

	polkit->priv = URF_POLKIT_GET_PRIVATE (polkit);

#ifdef USE_SECURITY_POLKIT_NEW
	polkit->priv->authority = polkit_authority_get_sync (NULL, &error);
	if (polkit->priv->authority == NULL) {
		g_error ("failed to get pokit authority: %s", error->message);
		g_error_free (error);
		goto out;
	}
#else
	polkit->priv->authority = polkit_authority_get ();
#endif
out:
	return;
}

/**
 * urf_polkit_new:
 * Return value: A new polkit class instance.
 **/
UrfPolkit *
urf_polkit_new (void)
{
	if (urf_polkit_object != NULL) {
		g_object_ref (urf_polkit_object);
	} else {
		urf_polkit_object = g_object_new (URF_TYPE_POLKIT, NULL);
		g_object_add_weak_pointer (urf_polkit_object, &urf_polkit_object);
	}
	return URF_POLKIT (urf_polkit_object);
}

