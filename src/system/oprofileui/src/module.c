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


#include "module.h"

G_DEFINE_TYPE (Module, module, G_TYPE_OBJECT);

static void
module_dispose (GObject *object)
{
  Module *module = MODULE (object);

  if (module->count == 0)
    return;

  module->count = 0;

  g_slist_foreach (module->instances, (GFunc)g_object_unref, NULL);
  g_slist_foreach (module->symbols, (GFunc)g_object_unref, NULL);

  g_slist_free (module->instances);
  g_slist_free (module->symbols);

  g_free (module->name);

  if (G_OBJECT_CLASS (module_parent_class)->dispose)
    G_OBJECT_CLASS (module_parent_class)->dispose (object);
}

static void
module_finalize (GObject *object)
{
  G_OBJECT_CLASS (module_parent_class)->finalize (object);
}

static void
module_class_init (ModuleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = module_dispose;
  object_class->finalize = module_finalize;
}

static void
module_init (Module *module)
{
  module->name = NULL;
  module->count = 0;
  module->instances = NULL;
  module->symbols = NULL;
  module->report = NULL;
}

Module*
module_new (void)
{
  return g_object_new (TYPE_MODULE, NULL);
}

void
module_add_instance (Module *module, ModuleInstance *module_instance)
{
  g_object_ref (module_instance);
  module->instances = g_slist_append (module->instances, module_instance);
}

void
module_add_symbol (Module *module, Symbol *symbol)
{
  /* Check if the symbol is in the list of symbols for this module */
  if (g_slist_find (module->symbols, symbol) == NULL)
    {
      //If not then add it
      g_object_ref (symbol);
      module->symbols = g_slist_append (module->symbols, symbol);
    }
}
