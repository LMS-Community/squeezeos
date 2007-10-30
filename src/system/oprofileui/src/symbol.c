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


#include "symbol.h"
#include "symbol_instance.h"

G_DEFINE_TYPE (Symbol, symbol, G_TYPE_OBJECT);

static void
symbol_dispose (GObject *object)
{
  Symbol *symbol = SYMBOL (object);

  if (symbol->count == 0)
    return;

  symbol->count = 0;

  g_slist_foreach (symbol->instances, (GFunc)g_object_unref, NULL);
  g_slist_free (symbol->instances);

  g_free (symbol->name);

  if (G_OBJECT_CLASS (symbol_parent_class)->dispose)
    G_OBJECT_CLASS (symbol_parent_class)->dispose (object);
}

static void
symbol_finalize (GObject *object)
{
  G_OBJECT_CLASS (symbol_parent_class)->finalize (object);
}

static void
symbol_class_init (SymbolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = symbol_dispose;
  object_class->finalize = symbol_finalize;
}


static void
symbol_init (Symbol *symbol)
{
  symbol->name = NULL;
  symbol->instances = NULL;
  symbol->count = 0;
  symbol->report = NULL;
  symbol->module = NULL;
  symbol->callees = NULL;
  symbol->callers = NULL;
}

Symbol*
symbol_new (void)
{
  return g_object_new (TYPE_SYMBOL, NULL);
}

void
symbol_add_instance (Symbol *symbol, SymbolInstance *symbol_instance)
{
  g_object_ref (symbol_instance);

  /* Add this instance to the list of instances for the symbol */
  symbol->instances = g_slist_append (symbol->instances, symbol_instance);
}

static void
symbol_child_increment_count (GSList **child_list, Symbol *symbol, Symbol *child, guint count)
{
  GSList *cur_entry = NULL;
  SymbolInstance *symbol_instance = NULL;

  for (cur_entry = *child_list; cur_entry != NULL; cur_entry = g_slist_next (cur_entry))
    {
      SymbolInstance *tmp = SYMBOL_INSTANCE (cur_entry->data);
      
      if (tmp->symbol == child)
        {
          symbol_instance = tmp;
          break;
        }
    }

  if (symbol_instance == NULL)
    {
      symbol_instance = symbol_instance_new ();
      symbol_instance_set_symbol (symbol_instance, child);
      *child_list = g_slist_append (*child_list, symbol_instance);
    }

  symbol_instance->count += count;
}

void 
symbol_callee_increment_count (Symbol *symbol, Symbol *callee, guint count)
{
  symbol_child_increment_count (&symbol->callees, symbol, callee, count);
}

void 
symbol_caller_increment_count (Symbol *symbol, Symbol *caller, guint count)
{
  symbol_child_increment_count (&symbol->callers, symbol, caller, count);
}

void
symbol_set_module (Symbol *symbol, Module *module)
{
  symbol->module = module;
}
