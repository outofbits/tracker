/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008, Mr Jamie McCracken (jamiemcc@gnome.org)
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

#ifndef __TRACKER_INDEXER_MODULE_H__
#define __TRACKER_INDEXER_MODULE_H__

#include <glib.h>

G_BEGIN_DECLS

GModule *               tracker_indexer_module_load                   (const gchar *module_name);

G_CONST_RETURN gchar *  tracker_indexer_module_get_name               (GModule     *module);
gchar **                tracker_indexer_module_get_directories        (GModule     *module);
gchar **                tracker_indexer_module_get_ignore_directories (GModule     *module);

GHashTable *            tracker_indexer_module_get_file_metadata      (GModule     *module,
								       const gchar *file);
gchar *                 tracker_indexer_module_get_text               (GModule     *module,
								       const gchar *file);

G_END_DECLS

#endif /* __TRACKER_INDEXER_MODULE_H__ */
