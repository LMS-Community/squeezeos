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


#ifndef __OPROFILEUI_H_
#define __OPROFILEUI_H_

#include <glib.h>

struct oprofileui_config {
    /* Connection Config */
    gchar *host;
    gint port;
    gboolean localhost;

    /* Options */
    gchar *opcontrol_params;
    gchar *binaries_location;
    gchar *vmlinux_location;

    /* View Config */
    gboolean group_by_application;
    gboolean group_by_module;
};

extern struct oprofileui_config *opui_config;

#endif /* __OPROFILEUI_H_ */
