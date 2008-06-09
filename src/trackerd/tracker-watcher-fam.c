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

#include <string.h>
#include <unistd.h>
#include <fam.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "tracker-watcher.h"

typedef struct {
	gchar 	   *watch_directory;
	FAMRequest *fr;
	WatchTypes  watch_type;
} DirWatch;

extern Tracker 	     *tracker;
extern DBConnection  *main_thread_db_con;

static gint	      fam_watch_id;
static FAMConnection  fc;

static GHashTable    *watch_table;
static gint           monitor_limit = 300;

static WatchTypes
get_directory_watch_type (const gchar *dir)
{
	DirWatch *watch;

	watch = g_hash_table_lookup (watch_table, dir);

	if (watch != NULL) {
		return watch->watch_type;
	}

	return WATCH_OTHER;
}

static void
watch_table_value_destroy_func (gpointer user_data)
{
	DirWatch *fw;

	fw = user_data;

        g_free (fw->fr);
        g_free (fw->watch_directory);
        g_free (fw);
}

static gboolean
is_delete_event (TrackerAction event_type)
{
	return 
                event_type == TRACKER_ACTION_DELETE ||
		event_type == TRACKER_ACTION_DELETE_SELF ||
		event_type == TRACKER_ACTION_FILE_DELETED ||
		event_type == TRACKER_ACTION_DIRECTORY_DELETED;
}

static gboolean
fam_callback (GIOChannel   *source,
	      GIOCondition  condition,
	      gpointer      data)
{
	gint counter;

	counter = 1;

	while (FAMPending (&fc)) {
		FAMEvent       ev;
		DirWatch      *watch;
		TrackerAction  event_type;

		if (FAMNextEvent (&fc, &ev) != 1) {
			tracker_end_watching ();
			return FALSE;
		}

		watch = (DirWatch *) ev.userdata;
		event_type = TRACKER_ACTION_IGNORE;

		switch (ev.code) {
                case FAMChanged:
                        event_type = TRACKER_ACTION_CHECK;
                        counter = 1;
                        break;
                case FAMDeleted:
                        event_type = TRACKER_ACTION_DELETE;
                        counter = 0;
                        break;
                case FAMCreated:
                        event_type = TRACKER_ACTION_CREATE;
                        counter = 1;
                        break;
                        
                case FAMStartExecuting:
                case FAMStopExecuting:
                case FAMAcknowledge:
                case FAMExists:
                case FAMEndExist:
                case FAMMoved:
                        continue;
		}
                
		if (event_type != TRACKER_ACTION_IGNORE) {
			gchar	 *file_uri, *file_utf8_uri;
			FileInfo *info;
                        
			if (tracker_is_empty_string (ev.filename)) {
				continue;
			}
                        
			if (ev.filename[0] == G_DIR_SEPARATOR) {
				file_uri = g_strdup (ev.filename);
			} else {
				gchar *s;

				s = g_filename_to_utf8 (ev.filename, -1, NULL, NULL, NULL);

				if (tracker_ignore_file (s)) {
					g_free (s);
					continue;
				} else {
					file_uri = g_build_filename (watch->watch_directory, 
                                                                     ev.filename, NULL);
				}

				g_free (s);
			}

			file_utf8_uri = g_filename_to_utf8 (file_uri, -1, NULL, NULL, NULL);
			g_free (file_uri);

			if (!file_utf8_uri) {
                                g_critical ("File uri:'%s' could not be converted to utf8 format",
                                            filename);
				continue;
			}

                        if (file_utf8_uri[0] == G_DIR_SEPARATOR ||
                            tracker_process_files_should_be_ignored (tracker->config, file_utf8_uri) ||  
                            tracker_process_files_should_be_crawled (tracker, file_utf8_uri) || 
                            tracker_process_files_should_be_watched (tracker->config, file_utf8_uri)) {
				g_free (file_utf8_uri);
				continue;
			}

			info = tracker_create_file_info (file_utf8_uri, 
                                                         event_type, 
                                                         counter, 
                                                         WATCH_OTHER);

			if (tracker_file_info_is_valid (info)) {
				/* Process deletions immediately, schedule all others */
				if (is_delete_event (event_type)) {
					gchar *uri_in_locale, *parent;

					uri_in_locale = g_filename_from_utf8 (info->uri, -1, NULL, NULL, NULL);

					if (!uri_in_locale) {
						tracker_error ("ERROR: FAM uri could not be converted to locale format");
						return FALSE;
					}

					parent = g_path_get_dirname (uri_in_locale);
					g_free (uri_in_locale);

					if (tracker_is_directory_watched (parent, NULL) || 
                                            tracker_is_directory_watched (info->uri, NULL)) {
						g_async_queue_push (tracker->file_process_queue, info);
						tracker_notify_file_data_available ();
					} else {
						tracker_free_file_info (info);
					}

					g_free (parent);
				} else {
					
					if (event_type == TRACKER_ACTION_CREATE) {
						tracker_db_insert_pending_file (main_thread_db_con,
                                                                                info->file_id, 
                                                                                info->uri, NULL, 
                                                                                info->mime, 
                                                                                info->counter, 
                                                                                info->action, 
                                                                                info->is_directory, 
                                                                                TRUE,
                                                                                -1);
					} else {
						tracker_db_insert_pending_file (main_thread_db_con, 
                                                                                info->file_id, 
                                                                                info->uri, 
                                                                                NULL, 
                                                                                info->mime, 
                                                                                info->counter, 
                                                                                info->action, 
                                                                                info->is_directory, 
                                                                                FALSE, 
                                                                                -1);
					}

					tracker_free_file_info (info);
				}
			}

			g_free (file_utf8_uri);
		}
	}

	return TRUE;
}

gboolean
tracker_watcher_init (void)
{
	GIOChannel *channel;

        if (fam_watch_id != 0) {
                return TRUE;
        }

	if (FAMOpen2 (&fc, "Tracker") != 0) {
		return FALSE;
	}

        g_message ("Using inotify monitor limit of %d", monitor_limit);

	watch_table = g_hash_table_new_full (g_str_hash, 
                                             g_str_equal, 
                                             g_free, 
                                             watch_table_value_destroy_func);

	channel = g_io_channel_unix_new (FAMCONNECTION_GETFD (&fc));

	fam_watch_id = g_io_add_watch (channel,
				       G_IO_IN | G_IO_HUP | G_IO_ERR,
				       fam_callback,
                                       &fc);

        /* Should we be unrefing here? */
	g_io_channel_unref (channel);

	return TRUE;
}

void
tracker_watcher_shutdown (void)
{
	FAMClose (&fc);

	if (fam_watch_id > 0) {
		g_source_remove (fam_watch_id);
		fam_watch_id = 0;
	}

        if (watch_table) {
                g_hash_table_unref (watch_table);
                watch_table = NULL;
        }
}

gboolean
tracker_watcher_add_dir (const gchar  *dir, 
                         DBConnection *db_con)
{
	gchar *dir_in_locale;
        
        g_return_val_if_fail (dir != NULL, FALSE);
        g_return_val_if_fail (db_con != NULL, FALSE);

	if (!tracker_file_is_valid (dir)) {
		return FALSE;
	}

	if (!tracker_is_directory (dir)) {
		return FALSE;
	}

	if (g_hash_table_size (watch_table) >= monitor_limit) {
                g_warning ("The directory watch limit (%d) has been reached, "
                           "you should increase the number of inotify watches on your system",
                           monitor_limit);
		return FALSE;
	}

	dir_in_locale = g_filename_from_utf8 (dir, -1, NULL, NULL, NULL);

	if (!dir_in_locale) {
                g_critical ("File uri:'%s' could not be converted to utf8 format",
                            filename);
		return FALSE;
	}

	/* check directory permissions are okay */
	if (g_access (dir_in_locale, F_OK) == 0 &&
            g_access (dir_in_locale, R_OK) == 0) {
		DirWatch   *fwatch;
		FAMRequest *fr;
		gint        rc;

		fwatch = g_new (DirWatch, 1);
		fr = g_new (FAMRequest, 1);
		fwatch->watch_directory = g_strdup (dir);
		fwatch->fr = fr;
		rc = -1;
		rc = FAMMonitorDirectory (&fc, dir_in_locale, fr, fwatch);

	 	if (rc > 0) {
			g_critical ("Could not watch directory:'%s', FAMMonitorDirectory() failed",
                                    dir);

			g_free (dir_in_locale);
			free_watch_func (fwatch);

			return FALSE;
		} else {
			g_hash_table_insert (watch_table, g_strdup (dir), fwatch);
			g_free (dir_in_locale);

                        g_message ("Watching directory:'%s' (total = %d)", 
                                   dir, g_hash_table_size (watch_table));

			return TRUE;
		}
	}

	g_free (dir_in_locale);

	return FALSE;
}

static gboolean
watcher_remove_dir_foreach (gpointer key,
                            gpointer value,
                            gpointer user_data)
{
	DirWatch    *fwatch;
	const gchar *dir;

	dir = user_data;
	fwatch = value;

	if (fwatch && dir) {
		gchar *dir_part;

		dir_part = g_strconcat (dir, G_DIR_SEPARATOR_S, NULL);

		/* Need to delete subfolders of dir as well */
		if (strcmp (fwatch->watch_directory, dir) == 0 || 
                    g_str_has_prefix (fwatch->watch_directory, dir_part)) {
			FAMCancelMonitor (&fc, fwatch->fr);
			g_free (dir_part);

			return TRUE;
		}

		g_free (dir_part);
	}

	return FALSE;
}

void
tracker_watcher_remove_dir (const gchar  *dir, 
                            gboolean      delete_subdirs, 
                            DBConnection *db_con)
{
	gchar *str;

	str = g_strdup (dir);

	if (delete_subdirs) {
		g_hash_table_foreach_remove (watch_table, 
                                             watcher_remove_dir_foreach, 
                                             str);
	} else {
		DirWatch *fwatch;

		fwatch = g_hash_table_lookup (watch_table, dir);
		FAMCancelMonitor (&fc, fwatch->fr);
		g_hash_table_remove (watch_table, dir);
	}

	g_free (str);
}

gboolean
tracker_watcher_is_dir_watched (const gchar  *dir,
                                DBConnection *db_con)
{
        g_return_val_if_fail (dir != NULL, FALSE);
        g_return_val_if_fail (db_con != NULL, FALSE);

	return g_hash_table_lookup (watch_table, dir) != NULL;
}

gint
tracker_watcher_get_dir_count (void)
{
	return g_hash_table_size (watch_table);
}
