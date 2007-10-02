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


#ifndef _SYMBOL
#define _SYMBOL

#include <glib-object.h>
#include <glib.h>

#include "types.h"

#include "report.h"
#include "module.h"

G_BEGIN_DECLS

#define TYPE_SYMBOL symbol_get_type()

#define SYMBOL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_SYMBOL, Symbol))

#define SYMBOL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_SYMBOL, SymbolClass))

#define IS_SYMBOL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_SYMBOL))

#define IS_SYMBOL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_SYMBOL))

#define SYMBOL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_SYMBOL, SymbolClass))

struct _Symbol {
  GObject parent;
  gchar *name;
  GSList *instances;
  guint count;
  Report *report;
  Module *module;
  GSList *callees;
  GSList *callers;
};


typedef struct {
  GObjectClass parent_class;
} SymbolClass;

GType symbol_get_type (void);

Symbol* symbol_new (void);

void symbol_add_instance (Symbol *symbol, SymbolInstance *symbol_instance);

void symbol_set_module (Symbol *symbol, Module *module);
void  symbol_callee_increment_count (Symbol *symbol, Symbol *callee, 
    guint count);
void  symbol_caller_increment_count (Symbol *symbol, Symbol *caller, 
    guint count);

G_END_DECLS

#endif /* _SYMBOL */
