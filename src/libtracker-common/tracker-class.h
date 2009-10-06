/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2008, Nokia (urho.konttori@nokia.com)
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
 */

#ifndef __LIBTRACKER_COMMON_CLASS_H__
#define __LIBTRACKER_COMMON_CLASS_H__

#include <glib-object.h>

G_BEGIN_DECLS

#if !defined (__LIBTRACKER_COMMON_INSIDE__) && !defined (TRACKER_COMPILATION)
#error "only <libtracker-common/tracker-common.h> must be included directly."
#endif

#define TRACKER_TYPE_CLASS	   (tracker_class_get_type ())
#define TRACKER_CLASS(o)	   (G_TYPE_CHECK_INSTANCE_CAST ((o), TRACKER_TYPE_CLASS, TrackerClass))
#define TRACKER_CLASS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), TRACKER_TYPE_CLASS, TrackerClassClass))
#define TRACKER_IS_CLASS(o)	   (G_TYPE_CHECK_INSTANCE_TYPE ((o), TRACKER_TYPE_CLASS))
#define TRACKER_IS_CLASS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TRACKER_TYPE_CLASS))
#define TRACKER_CLASS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TRACKER_TYPE_CLASS, TrackerClassClass))

typedef struct _TrackerClass TrackerClass;
typedef struct _TrackerClassClass TrackerClassClass;

struct _TrackerClass {
	GObject parent;
};

struct _TrackerClassClass {
	GObjectClass parent_class;
};

GType		tracker_class_get_type		(void) G_GNUC_CONST;
TrackerClass *  tracker_class_new               (void);
const gchar *   tracker_class_get_uri           (TrackerClass  *service);
const gchar *   tracker_class_get_name          (TrackerClass  *service);
gint            tracker_class_get_count         (TrackerClass  *service);
TrackerClass  **tracker_class_get_super_classes (TrackerClass  *service);
void            tracker_class_set_uri           (TrackerClass  *service,
						 const gchar   *value);
void            tracker_class_set_count         (TrackerClass  *service,
						 gint           value);
void            tracker_class_set_super_classes (TrackerClass  *self,
						 TrackerClass **super_classes);
void            tracker_class_add_super_class   (TrackerClass  *self,
						 TrackerClass  *value);

G_END_DECLS

#endif /* __LIBTRACKER_COMMON_CLASS_H__ */

