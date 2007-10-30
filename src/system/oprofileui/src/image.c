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


#include "image.h"

G_DEFINE_TYPE (Image, image, G_TYPE_OBJECT);

static void
image_dispose (GObject *object)
{
  Image *image = IMAGE(object);

  if (image->count == 0)
    return;

  image->count = 0;

  g_slist_foreach (image->symbol_instances, (GFunc)g_object_unref, NULL);
  g_slist_foreach (image->module_instances, (GFunc)g_object_unref, NULL);

  g_slist_free (image->symbol_instances);
  g_slist_free (image->module_instances);

  g_free (image->path);

  if (G_OBJECT_CLASS (image_parent_class)->dispose)
    G_OBJECT_CLASS (image_parent_class)->dispose (object);
}

static void
image_finalize (GObject *object)
{
  G_OBJECT_CLASS (image_parent_class)->finalize (object);
}

static void
image_class_init (ImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = image_dispose;
  object_class->finalize = image_finalize;
}

static void
image_init (Image *image)
{
  image->path = NULL;
  image->count = 0;
  image->symbol_instances = NULL;
  image->module_instances = NULL;
}

Image*
image_new (void)
{
  return g_object_new (TYPE_IMAGE, NULL);
}

void
image_add_symbol_instance (Image *image, SymbolInstance *symbol_instance)
{
  g_object_ref (symbol_instance);

  image->symbol_instances = g_slist_append (image->symbol_instances,
      symbol_instance);
}

void
image_add_module_instance (Image *image, ModuleInstance *module_instance)
{
  g_object_ref (module_instance);

  image->module_instances = g_slist_append (image->module_instances,
      module_instance);
}

