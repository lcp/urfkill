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

#ifndef __URF_DAEMON_H__
#define __URF_DAEMON_H__

#include <glib-object.h>
#include <polkit/polkit.h>
#include <dbus/dbus-glib.h>

#include "urf-config.h"

G_BEGIN_DECLS

#define URF_TYPE_DAEMON		(urf_daemon_get_type ())
#define URF_DAEMON(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_DAEMON, UrfDaemon))
#define URF_DAEMON_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_DAEMON, UrfDaemonClass))
#define URF_IS_DAEMON(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_DAEMON))
#define URF_IS_DAEMON_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_DAEMON))
#define URF_DAEMON_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_DAEMON, UrfDaemonClass))

typedef struct UrfDaemonPrivate UrfDaemonPrivate;

typedef struct
{
	GObject	parent;
	UrfDaemonPrivate	*priv;
} UrfDaemon;

typedef struct
{
	GObjectClass		 parent_class;
} UrfDaemonClass;

typedef enum
{
	URF_DAEMON_ERROR_GENERAL,
	URF_DAEMON_ERROR_NOT_SUPPORTED,
	URF_DAEMON_ERROR_NO_SUCH_DEVICE,
	URF_DAEMON_NUM_ERRORS
} UrfDaemonError;

#define URF_DAEMON_ERROR urf_daemon_error_quark ()

GType urf_daemon_error_get_type (void);
#define URF_DAEMON_TYPE_ERROR (urf_daemon_error_get_type ())

GQuark		 urf_daemon_error_quark		(void);
GType		 urf_daemon_get_type		(void);
UrfDaemon	*urf_daemon_new			(void);
//void		 urf_daemon_test		(gpointer  user_data);

gboolean	 urf_daemon_startup		(UrfDaemon		*daemon,
						 UrfConfig		*config);
gboolean	 urf_daemon_block		(UrfDaemon		*daemon,
						 const char		*type_name,
						 DBusGMethodInvocation  *context);
gboolean	 urf_daemon_block_idx		(UrfDaemon		*daemon,
						 const guint		 index,
						 DBusGMethodInvocation  *context);
gboolean	 urf_daemon_unblock		(UrfDaemon		*daemon,
						 const char 		*type_name,
						 DBusGMethodInvocation	*context);
gboolean	 urf_daemon_unblock_idx		(UrfDaemon		*daemon,
						 const guint 		 index,
						 DBusGMethodInvocation	*context);
gboolean	 urf_daemon_get_all		(UrfDaemon		*daemon,
						 DBusGMethodInvocation  *context);
gboolean	 urf_daemon_get_killswitch	(UrfDaemon		*daemon,
						 guint			 index,
						 DBusGMethodInvocation  *context);

G_END_DECLS

#endif /* __URF_DAEMON_H__ */
