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


#include <glib.h>

#include "types.h"
#include "symbol_instance.h"
#include "symbol.h"

G_DEFINE_TYPE (SymbolInstance, symbol_instance, G_TYPE_OBJECT);

static void
symbol_instance_dispose (GObject *object)
{
  SymbolInstance *symbol_instance = SYMBOL_INSTANCE (object);

  if (symbol_instance->count == 0)
    return;

  symbol_instance->count = 0;

  g_slist_foreach (symbol_instance->callees, (GFunc)g_object_unref, NULL);
  g_slist_foreach (symbol_instance->callers, (GFunc)g_object_unref, NULL);

  g_slist_free (symbol_instance->callees);
  g_slist_free (symbol_instance->callers);

  if (G_OBJECT_CLASS (symbol_instance_parent_class)->dispose)
    G_OBJECT_CLASS (symbol_instance_parent_class)->dispose (object);
}

static void
symbol_instance_finalize (GObject *object)
{
  G_OBJECT_CLASS (symbol_instance_parent_class)->finalize (object);
}

static void
symbol_instance_class_init (SymbolInstanceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = symbol_instance_dispose;
  object_class->finalize = symbol_instance_finalize;
}

static void
symbol_instance_init (SymbolInstance *symbol_instance)
{
  symbol_instance->symbol = NULL;
  symbol_instance->count = 0;
  symbol_instance->image = NULL;
  symbol_instance->callees = NULL;
  symbol_instance->callers = NULL;
}

SymbolInstance*
symbol_instance_new (void)
{
  return g_object_new (TYPE_SYMBOL_INSTANCE, NULL);
}

void
symbol_instance_set_symbol (SymbolInstance *symbol_instance, Symbol *symbol)
{
  symbol_instance->symbol = symbol;
}

void
symbol_instance_set_image (SymbolInstance *symbol_instance, Image *image)
{
  symbol_instance->image = image;
}

void
symbol_instance_add_callee (SymbolInstance *symbol_instance, CalleeSymbolInstance *callee)
{
  g_object_ref (callee);
  symbol_instance->callees = g_slist_append (symbol_instance->callees, callee);
}

void
symbol_instance_add_caller (SymbolInstance *symbol_instance, CallerSymbolInstance *caller)
{
  g_object_ref (caller);
  symbol_instance->callers = g_slist_append (symbol_instance->callers, caller);
}

