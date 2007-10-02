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


#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#define RESPONSE_OP_OK 'A'
#define RESPONSE_OP_ERROR 'B'
#define RESPONSE_OP_FILE 'C'
#define RESPONSE_OP_ARCHIVE 'D'
#define RESPONSE_OP_STATUS 'E'
#define RESPONSE_OP_FILESTAT 'F'

struct response
{
  char opcode;
  uint32_t length;
  char payload[];
};

struct response_filestat
{
  uint64_t mtime;
  uint64_t size;
} __attribute__((packed));

struct response_status
{
  uint32_t proto_version;
  uint32_t profiling :1;
  uint32_t configured :1;
} __attribute__((packed));

#define SIZE_OF_RESPONSE(x) sizeof(struct response) + x->length
#endif /* __RESPONSE_H__ */
