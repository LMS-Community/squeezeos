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


#ifndef __COMMAND_H__
#define __COMMAND_H__

#define OPROFILEUI_LOCAL_SOCKET "/tmp/oprofileui.socket"
#define OPROFILEUI_PROTO_VERSION 2

#define COMMAND_OP_START 'A'
#define COMMAND_OP_STOP 'B'
#define COMMAND_OP_ARCHIVE 'C'
#define COMMAND_OP_GET 'D'
#define COMMAND_OP_STATUS 'E'
#define COMMAND_OP_RESET 'F'
#define COMMAND_OP_FILESTAT 'G'
#define COMMAND_OP_CONFIG 'H'

struct command
{
  char opcode;
  unsigned long length;
  char payload[];
};

#define SIZE_OF_COMMAND(x) sizeof(struct command) + x->length
#endif /* __COMMAND_H__ */
