/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2009, Nokia (urho.konttori@nokia.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Authors: Martyn Russell <martyn@imendio.com>
 */

#ifndef __TRACKER_APPLET_H__
#define __TRACKER_APPLET_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct {
	GtkBuilder *builder;

	GtkWidget *results;
	GtkWidget *parent;

	GtkWidget *box;
	GtkWidget *event_box;
	GtkWidget *image;
	GtkWidget *entry;

	guint idle_draw_id;

	GtkOrientation orient;
	GdkPixbuf *icon;
	guint size;
} TrackerApplet;

G_END_DECLS

#endif /* __TRACKER_APPLET_H__ */
