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


#ifndef __CLIENT_H__
#define __CLIENT_H__
#include <glib.h>
#include "command.h"

typedef struct _Client Client;

gboolean client_channel_read_cb (GIOChannel *source, GIOCondition condition, gpointer data);

void client_connect (Client *client, char *hostname, int port);
void client_disconnect (Client *client);

struct command *client_command_new (char opcode, int length, char *payload);

void client_send_command_archive (Client *client);
void client_send_command_start (Client *client);
void client_send_command_stop (Client *client);
void client_send_command_status (Client *client);
void client_send_command_reset (Client *client);
void client_send_command_get (Client *client, gchar *path);
void client_send_command_filestat (Client *client, gchar *path);
void client_send_command_config (Client *client, gchar *options);

Client *client_new (void);
#endif /* __CLIENT_H__ */

