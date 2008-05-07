/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2008, Nokia
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __TRACKERD_DBUS_SEARCH_H__
#define __TRACKERD_DBUS_SEARCH_H__

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "tracker-db-sqlite.h"
#include "tracker-indexer.h"

#define TRACKER_DBUS_SEARCH_SERVICE         "org.freedesktop.Tracker"
#define TRACKER_DBUS_SEARCH_PATH            "/org/freedesktop/Tracker/Search"
#define TRACKER_DBUS_SEARCH_INTERFACE       "org.freedesktop.Tracker.Search"

G_BEGIN_DECLS

#define TRACKER_TYPE_DBUS_SEARCH            (tracker_dbus_search_get_type ())
#define TRACKER_DBUS_SEARCH(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), TRACKER_TYPE_DBUS_SEARCH, TrackerDBusSearch))
#define TRACKER_DBUS_SEARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TRACKER_TYPE_DBUS_SEARCH, TrackerDBusSearchClass))
#define TRACKER_IS_DBUS_SEARCH(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TRACKER_TYPE_DBUS_SEARCH))
#define TRACKER_IS_DBUS_SEARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TRACKER_TYPE_DBUS_SEARCH))
#define TRACKER_DBUS_SEARCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TRACKER_TYPE_DBUS_SEARCH, TrackerDBusSearchClass))

typedef struct TrackerDBusSearch      TrackerDBusSearch;
typedef struct TrackerDBusSearchClass TrackerDBusSearchClass;

struct TrackerDBusSearch {
	GObject parent;
};

struct TrackerDBusSearchClass {
	GObjectClass parent;
};

GType    tracker_dbus_search_get_type          (void);

TrackerDBusSearch *
         tracker_dbus_search_new               (DBConnection          *db_con);
void     tracker_dbus_search_set_db_connection (TrackerDBusSearch   *object,
						DBConnection        *db_con);
void     tracker_dbus_search_set_file_index    (TrackerDBusSearch   *object,
						Indexer             *file_index);
void     tracker_dbus_search_set_email_index   (TrackerDBusSearch   *object,
						Indexer             *email_index);
gboolean tracker_dbus_search_get_hit_count     (TrackerDBusSearch   *object,
						const gchar         *service,
						const gchar         *search_text,
						gint                *result,
						GError             **error);
gboolean tracker_dbus_search_get_hit_count_all (TrackerDBusSearch   *object,
						const gchar         *search_text,
						GPtrArray          **values,
						GError             **error);
gboolean tracker_dbus_search_text              (TrackerDBusSearch   *object,
						gint                 live_query_id,
						const gchar         *service,
						const gchar         *search_text,
						gint                 offset,
						gint                 max_hits,
						gchar             ***values,
						GError             **error);
gboolean tracker_dbus_search_text_detailed     (TrackerDBusSearch   *object,
						gint                 live_query_id,
						const gchar         *service,
						const gchar         *search_text,
						gint                 offset,
						gint                 max_hits,
						GPtrArray          **values,
						GError             **error);
gboolean tracker_dbus_search_get_snippet       (TrackerDBusSearch   *object,
						const gchar         *service,
						const gchar         *id,
						const gchar         *search_text,
						gchar              **result,
						GError             **error);
gboolean tracker_dbus_search_files_by_text     (TrackerDBusSearch   *object,
						gint                 live_query_id,
						const gchar         *search_text,
						gint                 offset,
						gint                 max_hits,
						gboolean             group_results,
						GHashTable         **values,
						GError             **error);
gboolean tracker_dbus_search_metadata          (TrackerDBusSearch   *object,
						const gchar         *service,
						const gchar         *field,
						const gchar         *search_text,
						gint                 offset,
						gint                 max_hits,
						gchar             ***values,
						GError             **error);
gboolean tracker_dbus_search_matching_fields   (TrackerDBusSearch   *object,
						const gchar         *service,
						const gchar         *id,
						const gchar         *search_text,
						GHashTable         **values,
						GError             **error);
gboolean tracker_dbus_search_query             (TrackerDBusSearch   *object,
						gint                 live_query_id,
						const gchar         *service,
						gchar              **fields,
						const gchar         *search_text,
						const gchar         *keyword,
						const gchar         *query_condition,
						gboolean             sort_by_service,
						gint                 offset,
						gint                 max_hits,
						GPtrArray          **values,
						GError             **error);
gboolean tracker_dbus_search_suggest           (TrackerDBusSearch   *object,
						const gchar         *search_text,
						gint                 max_dist,
						gchar              **value,
						GError             **error);

G_END_DECLS

#endif /* __TRACKERD_DBUS_SEARCH_H__ */
