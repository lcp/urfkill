/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Mathieu Trudel-Lapierre <mathieu-tl@ubuntu.com>
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

#include "urf-session-checker-none.h"

typedef struct {
	guint		 cookie;
	char		*session_id;
	char		*bus_name;
	char		*reason;
} UrfInhibitor;

struct UrfLogindPrivate {
	GDBusProxy	*proxy;
	GDBusProxy	*bus_proxy;
	GList		*seats;
	GList		*inhibitors;
	gboolean	 inhibit;
};

G_DEFINE_TYPE (UrfSessionChecker, urf_session_checker, G_TYPE_OBJECT)

#define URF_SESSION_CHECKER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_SESSION_CHECKER, UrfLogindPrivate))

/**
 * urf_session_checker_is_inhibited:
 **/
gboolean
urf_session_checker_is_inhibited (UrfSessionChecker *logind)
{
	return FALSE;
}

/**
 * urf_session_checker_inhibit:
 **/
guint
urf_session_checker_inhibit (UrfSessionChecker *logind,
                             const char *bus_name,
                             const char *reason)
{
	return 0;
}

/**
 * urf_session_checker_uninhibit:
 **/
void
urf_session_checker_uninhibit (UrfSessionChecker *logind,
                               const guint    cookie)
{
}

/**
 * urf_session_checker_add_seat:
 **/
static void
urf_session_checker_add_seat (UrfSessionChecker *logind,
                              const char *object_path)
{
}

/**
 * urf_session_checker_startup:
 **/
gboolean
urf_session_checker_startup (UrfSessionChecker *logind)
{
	return TRUE;
}

/**
 * urf_session_checker_new:
 **/
UrfSessionChecker *
urf_session_checker_new (void)
{
	UrfSessionChecker *logind;
	logind = URF_SESSION_CHECKER (g_object_new (URF_TYPE_SESSION_CHECKER, NULL));
	return logind;
}
