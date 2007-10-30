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


#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "response.h"
#include "client.h"

void archive_handle_response (Client *client, struct response *reply);
void archive_save (gchar *path);
void archive_load (gchar *path);

gchar * archive_get_archive_path ();

void archive_file_got (Client *client);
void archive_filestat_got (Client *client, uint64_t mtime, uint64_t size);

void archive_cleanup ();
void archive_full_cleanup ();

#endif /* __ARCHIVE_H__ */
