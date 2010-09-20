/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@novell.com>
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

#ifndef __URFKILL_DAEMON_H__
#define __URFKILL_DAEMON_H__

#include <glib-object.h>
#include <polkit/polkit.h>
#include <dbus/dbus-glib.h>

#include "urfkill-types.h"

G_BEGIN_DECLS

#define URFKILL_TYPE_DAEMON		(urfkill_daemon_get_type ())
#define URFKILL_DAEMON(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), URFKILL_TYPE_DAEMON, UrfkillDaemon))
#define URFKILL_DAEMON_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), URFKILL_TYPE_DAEMON, UrfkillDaemonClass))
#define URFKILL_IS_DAEMON(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), URFKILL_TYPE_DAEMON))
#define URFKILL_IS_DAEMON_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), URFKILL_TYPE_DAEMON))
#define URFKILL_DAEMON_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), URFKILL_TYPE_DAEMON, UrfkillDaemonClass))

typedef struct UrfkillDaemonPrivate UrfkillDaemonPrivate;

typedef struct
{
	GObject	parent;
	UrfkillDaemonPrivate	*priv;
} UrfkillDaemon;

typedef struct
{
	GObjectClass		 parent_class;
} UrfkillDaemonClass;

typedef enum
{
	URFKILL_DAEMON_ERROR_GENERAL,
	URFKILL_DAEMON_ERROR_NOT_SUPPORTED,
	URFKILL_DAEMON_ERROR_NO_SUCH_DEVICE,
	URFKILL_DAEMON_NUM_ERRORS
} UrfkillDaemonError;

#define URFKILL_DAEMON_ERROR urfkill_daemon_error_quark ()

GType urfkill_daemon_error_get_type (void);
#define URFKILL_DAEMON_TYPE_ERROR (urfkill_daemon_error_get_type ())

//GQuark		 urfkill_daemon_error_quark		(void);
//GType		 urfkill_daemon_get_type		(void);
UrfkillDaemon	*urfkill_daemon_new			(void);
//void		 urfkill_daemon_test			(gpointer	 user_data);

gboolean	 urfkill_daemon_startup			(UrfkillDaemon	*daemon);

G_END_DECLS

#endif /* __URFKILL_DAEMON_H__ */
