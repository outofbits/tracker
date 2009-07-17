/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008, Nokia (urho.konttori@nokia.com)
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

#ifndef __TRACKER_DB_MANAGER_H__
#define __TRACKER_DB_MANAGER_H__

#include <glib-object.h>

#include "tracker-db-interface.h"

G_BEGIN_DECLS

#define TRACKER_TYPE_DB (tracker_db_get_type ())

typedef enum {
	TRACKER_DB_UNKNOWN,
	TRACKER_DB_COMMON,
	TRACKER_DB_QUAD,
	TRACKER_DB_METADATA,
	TRACKER_DB_CONTENTS,
	TRACKER_DB_FULLTEXT,
} TrackerDB;

typedef enum {
	TRACKER_DB_MANAGER_FORCE_REINDEX    = 1 << 1,
	TRACKER_DB_MANAGER_REMOVE_CACHE     = 1 << 2,
	TRACKER_DB_MANAGER_LOW_MEMORY_MODE  = 1 << 3,
	TRACKER_DB_MANAGER_REMOVE_ALL       = 1 << 4,
	TRACKER_DB_MANAGER_READONLY        = 1 << 5,
	TRACKER_DB_MANAGER_TEST_MODE        = 1 << 6
} TrackerDBManagerFlags;

GType	     tracker_db_get_type			    (void) G_GNUC_CONST;

gboolean     tracker_db_manager_init			    (TrackerDBManagerFlags  flags,
							     gboolean		   *first_time,
							     gboolean 		    shared_cache);
void	     tracker_db_manager_shutdown		    (void);

void	     tracker_db_manager_remove_all		    (void);
void         tracker_db_manager_optimize		    (void);

const gchar *tracker_db_manager_get_file		    (TrackerDB		    db);
TrackerDBInterface *
	     tracker_db_manager_get_db_interface	    (void);
TrackerDBInterface *
	     tracker_db_manager_get_db_interfaces	    (gint num, ...);
TrackerDBInterface *
	     tracker_db_manager_get_db_interfaces_ro	    (gint num, ...);

G_END_DECLS

#endif /* __TRACKER_DB_MANAGER_H__ */
