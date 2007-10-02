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


#ifndef _SYMBOL_INSTANCE
#define _SYMBOL_INSTANCE

#include <glib-object.h>
#include "image.h"
#include "types.h"

G_BEGIN_DECLS

#define TYPE_SYMBOL_INSTANCE symbol_instance_get_type()

#define SYMBOL_INSTANCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_SYMBOL_INSTANCE, SymbolInstance))

#define SYMBOL_INSTANCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_SYMBOL_INSTANCE, SymbolInstanceClass))

#define IS_SYMBOL_INSTANCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_SYMBOL_INSTANCE))

#define IS_SYMBOL_INSTANCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_SYMBOL_INSTANCE))

#define SYMBOL_INSTANCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_SYMBOL_INSTANCE, SymbolInstanceClass))

struct _SymbolInstance {
  GObject parent;
  Symbol *symbol;
  guint count;
  Image *image;
  GSList *callees;
  GSList *callers;
};

typedef struct {
  GObjectClass parent_class;
} SymbolInstanceClass;

GType symbol_instance_get_type (void);

SymbolInstance* symbol_instance_new (void);

void symbol_instance_set_symbol (SymbolInstance *symbol_instance, Symbol *symbol);
void symbol_instance_set_image (SymbolInstance *symbol_instance, Image *image);

void symbol_instance_add_callee (SymbolInstance *symbol_instance, CalleeSymbolInstance *callee);
void symbol_instance_add_caller (SymbolInstance *symbol_instance, CallerSymbolInstance *caller);

G_END_DECLS

#endif /* _SYMBOL_INSTANCE */
