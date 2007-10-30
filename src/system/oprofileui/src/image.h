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


#ifndef _IMAGE
#define _IMAGE

#include <glib-object.h>
#include <glib.h>

#include "types.h"

G_BEGIN_DECLS

#define TYPE_IMAGE image_get_type()

#define IMAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_IMAGE, Image))

#define IMAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_IMAGE, ImageClass))

#define IS_IMAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_IMAGE))

#define IS_IMAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_IMAGE))

#define IMAGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_IMAGE, ImageClass))

struct _Image {
  GObject parent;
  gchar *path;
  guint count;
  GSList *symbol_instances;
  GSList *module_instances;
};

typedef struct {
  GObjectClass parent_class;
} ImageClass;

GType image_get_type (void);

Image* image_new (void);

void image_add_symbol_instance (Image *image, SymbolInstance *symbol_instance);
void image_add_module_instance (Image *image, ModuleInstance *module_instance);

G_END_DECLS

#endif /* _IMAGE */
