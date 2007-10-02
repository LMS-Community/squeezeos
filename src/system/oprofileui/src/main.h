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


#ifndef __MAIN_H_
#define __MAIN_H_

#include <glib.h>

enum oprofileui_state {
    OPUI_INIT,              /* Initial State */
    OPUI_DISCONNECTING,     /* In process of disconnecting a client */
    OPUI_DISCONNECTED,      /* No connected client */
    OPUI_CONNECTING,        /* In process of connecting to a client */
    OPUI_CONNECTED_WAIT,    /* Connected, waiting for status (command running) */
    OPUI_CONNECTED_IDLE,    /* Connected, profiling stopped */
    OPUI_CONNECTED_RUNNING, /* Connected, profiling active */
};

void main_status_response_cb (gboolean success, gchar *err_message, 
    struct response_status *status);
void main_start_response_cb (gboolean success, gchar *err_message);
void main_stop_response_cb (gboolean success, gchar *err_message);
void main_archive_response_cb (gboolean success, gchar *err_message);
void main_connect_response_cb (gboolean success, gchar *err_message);
void main_disconnect_response_cb (gboolean success, gchar *err_message);
void main_archive_finished_cb (gboolean success, gchar *err_message);
void main_reset_response_cb (gboolean success, gchar *err_message);
void main_config_response_cb (gboolean success, gchar *err_message);

void main_connection_lost_cb ();
void main_generic_error (gchar *err_message);

void main_set_state (enum oprofileui_state new_state);

#endif /* __MAIN_H_ */
