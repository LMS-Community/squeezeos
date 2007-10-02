/*
 * OProfile User Interface
 *
 * Copyright (C) 2007 Nokia Corporation. All rights reserved.
 *
 * Author: Robert Bradford <rob@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */


#ifndef __REPORT_H__
#define __REPORT_H__

#include <glib.h>
#include <gtk/gtk.h>
#include "types.h"

G_BEGIN_DECLS

#define TYPE_REPORT report_get_type()

#define REPORT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_REPORT, Report))

#define REPORT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_REPORT, ReportClass))

#define IS_REPORT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_REPORT))

#define IS_REPORT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_REPORT))

#define REPORT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_REPORT, ReportClass))

struct _Report
{
  GObject parent;
  GSList *images;
  GSList *symbols;
  GSList *modules;
  guint count;
};

typedef struct {
  GObjectClass parent_class;
} ReportClass;

GtkListStore *report_get_symbol_details_liststore (Report *report, Symbol *symbol);
GtkListStore *report_get_module_details_liststore (Report *report, Module *module);

GtkListStore *report_get_symbol_instance_callees (Report *report, SymbolInstance *symbol_instance);
GtkListStore *report_get_symbol_instance_callers (Report *report, SymbolInstance *symbol_instance);

GtkTreeStore *report_get_treestore (Report *report, gboolean group_by_application, gboolean group_by_module);

GtkListStore *report_get_symbol_callers (Report *report, Symbol *symbol);
GtkListStore *report_get_symbol_callees (Report *report, Symbol *symbol);

GType report_get_type (void);

void report_add_symbol (Report *report, Symbol *symbol);
void report_add_image (Report *report, Image *image);
void report_add_module (Report *report, Module *module);

Report *report_new_from_archive_path (gchar *archive_path);

G_END_DECLS

#endif /* __REPORT_H__ */
