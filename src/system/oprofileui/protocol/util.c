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


#include <unistd.h>
#include <sys/sendfile.h>

#include "util.h"

int
util_read_in_full (int fd, char *buf, size_t count)
{
  size_t pos = 0;

  while (count-pos > 0)
    {
      int res = read (fd, &buf[pos], count-pos);
  
      if (res < 0)
        {
          return res;
        }

      if (res == 0)
        return 0;

      pos += res;
    }

  return pos;
}

int
util_write_in_full (int fd, char *buf, size_t count)
{
  size_t pos = 0;

  while (count - pos > 0)
    {
      int res = write (fd, &buf[pos], count-pos);
  
      if (res < 0)
        {
          return res;
        }

      if (res == 0)
        return 0;

      pos += res;
    }

  return pos;
}

int 
util_sendfile_in_full (int src_fd, int dest_fd, size_t count)
{
  off_t offset = 0;

  while (count - offset > 0)
    {
      int res = sendfile (src_fd, dest_fd, &offset, count-offset);

      if (res < 0)
        {
          return res;
        }

      if (res == 0)
        return 0;
    }

  return count;
}

