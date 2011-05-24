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

#ifndef __URF_CONSOLEKIT_H__
#define __URF_CONSOLEKIT_H__

#include <glib-object.h>

#include "urf-seat.h"

G_BEGIN_DECLS

#define URF_TYPE_CONSOLEKIT (urf_consolekit_get_type())
#define URF_CONSOLEKIT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_CONSOLEKIT, UrfConsolekit))
#define URF_CONSOLEKIT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_CONSOLEKIT, UrfConsolekitClass))
#define URF_IS_CONSOLEKIT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_CONSOLEKIT))
#define URF_IS_CONSOLEKIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_CONSOLEKIT))
#define URF_GET_CONSOLEKIT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_CONSOLEKIT, UrfConsolekitClass))

typedef struct UrfConsolekitPrivate UrfConsolekitPrivate;

typedef struct {
	GObject 		 parent;
	UrfConsolekitPrivate 	*priv;
} UrfConsolekit;

typedef struct {
        GObjectClass	 	 parent_class;
} UrfConsolekitClass;

GType			 urf_consolekit_get_type	(void);

UrfConsolekit		*urf_consolekit_new		(void);

gboolean		 urf_consolekit_startup		(UrfConsolekit	*consolekit);

gboolean		 urf_consolekit_is_inhibited	(UrfConsolekit	*consolekit);
guint			 urf_consolekit_inhibit		(UrfConsolekit	*consolekit,
							 const char	*bus_name,
							 const char	*reason);
void			 urf_consolekit_uninhibit	(UrfConsolekit	*consolekit,
							 const guint	 cookie);

G_END_DECLS

#endif /* __URF_CONSOLEKIT_H__ */
