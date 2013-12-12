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

#ifndef __URF_SESSION_CHECKER_CONSOLEKIT_H__
#define __URF_SESSION_CHECKER_CONSOLEKIT_H__

#include <glib-object.h>

#include "urf-seat-consolekit.h"

G_BEGIN_DECLS

#define URF_TYPE_SESSION_CHECKER (urf_session_checker_get_type())
#define URF_SESSION_CHECKER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_SESSION_CHECKER, UrfSessionChecker))
#define URF_SESSION_CHECKER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_SESSION_CHECKER, UrfSessionCheckerClass))
#define URF_IS_SESSION_CHECKER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_SESSION_CHECKER))
#define URF_IS_SESSION_CHECKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_SESSION_CHECKER))
#define URF_GET_SESSION_CHECKER_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_SESSION_CHECKER, UrfSessionCheckerClass))

typedef struct UrfConsolekitPrivate UrfConsolekitPrivate;

typedef struct {
	GObject 		 parent;
	UrfConsolekitPrivate 	*priv;
} UrfSessionChecker;

typedef struct {
        GObjectClass	 	 parent_class;
} UrfSessionCheckerClass;

GType			 urf_session_checker_get_type		(void);

UrfSessionChecker	*urf_session_checker_new		(void);

gboolean		 urf_session_checker_startup		(UrfSessionChecker *consolekit);

gboolean		 urf_session_checker_is_inhibited	(UrfSessionChecker *consolekit);
guint			 urf_session_checker_inhibit		(UrfSessionChecker *consolekit,
								 const char	*bus_name,
								 const char	*reason);
void			 urf_session_checker_uninhibit		(UrfSessionChecker *consolekit,
								 const guint	 cookie);

G_END_DECLS

#endif /* __URF_SESSION_CHECKER_CONSOLEKIT_H__ */
