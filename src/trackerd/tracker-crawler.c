/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include <libtracker-common/tracker-dbus.h>
#include <libtracker-common/tracker-file-utils.h>
#include <libtracker-common/tracker-utils.h>
#include <libtracker-common/tracker-module-config.h>

#include "tracker-crawler.h"
#include "tracker-dbus.h"
#include "tracker-indexer-client.h"
#include "tracker-monitor.h"
#include "tracker-marshal.h"

#define TRACKER_CRAWLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TRACKER_TYPE_CRAWLER, TrackerCrawlerPrivate))

#define FILE_ATTRIBUTES				\
	G_FILE_ATTRIBUTE_STANDARD_NAME ","	\
	G_FILE_ATTRIBUTE_STANDARD_TYPE

#define FILES_QUEUE_PROCESS_INTERVAL 2000
#define FILES_QUEUE_PROCESS_MAX      5000

struct _TrackerCrawlerPrivate {
	TrackerConfig  *config;
	TrackerHal     *hal;

	gchar          *module_name;

	/* Found data */
	GQueue         *directories;
	GQueue         *files;

	/* Idle handler for processing found data */
	guint           idle_id;

	/* Actual paths that exist which we are crawling */
	GSList         *paths;
	GSList         *current_path;
	gboolean        handled_paths;

	GSList         *recurse_paths;
	GSList         *current_recurse_path;
	gboolean        handled_recurse_paths;

	/* Specific to each crawl ... */
	GList          *ignored_directory_patterns;
	GList          *ignored_file_patterns;
	GList          *index_file_patterns;

	/* Statistics */
	GTimer         *timer;
	guint           enumerations;
	guint           directories_found;
	guint           directories_ignored;
	guint           files_found;
	guint           files_ignored;

	/* Status */
	gboolean        running;
	gboolean        finished;
};

enum {
	PROCESSING_DIRECTORY,
	FINISHED,
	LAST_SIGNAL
};

typedef struct {
	TrackerCrawler *crawler;
	GFile          *parent;
} EnumeratorData;

static void crawler_finalize        (GObject         *object);
static void file_enumerate_next     (GFileEnumerator *enumerator,
				     EnumeratorData  *ed);
static void file_enumerate_children (TrackerCrawler  *crawler,
				     GFile           *file);

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(TrackerCrawler, tracker_crawler, G_TYPE_OBJECT)

static void
tracker_crawler_class_init (TrackerCrawlerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = crawler_finalize;

	signals[PROCESSING_DIRECTORY] =
		g_signal_new ("processing-directory",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      tracker_marshal_VOID__STRING_OBJECT,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_STRING,
			      G_TYPE_OBJECT);
	signals[FINISHED] =
		g_signal_new ("finished",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      tracker_marshal_VOID__STRING_POINTER_UINT_UINT_UINT_UINT,
			      G_TYPE_NONE,
			      6,
			      G_TYPE_STRING,
			      G_TYPE_POINTER,
			      G_TYPE_UINT,
			      G_TYPE_UINT,
			      G_TYPE_UINT,
			      G_TYPE_UINT);

	g_type_class_add_private (object_class, sizeof (TrackerCrawlerPrivate));
}

static void
tracker_crawler_init (TrackerCrawler *object)
{
	TrackerCrawlerPrivate *priv;

	object->private = TRACKER_CRAWLER_GET_PRIVATE (object);

	priv = object->private;

	priv->directories = g_queue_new ();
	priv->files = g_queue_new ();
}

static void
crawler_finalize (GObject *object)
{
	TrackerCrawlerPrivate *priv;

	priv = TRACKER_CRAWLER_GET_PRIVATE (object);

	if (priv->timer) {
		g_timer_destroy (priv->timer);
	}

	if (priv->index_file_patterns) {
		g_list_free (priv->index_file_patterns);
	}

	if (priv->ignored_file_patterns) {
		g_list_free (priv->ignored_file_patterns);
	}

	if (priv->ignored_directory_patterns) {
		g_list_free (priv->ignored_directory_patterns);
	}

	/* Don't free the 'current_' variant of these, they are just
	 * place holders so we know our status.
	 */
	g_slist_foreach (priv->paths, (GFunc) g_free, NULL);
	g_slist_free (priv->paths);

	g_slist_foreach (priv->recurse_paths, (GFunc) g_free, NULL);
	g_slist_free (priv->recurse_paths);

	if (priv->idle_id) {
		g_source_remove (priv->idle_id);
	}

	g_queue_foreach (priv->files, (GFunc) g_object_unref, NULL);
	g_queue_free (priv->files);

	g_queue_foreach (priv->directories, (GFunc) g_object_unref, NULL);
	g_queue_free (priv->directories);

	g_free (priv->module_name);

	g_object_unref (priv->config);

	if (priv->hal) {
		g_object_unref (priv->hal);
	}

	G_OBJECT_CLASS (tracker_crawler_parent_class)->finalize (object);
}

TrackerCrawler *
tracker_crawler_new (TrackerConfig *config,
		     TrackerHal    *hal,
		     const gchar   *module_name)
{
	TrackerCrawler *crawler;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), NULL);
	g_return_val_if_fail (module_name != NULL, NULL);

#ifdef HAVE_HAL
	g_return_val_if_fail (TRACKER_IS_HAL (hal), NULL);
#endif /* HAVE_HAL */

	crawler = g_object_new (TRACKER_TYPE_CRAWLER, NULL);

	crawler->private->config = g_object_ref (config);

#ifdef HAVE_HAL
	crawler->private->hal = g_object_ref (hal);
#endif /* HAVE_HAL */

	crawler->private->module_name = g_strdup (module_name);

	/* Set up crawl data */
	crawler->private->ignored_directory_patterns =
		tracker_module_config_get_ignored_directory_patterns (module_name);
	crawler->private->ignored_file_patterns =
		tracker_module_config_get_ignored_file_patterns (module_name);
	crawler->private->index_file_patterns =
		tracker_module_config_get_index_file_patterns (module_name);

	return crawler;
}

/*
 * Functions
 */

static void
get_remote_roots (TrackerCrawler  *crawler,
		  GSList         **mounted_directory_roots,
		  GSList         **removable_device_roots)
{
        GSList *l1 = NULL;
        GSList *l2 = NULL;

	/* FIXME: Shouldn't we keep this static for a period of time
	 * so we make this process faster?
	 */

#ifdef HAVE_HAL
        l1 = tracker_hal_get_mounted_directory_roots (crawler->private->hal);
        l2 = tracker_hal_get_removable_device_roots (crawler->private->hal);
#endif /* HAVE_HAL */

        /* The options to index removable media and the index mounted
         * directories are both mutually exclusive even though
         * removable media is mounted on a directory.
         *
         * Since we get ALL mounted directories from HAL, we need to
         * remove those which are removable device roots.
         */
        if (l2) {
                GSList *l;
                GSList *list = NULL;

                for (l = l1; l; l = l->next) {
                        if (g_slist_find_custom (l2, l->data, (GCompareFunc) strcmp)) {
                                continue;
                        }

                        list = g_slist_prepend (list, l->data);
                }

                *mounted_directory_roots = g_slist_reverse (list);
        } else {
                *mounted_directory_roots = NULL;
        }

        *removable_device_roots = g_slist_copy (l2);
}

static gboolean
path_should_be_ignored_for_media (TrackerCrawler *crawler,
				  const gchar    *path)
{
        GSList   *roots = NULL;
        GSList   *mounted_directory_roots = NULL;
        GSList   *removable_device_roots = NULL;
	GSList   *l;
        gboolean  ignore_mounted_directories;
        gboolean  ignore_removable_devices;
        gboolean  ignore = FALSE;

        ignore_mounted_directories =
		!tracker_config_get_index_mounted_directories (crawler->private->config);
        ignore_removable_devices =
		!tracker_config_get_index_removable_devices (crawler->private->config);

        if (ignore_mounted_directories || ignore_removable_devices) {
                get_remote_roots (crawler,
				  &mounted_directory_roots,
				  &removable_device_roots);
        }

        if (ignore_mounted_directories) {
                roots = g_slist_concat (roots, mounted_directory_roots);
        }

        if (ignore_removable_devices) {
                roots = g_slist_concat (roots, removable_device_roots);
        }

	for (l = roots; l && !ignore; l = l->next) {
		/* If path matches a mounted or removable device by
		 * prefix then we should ignore it since we don't
		 * crawl those by choice in the config.
		 */
		if (strcmp (path, l->data) == 0) {
			ignore = TRUE;
		}

		/* FIXME: Should we add a DIR_SEPARATOR on the end of
		 * these before comparing them?
		 */
		if (g_str_has_prefix (path, l->data)) {
			ignore = TRUE;
		}
	}

        g_slist_free (roots);

	return ignore;
}

static gboolean
path_should_be_ignored (TrackerCrawler *crawler,
			const gchar    *path,
			gboolean        is_directory)
{
	GList    *l;
	gchar    *basename;
        gboolean  ignore;

	if (tracker_is_empty_string (path)) {
		return TRUE;
	}

	if (!g_utf8_validate (path, -1, NULL)) {
		g_message ("Ignoring path:'%s', not valid UTF-8", path);
		return TRUE;
	}

	/* Most common things to ignore */
	if (strcmp (path, "/boot") == 0 ||
	    strcmp (path, "/dev") == 0 ||
	    strcmp (path, "/lib") == 0 ||
	    strcmp (path, "/proc") == 0 ||
	    strcmp (path, "/sys") == 0 ||
	    strcmp (path, "/tmp") == 0 ||
	    strcmp (path, "/var") == 0) {
		return TRUE;
	}

	if (g_str_has_prefix (path, g_get_tmp_dir ())) {
		return TRUE;
	}

	basename = g_path_get_basename (path);
	ignore = TRUE;

	if (!basename || basename[0] == '.') {
		goto done;
	}

	/* Test ignore types */
	if (is_directory) {
		for (l = crawler->private->ignored_directory_patterns; l; l = l->next) {
			if (g_pattern_match_string (l->data, basename)) {
				goto done;
			}
		}
	} else {
		for (l = crawler->private->ignored_file_patterns; l; l = l->next) {
			if (g_pattern_match_string (l->data, basename)) {
				goto done;
			}
		}

		for (l = crawler->private->index_file_patterns; l; l = l->next) {
			if (!g_pattern_match_string (l->data, basename)) {
				goto done;
			}
		}
	}

	/* Should we crawl mounted or removable media */
	if (path_should_be_ignored_for_media (crawler, path)) {
		goto done;
	}

        ignore = FALSE;

done:
	g_free (basename);

	return ignore;
}

static void
add_file (TrackerCrawler *crawler,
	  GFile          *file)
{
	gchar *path;

	g_return_if_fail (G_IS_FILE (file));

	path = g_file_get_path (file);

	if (path_should_be_ignored (crawler, path, FALSE)) {
		crawler->private->files_ignored++;

		g_debug ("Ignored:'%s' (%d)",
			 path,
			 crawler->private->enumerations);
	} else {
		crawler->private->files_found++;

		g_debug ("Found  :'%s' (%d)",
			 path,
			 crawler->private->enumerations);

		g_queue_push_tail (crawler->private->files, g_object_ref (file));
	}

	g_free (path);
}

static void
add_directory (TrackerCrawler *crawler,
	       GFile          *file)
{
	gchar *path;

	g_return_if_fail (G_IS_FILE (file));

	path = g_file_get_path (file);

	if (path_should_be_ignored (crawler, path, TRUE)) {
		crawler->private->directories_ignored++;

		g_debug ("Ignored:'%s' (%d)",
			 path,
			 crawler->private->enumerations);
	} else {
		crawler->private->directories_found++;

		g_debug ("Found  :'%s' (%d)",
			 path,
			 crawler->private->enumerations);
		
		g_queue_push_tail (crawler->private->directories, g_object_ref (file));
	}

	g_free (path);
}

static void
process_directory (TrackerCrawler *crawler,
		   const gchar    *module_name,
		   GFile          *file)
{
	g_signal_emit (crawler, signals[PROCESSING_DIRECTORY], 0, module_name, file);

	file_enumerate_children (crawler, file);
}

static gboolean
process_func (gpointer data)
{
	TrackerCrawler        *crawler;
	TrackerCrawlerPrivate *priv;
	GFile                 *file;

	crawler = TRACKER_CRAWLER (data);
	priv = crawler->private;

	/* Crawler directory contents */
	file = g_queue_pop_head (priv->directories);

	if (file) {
		process_directory (crawler, priv->module_name, file);
		g_object_unref (file);

		return TRUE;
	}

	/* If we still have some async operations in progress, wait
	 * for them to finish, if not, we are truly done.
	 */
	if (priv->enumerations > 0) {
		return TRUE;
	}

	/* Process next path in list */
	if (!priv->handled_paths) {
		priv->handled_paths = TRUE;

		if (!priv->current_path) {
			priv->current_path = priv->paths;
		} else {
			priv->current_path = priv->current_path->next;
		}
		
		if (priv->current_path) {
			g_message ("  Searching directory:'%s'", 
				   (gchar*) priv->current_path->data);
			
			file = g_file_new_for_path (priv->current_path->data);
			add_directory (crawler, file);
			g_object_unref (file);
			
			return TRUE;
		}
	}

	/* Process next recursive path in list */
	if (!priv->handled_recurse_paths) {
		priv->handled_recurse_paths = TRUE;

		if (!priv->current_recurse_path) {
			priv->current_recurse_path = priv->recurse_paths;
		} else {
			priv->current_recurse_path = priv->current_recurse_path->next;
		}
	
		if (priv->current_recurse_path) {
			g_message ("  Searching directory:'%s' (recursively)", 
				   (gchar *) priv->current_recurse_path->data);
			
			file = g_file_new_for_path (priv->current_recurse_path->data);
			add_directory (crawler, file);
			g_object_unref (file);
			
			return TRUE;
		}
	}

	priv->idle_id = 0;
	priv->finished = TRUE;

	tracker_crawler_stop (crawler);

	return FALSE;
}

static EnumeratorData *
enumerator_data_new (TrackerCrawler *crawler,
		     GFile          *parent)
{
	EnumeratorData *ed;

	ed = g_slice_new0 (EnumeratorData);
	ed->crawler = g_object_ref (crawler);
	ed->parent = g_object_ref (parent);

	return ed;
}

static void
enumerator_data_free (EnumeratorData *ed)
{
	g_object_unref (ed->parent);
	g_object_unref (ed->crawler);
	g_slice_free (EnumeratorData, ed);
}

static void
file_enumerator_close_cb (GObject      *enumerator,
			  GAsyncResult *result,
			  gpointer      user_data)
{
	TrackerCrawler *crawler;

	crawler = TRACKER_CRAWLER (user_data);
	crawler->private->enumerations--;

	if (!g_file_enumerator_close_finish (G_FILE_ENUMERATOR (enumerator),
					     result,
					     NULL)) {
		g_warning ("Couldn't close GFileEnumerator:%p", enumerator);
	}
}

static void
file_enumerate_next_cb (GObject      *object,
			GAsyncResult *result,
			gpointer      user_data)
{
	TrackerCrawler  *crawler;
	EnumeratorData  *ed;
	GFileEnumerator *enumerator;
	GFile           *parent, *child;
	GFileInfo       *info;
	GList           *files;

	enumerator = G_FILE_ENUMERATOR (object);

	ed = (EnumeratorData*) user_data;
	crawler = ed->crawler;
	parent = ed->parent;

	files = g_file_enumerator_next_files_finish (enumerator,
						     result,
						     NULL);

	if (!files || !crawler->private->running) {
		/* No more files or we are stopping anyway, so clean
		 * up and close all file enumerators.
		 */
		enumerator_data_free (ed);
		g_file_enumerator_close_async (enumerator,
					       G_PRIORITY_DEFAULT,
					       NULL,
					       file_enumerator_close_cb,
					       crawler);
		return;
	}

	/* Files should only have 1 item in it */
	info = files->data;
	child = g_file_get_child (parent, g_file_info_get_name (info));

	if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
		/* This is a bit of a hack, but we assume this is a
		 * recursive lookup because the current non-recursive
		 * path is NULL, meaning they have all been traversed
		 * already.
		 */
		if (crawler->private->handled_paths) {
			add_directory (crawler, child);
		}
	} else {
		add_file (crawler, child);
	}

	g_object_unref (child);
	g_list_free (files);

	/* Get next file */
	file_enumerate_next (enumerator, ed);
}

static void
file_enumerate_next (GFileEnumerator *enumerator,
		     EnumeratorData  *ed)
{
	g_file_enumerator_next_files_async (enumerator,
					    1,
					    G_PRIORITY_DEFAULT,
					    NULL,
					    file_enumerate_next_cb,
					    ed);
}

static void
file_enumerate_children_cb (GObject      *file,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	TrackerCrawler  *crawler;
	EnumeratorData  *ed;
	GFileEnumerator *enumerator;
	GFile           *parent;

	parent = G_FILE (file);
	crawler = TRACKER_CRAWLER (user_data);
	enumerator = g_file_enumerate_children_finish (parent, result, NULL);

	if (!enumerator) {
		crawler->private->enumerations--;
		return;
	}

	ed = enumerator_data_new (crawler, parent);

	/* Start traversing the directory's files */
	file_enumerate_next (enumerator, ed);
}

static void
file_enumerate_children (TrackerCrawler *crawler,
			 GFile          *file)
{
	crawler->private->enumerations++;

	g_file_enumerate_children_async (file,
					 FILE_ATTRIBUTES,
					 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
					 G_PRIORITY_DEFAULT,
					 NULL,
					 file_enumerate_children_cb,
					 crawler);
}

gboolean
tracker_crawler_start (TrackerCrawler *crawler)
{
	TrackerCrawlerPrivate *priv;
	GFile                 *file;
	GSList                *paths = NULL;
	GList                 *recurse_directories;
	GList                 *directories;
	GList                 *l;
	gchar                 *path;
	gboolean               exists;

	g_return_val_if_fail (TRACKER_IS_CRAWLER (crawler), FALSE);

	priv = crawler->private;

	g_message ("Crawling directories for module:'%s'",
		   crawler->private->module_name);

	recurse_directories =
		tracker_module_config_get_monitor_recurse_directories (priv->module_name);
	directories =
		tracker_module_config_get_monitor_directories (priv->module_name);

	if (!recurse_directories && !directories) {
		g_message ("  No directories to iterate, doing nothing");
		return TRUE;
	}

	/* First we do non-recursive directories */
	for (l = directories; l; l = l->next) {
		path = l->data;

		/* Check location exists before we do anything */
		file = g_file_new_for_path (path);
		exists = g_file_query_exists (file, NULL);

		if (!exists) {
			g_message ("  Directory:'%s' does not exist",
				   path);
			g_object_unref (file);
			continue;
		}

		g_message ("  Directory:'%s' added to list to crawl",
			   path);

		priv->paths = g_slist_prepend (priv->paths, g_strdup (l->data));
		g_object_unref (file);
	}

	g_list_free (directories);

	/* Second we do recursive directories */
	for (l = recurse_directories; l; l = l->next) {
		path = l->data;

		/* Check location exists before we do anything */
		file = g_file_new_for_path (path);
		exists = g_file_query_exists (file, NULL);

		if (!exists) {
			g_message ("  Directory:'%s' does not exist",
				   path);
			g_object_unref (file);
			continue;
		}

		g_message ("  Directory:'%s' added to list to crawl (recursively)",
			   path);

		priv->recurse_paths = g_slist_prepend (priv->recurse_paths, g_strdup (l->data));
		g_object_unref (file);
	}

	g_list_free (recurse_directories);

	if (!priv->paths && !priv->recurse_paths) {
		g_message ("  No directories that actually exist to iterate, doing nothing");
		return FALSE;
	}

	priv->paths = g_slist_reverse (priv->paths);
	priv->recurse_paths = g_slist_reverse (priv->recurse_paths);

	/* Time the event */
	if (priv->timer) {
		g_timer_destroy (priv->timer);
	}

	priv->timer = g_timer_new ();

	/* Set idle handler to process directories and files found */
	priv->idle_id = g_idle_add (process_func, crawler);

	/* Set as running now */
	priv->running = TRUE;
	priv->finished = FALSE;

	/* Reset stats */
	priv->directories_found = 0;
	priv->directories_ignored = 0;
	priv->files_found = 0;
	priv->files_ignored = 0;

	g_slist_free (paths);

	return TRUE;
}

void
tracker_crawler_stop (TrackerCrawler *crawler)
{
	TrackerCrawlerPrivate *priv;

	g_return_if_fail (TRACKER_IS_CRAWLER (crawler));

	priv = crawler->private;

	g_message ("  %s crawling files in %4.4f seconds",
		   priv->finished ? "Finished" : "Stopped",
		   g_timer_elapsed (priv->timer, NULL));
	g_message ("  Found %d directories, ignored %d directories",
		   priv->directories_found,
		   priv->directories_ignored);
	g_message ("  Found %d files, ignored %d files",
		   priv->files_found,
		   priv->files_ignored);

	priv->running = FALSE;

	if (priv->idle_id) {
		g_source_remove (priv->idle_id);
	}

	g_timer_destroy (priv->timer);
	priv->timer = NULL;

	g_signal_emit (crawler, signals[FINISHED], 0,
		       priv->module_name,
		       priv->files,
		       priv->directories_found,
		       priv->directories_ignored,
		       priv->files_found,
		       priv->files_ignored);
}
