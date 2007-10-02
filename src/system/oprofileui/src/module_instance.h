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


#ifndef _MODULE_INSTANCE
#define _MODULE_INSTANCE

#include <glib-object.h>
#include "types.h"
#include "image.h"

G_BEGIN_DECLS

#define TYPE_MODULE_INSTANCE module_instance_get_type()

#define MODULE_INSTANCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_MODULE_INSTANCE, ModuleInstance))

#define MODULE_INSTANCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_MODULE_INSTANCE, ModuleInstanceClass))

#define IS_MODULE_INSTANCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_MODULE_INSTANCE))

#define IS_MODULE_INSTANCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_MODULE_INSTANCE))

#define MODULE_INSTANCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_MODULE_INSTANCE, ModuleInstanceClass))

struct _ModuleInstance {
  GObject parent;
  Module *module;
  guint count;
  GSList *symbol_instances;
  Image *image;
};

typedef struct {
  GObjectClass parent_class;
} ModuleInstanceClass;

GType module_instance_get_type (void);

ModuleInstance* module_instance_new (void);


void module_instance_set_module (ModuleInstance *module_instance, Module *module);
void module_instance_add_symbol_instance (ModuleInstance *module_instance, SymbolInstance *symbol_instance);
void module_instance_set_image (ModuleInstance *module_instance, Image *image);

G_END_DECLS

#endif /* _MODULE_INSTANCE */
