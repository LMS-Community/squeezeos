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


#ifndef _MODULE
#define _MODULE

#include <glib-object.h>
#include <glib.h>
#include "types.h"
#include "report.h"

G_BEGIN_DECLS

#define TYPE_MODULE module_get_type()

#define MODULE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_MODULE, Module))

#define MODULE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_MODULE, ModuleClass))

#define IS_MODULE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_MODULE))

#define IS_MODULE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_MODULE))

#define MODULE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_MODULE, ModuleClass))

struct _Module {
  GObject parent;
  gchar *name;
  guint count;
  GSList *instances;
  GSList *symbols;
  Report *report;
} ;


typedef struct {
  GObjectClass parent_class;
} ModuleClass;

GType module_get_type (void);

Module* module_new (void);

void module_add_instance (Module *module, ModuleInstance *module_instance);
void module_add_symbol (Module *module, Symbol *symbol);

G_END_DECLS

#endif /* _MODULE */
