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
 */

#ifndef __LIBTRACKER_COMMON_NAMESPACE_H__
#define __LIBTRACKER_COMMON_NAMESPACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#if !defined (__LIBTRACKER_COMMON_INSIDE__) && !defined (TRACKER_COMPILATION)
#error "only <libtracker-common/tracker-common.h> must be included directly."
#endif

#define TRACKER_TYPE_NAMESPACE	       (tracker_namespace_get_type ())
#define TRACKER_NAMESPACE(o)	       (G_TYPE_CHECK_INSTANCE_CAST ((o), TRACKER_TYPE_NAMESPACE, TrackerNamespace))
#define TRACKER_NAMESPACE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), TRACKER_TYPE_NAMESPACE, TrackerNamespaceClass))
#define TRACKER_IS_NAMESPACE(o)	       (G_TYPE_CHECK_INSTANCE_TYPE ((o), TRACKER_TYPE_NAMESPACE))
#define TRACKER_IS_NAMESPACE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TRACKER_TYPE_NAMESPACE))
#define TRACKER_NAMESPACE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TRACKER_TYPE_NAMESPACE, TrackerNamespaceClass))

typedef struct _TrackerNamespace TrackerNamespace;
typedef struct _TrackerNamespaceClass TrackerNamespaceClass;

struct _TrackerNamespace {
	GObject parent;
};

struct _TrackerNamespaceClass {
	GObjectClass parent_class;
};

GType		  tracker_namespace_get_type   (void) G_GNUC_CONST;
TrackerNamespace *tracker_namespace_new        (void);
const gchar *     tracker_namespace_get_uri    (TrackerNamespace *namespace_);
const gchar *     tracker_namespace_get_prefix (TrackerNamespace *namespace_);
void              tracker_namespace_set_uri    (TrackerNamespace *namespace_,
						const gchar      *value);
void              tracker_namespace_set_prefix (TrackerNamespace *namespace_,
						const gchar      *value);

G_END_DECLS

#endif /* __LIBTRACKER_COMMON_NAMESPACE_H__ */

