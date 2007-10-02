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


#ifndef __VIEW_H__
#define __VIEW_H__

#include <glade/glade.h>
#include "report.h"

#define TYPE_VIEW view_get_type()

#define VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_VIEW, View))

#define VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_VIEW, ViewClass))

#define IS_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_VIEW))

#define IS_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_VIEW))

#define VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_VIEW, ViewClass))

typedef struct
{
  GObject parent;
} View;

typedef struct
{
  GObjectClass parent_class;
} ViewClass;

void view_set_display_mode (View *view, gboolean group_by_application, gboolean group_by_module);

View *view_new (GladeXML *xml);
#endif /* __VIEW_H__ */
