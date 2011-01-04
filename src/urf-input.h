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

#ifndef __URF_INPUT_H__
#define __URF_INPUT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define URF_TYPE_INPUT (urf_input_get_type())
#define URF_INPUT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_INPUT, UrfInput))
#define URF_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_INPUT, UrfInputClass))
#define URF_IS_INPUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_INPUT))
#define URF_IS_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_INPUT))
#define URF_GET_INPUT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_INPUT, UrfInputClass))

typedef struct UrfInputPrivate UrfInputPrivate;

typedef struct {
	GObject parent;
	UrfInputPrivate *priv;
} UrfInput;

typedef struct {
        GObjectClass parent_class;

        void (*rf_key_pressed)   (UrfInput *input, guint code);
} UrfInputClass;

GType		 urf_input_get_type 	(void);
UrfInput	*urf_input_new		(void);
gboolean	 urf_input_startup	(UrfInput *input);

G_END_DECLS

#endif /* __URF_INPUT_H__ */
