/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@suse.com>
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
#include <gio/gio.h>

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
} UrfDaemonError;

#define URF_DAEMON_ERROR urf_daemon_error_quark ()

GQuark		 urf_daemon_error_quark		(void);
GType		 urf_daemon_get_type		(void);
UrfDaemon	*urf_daemon_new			(UrfConfig		*config);

gboolean	 urf_daemon_startup		(UrfDaemon		*daemon);
gboolean	 urf_daemon_block		(UrfDaemon		*daemon,
						 const gint		 type,
						 const gboolean		 block,
						 GDBusMethodInvocation  *invocation);
gboolean	 urf_daemon_block_idx		(UrfDaemon		*daemon,
						 const gint		 index,
						 const gboolean		 block,
						 GDBusMethodInvocation  *invocation);
gboolean	 urf_daemon_enumerate_devices	(UrfDaemon		*daemon,
						 GDBusMethodInvocation  *invocation);
gboolean	 urf_daemon_is_flight_mode	(UrfDaemon		*daemon,
						 GDBusMethodInvocation  *invocation);
gboolean	 urf_daemon_flight_mode	        (UrfDaemon		*daemon,
						 const gboolean		 block,
						 GDBusMethodInvocation  *invocation);
gboolean	 urf_daemon_is_inhibited	(UrfDaemon		*daemon,
						 GDBusMethodInvocation  *invocation);
gboolean	 urf_daemon_inhibit		(UrfDaemon		*daemon,
						 const char		*reason,
						 GDBusMethodInvocation  *invocation);
void		 urf_daemon_uninhibit		(UrfDaemon		*daemon,
						 const guint		 cookie,
						 GDBusMethodInvocation  *invocation);

G_END_DECLS

#endif /* __URF_DAEMON_H__ */
