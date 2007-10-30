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


#include "module_instance.h"

G_DEFINE_TYPE (ModuleInstance, module_instance, G_TYPE_OBJECT);

static void
module_instance_dispose (GObject *object)
{
  ModuleInstance *module_instance = MODULE_INSTANCE (object);

  if (module_instance->count == 0)
    return;

  module_instance->count = 0;

  g_slist_foreach (module_instance->symbol_instances, (GFunc)g_object_unref, NULL);
  g_slist_free (module_instance->symbol_instances);

  if (G_OBJECT_CLASS (module_instance_parent_class)->dispose)
    G_OBJECT_CLASS (module_instance_parent_class)->dispose (object);
}

static void
module_instance_finalize (GObject *object)
{
  G_OBJECT_CLASS (module_instance_parent_class)->finalize (object);
}

static void
module_instance_class_init (ModuleInstanceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = module_instance_dispose;
  object_class->finalize = module_instance_finalize;
}

static void
module_instance_init (ModuleInstance *module_instance)
{
  module_instance->module = NULL;
  module_instance->count = 0;
  module_instance->symbol_instances = NULL;
  module_instance->image = NULL;
}

ModuleInstance*
module_instance_new (void)
{
  return g_object_new (TYPE_MODULE_INSTANCE, NULL);
}

void
module_instance_set_module (ModuleInstance *module_instance, Module *module)
{
  module_instance->module = module;
}

void
module_instance_add_symbol_instance (ModuleInstance *module_instance, SymbolInstance *symbol_instance)
{
  g_object_ref (symbol_instance);

  /* Add symbol instance that occurs within a module */
  module_instance->symbol_instances = 
    g_slist_append (module_instance->symbol_instances, symbol_instance);
}

void
module_instance_set_image (ModuleInstance *module_instance, Image *image)
{
  module_instance->image = image;
}

