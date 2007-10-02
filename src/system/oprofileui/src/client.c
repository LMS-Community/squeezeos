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


#include <stdlib.h>
#include <glib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <assert.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>

#include "response.h"
#include "command.h"

#include "archive.h"
#include "client.h"
#include "main.h"
#include "util.h"

struct _Client {
  GIOChannel *connection;
  struct response *cur_response;
  struct command *cur_cmd;
};

void
client_handle_response_status (Client *client, struct response *reply)
{
  if (reply->opcode == RESPONSE_OP_STATUS)
    {
      struct response_status *status = (struct response_status *) &reply->payload[0];

      if ((reply->length != sizeof (struct response_status)) || 
              (status->proto_version != OPROFILEUI_PROTO_VERSION))
        {
          main_status_response_cb (FALSE, "Protocol of server doesn't match viewer.", 0);
        }
      else
        {
          main_status_response_cb (TRUE, NULL, status);
        }
    } 
  else
   {
      if (reply->opcode == RESPONSE_OP_ERROR)
        {
          main_status_response_cb (FALSE, reply->payload, 0);
        }
    }
}

void
client_handle_response_config (Client *client, struct response *reply)
{
  if (reply->opcode == RESPONSE_OP_OK)
    {
      main_config_response_cb (TRUE, NULL);
    } 
  if (reply->opcode == RESPONSE_OP_ERROR)
    {
      printf ("ERROR: %s\n", reply->payload);
      main_config_response_cb (FALSE, reply->payload);
    }
}

void
client_handle_response_stop (Client *client, struct response *reply)
{
  if (reply->opcode == RESPONSE_OP_OK)
    {
      main_stop_response_cb (TRUE, NULL);
    }

  if (reply->opcode == RESPONSE_OP_ERROR)
    {
      printf ("ERROR: %s\n", reply->payload);
      main_stop_response_cb (FALSE, reply->payload);
    }

  g_free (reply);
}

void
client_handle_response_reset (Client *client, struct response *reply)
{
  if (reply->opcode == RESPONSE_OP_OK)
    {
      main_reset_response_cb (TRUE, NULL);
    }

  if (reply->opcode == RESPONSE_OP_ERROR)
    {
      printf ("ERROR: %s\n", reply->payload);
      main_reset_response_cb (FALSE, reply->payload);
    }

  g_free (reply);
}

void
client_handle_response_start (Client *client, struct response *reply)
{
  if (reply->opcode == RESPONSE_OP_OK)
    {
      main_start_response_cb (TRUE, NULL);
    }

  if (reply->opcode == RESPONSE_OP_ERROR)
    {
      printf ("ERROR: %s\n", reply->payload);
      main_start_response_cb (FALSE, reply->payload);
    }

  g_free (reply);
}

void
client_handle_response_archive (Client *client, struct response *reply)
{
  if (reply->opcode == RESPONSE_OP_ERROR)
    {
      main_archive_response_cb (FALSE, reply->payload);
    } else {
      main_archive_response_cb (TRUE, NULL);
      archive_handle_response (client, reply);
    }

  g_free (reply);
}

void
client_handle_response_get (Client *client, struct response *reply)
{
  archive_file_got (client);

  g_free (reply);
}

void
client_handle_response_filestat (Client *client, struct response *reply)
{
  struct response_filestat *filestat = (struct response_filestat *) &reply->payload[0];

  archive_filestat_got (client, filestat->mtime, filestat->size);

  g_free (reply);
}

void
client_handle_connection_lost (Client *client)
{
  client_disconnect (client);
  main_connection_lost_cb ();
}

/*
 * This callback is fired whenever there is data to be read from the channel.
 * It is used to process data about a response and depending on what that
 * response is it will call the appropriate handler/callback.
 *
 * This is a very scary function. I've thought about ways to split this up,
 * but they all seem pretty fugly.
 *
 */
gboolean 
client_channel_read_cb (GIOChannel *source, GIOCondition cond, gpointer data)
{
  Client *client = (Client *)data;
  GError *err = NULL;
  struct response *reply = client->cur_response;

  /*
   * Used for GET file requests to channel the data straight into the target
   * file
   */
  static int file_fd = 0;

  /* 
   * This is the number of bytes that of the response that we have already
   * read in
   */
  static unsigned long already_read = 0;

  /*
   * This is needed to poke the I/O channel into updating it's internal state.
   */
  if (!(g_io_channel_get_flags (source) & G_IO_FLAG_IS_READABLE))
    return TRUE;

  /* If this is a new response or a partially retrieved header of response */
  if (reply == NULL || 
        already_read < sizeof (struct response))
    {
      gsize bytes_read = 0;
      GIOStatus status;

      /* 
       * We must set the cur_response to NULL after we have dealt with
       * the response
       */
      if (reply  == NULL)
        {
          /* Create a new response to be filled */
          reply = g_malloc0 (sizeof (struct response));
          already_read = 0; /* Important: zero this counter */
        }

      /*
       * Read in as much data as we can but no more than we strictly need at
       * this point. This I/O channel is non-blocking. So it is a real
       * possibility that we might get a short read.
       */
      status = g_io_channel_read_chars (source, 
          (gchar *)&reply[already_read], 
          sizeof (struct response) - already_read, 
          &bytes_read, &err);

      /* Update counter */
      already_read += bytes_read; 

      if (status == G_IO_STATUS_ERROR)
        {
          main_generic_error (err->message);
          g_clear_error (&err);
          return TRUE;
        }

      if (status == G_IO_STATUS_EOF)
        {
          /* Improve error handling here */
          client_handle_connection_lost (client);
          return FALSE;
        }

      /* If we have read in the complete header of the structure */
      if (already_read == sizeof (struct response))
        {
          /* Prepare to receive a file */
          if (reply->opcode == RESPONSE_OP_FILE)
            {
              gchar *path = client->cur_cmd->payload;
              gchar *dest;
              gchar *dirname;
              gchar *archive_path;

              archive_path = archive_get_archive_path (); 

              /* Generate the destination directory structure */
              dest = g_strdup_printf("%s/%s",archive_path, path);
              dirname = g_path_get_dirname (dest);
              g_mkdir_with_parents (dirname, 00700);
              
              file_fd = open (dest, O_WRONLY|O_CREAT, 00755);

              if (file_fd < 0)
                {
                  printf ("Error opening file for writing: %s, %s", dest,
                        g_strerror (errno));
                }

              g_free (dirname);
              g_free (dest);
            } else {
              /* 
               * In all other cases we must allocate more memory for the
               * payload if necessary.
               */
              if (reply->length > 0)
                {
                  /* If we have more data to get then allocate some memory */
                  reply = g_realloc (reply, SIZE_OF_RESPONSE(reply));
                }
            }

        }

      /* Update pointer in state */
      client->cur_response = reply;

    } else {

      /* We get here if we just need to collect the payload */
      if (already_read < SIZE_OF_RESPONSE (reply))

        /* And we need to get some more still */
        {
          if (reply->opcode == RESPONSE_OP_ERROR ||
                reply->opcode == RESPONSE_OP_ARCHIVE ||
                reply->opcode == RESPONSE_OP_FILE ||
                reply->opcode == RESPONSE_OP_FILESTAT ||
                reply->opcode == RESPONSE_OP_STATUS)
            {
              gsize bytes_read = 0;
              GIOStatus status;
              gchar *buf;

              /* Like already_read but for the payload */
              static int payload_pos = 0;

              if (reply->opcode == RESPONSE_OP_FILE)
              {
                buf = g_malloc0 (reply->length);
              } else {
                buf = &(reply->payload[0]);
              }

              /* 
               * Once again read in as much as possible but not more than we
               * need.
               */
              status = g_io_channel_read_chars (source, 
                  &buf[payload_pos],
                  reply->length-payload_pos, 
                  &bytes_read, &err);

              /* On error fire a generic handler */
              if (status == G_IO_STATUS_ERROR)
                {
                  main_generic_error (err->message);
                  g_clear_error (&err);
                  return TRUE;
                }

              /* 
               * If we get an EOF. Then we've lost the connection with the
               * server 
               */
              if (status == G_IO_STATUS_EOF)
                {
                  client_handle_connection_lost (client);
                  return FALSE;
                }

              /* 
               * If we are dealing with a file GET then we need to dump out
               * the output to a file
               */
              if (reply->opcode == RESPONSE_OP_FILE)
                {
                  int res = 0;
                  res = util_write_in_full (file_fd, &buf[payload_pos], bytes_read);

                  if (res < 0)
                    {
                      const gchar *err_string = g_strerror (errno);

                      main_generic_error ((gchar *)err_string);
                    }

                  g_free (buf);
                }

              /* Update counters */
              payload_pos += bytes_read;
              already_read += bytes_read;

              /* 
               * When we have read in all the payload then we must reset the
               * payload counter for the next time.
               */
              if (payload_pos == reply->length)
                payload_pos = 0;
            }
        }
  }

  /* Test if we have basically finished doing what we needed to do */
  if (already_read >= sizeof (struct response) && 
        already_read == SIZE_OF_RESPONSE(reply))
    {
      char opcode = client->cur_cmd->opcode;

      /* Clear the previous command */
      g_free (client->cur_cmd);
      client->cur_cmd = NULL;

      switch (opcode)
        {
          case COMMAND_OP_START:
            client_handle_response_start (client, reply);
            break;
          case COMMAND_OP_ARCHIVE:
            client_handle_response_archive (client, reply);
            break;
          case COMMAND_OP_STOP:
            client_handle_response_stop (client, reply);
            break;
          case COMMAND_OP_RESET:
            client_handle_response_reset (client, reply);
            break;
          case COMMAND_OP_GET:
            /* Close the file handle we opened earlier (for file GET mode) */
            close (file_fd);
            client_handle_response_get (client, reply);
            break;
          case COMMAND_OP_STATUS:
            client_handle_response_status (client, reply);
            break;
          case COMMAND_OP_CONFIG:
            client_handle_response_config (client, reply);
            break;
          case COMMAND_OP_FILESTAT:
            client_handle_response_filestat (client, reply);
            break;
          default:
            g_warning ("Unknown response type received: %x.", opcode);
        }

      /* Must reset current to NULL */
      client->cur_response = NULL;

    }
  return TRUE;
}

void
client_connect (Client *client, char *hostname, int port)
{
  GError *err = NULL;

  int fd = 0;
  int sock_type, addrlen;
  struct sockaddr_in addr_in;
  struct sockaddr_un addr_un;
  struct sockaddr *addr;
  struct hostent *host = gethostbyname (hostname);
  char *ip;
  
  if (port == -1)
    {
      sock_type = AF_UNIX;
      memset ((char *)&addr_un, sizeof(addr_un), 0);
      addr_un.sun_family = AF_UNIX;
      strncpy (addr_un.sun_path, hostname, sizeof(addr_un.sun_path));
      addr = (struct sockaddr *) &addr_un;
      addrlen = strlen(addr_un.sun_path) + sizeof(addr_un.sun_family);;
    }
  else if (host)
    {
      sock_type = AF_INET;
      ip = host->h_addr_list[0];
      addr_in.sin_family = AF_INET;
      addr_in.sin_port = htons (port);
      memcpy (&addr_in.sin_addr.s_addr, ip, sizeof (ip));
      addr = (struct sockaddr *) &addr_in;
      addrlen = sizeof (addr_in);
    }
  else
    {
      gchar *err_string = g_strdup_printf ("Could not resolve the hostname for \"%s\"", hostname);

      main_connect_response_cb (FALSE, err_string);

      g_free (err_string);

      return;
    }

  fd = socket (sock_type, SOCK_STREAM, 0);

  if (fd < 0)
    {
      gchar *err_string = NULL;
      const gchar *error = g_strerror (errno);

      err_string = g_strdup_printf ("Could not open client socket: %s\n", error);

      main_connect_response_cb (FALSE, err_string);

      g_free (err_string);

      return;
    }

  if (connect (fd, addr, addrlen) == -1)
    {
      gchar *err_string = NULL;
      const gchar *error = g_strerror (errno);

      err_string = g_strdup_printf ("Could not open client socket: %s\n", error);

      main_connect_response_cb (FALSE, err_string);

      g_free (err_string);

      return;
    }

  client->connection = g_io_channel_unix_new (fd);

  /* 
   * Set the channel to non-blocking. This is necessary due to the possibility
   * of a callback being fired but there actually being no data to be read.
   */
  g_io_channel_set_flags (client->connection, G_IO_FLAG_NONBLOCK, &err);

  if (err != NULL)
    {
      main_connect_response_cb (FALSE, err->message);
      g_clear_error (&err);
      return;
    }

  /* NULL encoding because we have raw binary data */
  g_io_channel_set_encoding (client->connection, NULL, &err);

  if (err != NULL)
    {
      main_connect_response_cb (FALSE, err->message);
      g_clear_error (&err);
      return;
    }

  /* Don't buffer the channel */
  g_io_channel_set_buffered (client->connection, FALSE);

  /* Setup the callback */
  g_io_add_watch (client->connection, G_IO_IN, client_channel_read_cb, client);;

  main_connect_response_cb (TRUE, NULL);
} 

void
client_disconnect (Client *client)
{
  GError *err = NULL;

  /* Shutdown the channel and flush */
  g_io_channel_shutdown (client->connection, TRUE, &err);

  /* Unref -> destroy */
  g_io_channel_unref (client->connection);

  client->connection = NULL;

  if (err == NULL)
    {
      main_disconnect_response_cb (TRUE, NULL);
    } else {
      main_disconnect_response_cb (FALSE, err->message);
      g_clear_error (&err);
    }
}

struct command *
client_command_new (char opcode, int length, char *payload)
{
  struct command *buf;

  buf = g_malloc0 (sizeof (struct command) + length);

  buf->opcode = opcode;
  buf->length = length;

  if (length > 0)
    {
      assert (payload != NULL);
      memcpy(&(buf->payload[0]), payload, length);
    }

  return buf;
}

void
client_send_command (Client *client, struct command *cmd, GError **err)
{
  GIOStatus status;

  /* 
   * If this is the case then a command has been issued and the reponse has
   * not been processed. The state management should prevent this from
   * happening.
   */
  g_assert (client->cur_cmd == NULL);


  /*
   * Set the current command in the state. Needed for demultiplexing the responses later
   */
  client->cur_cmd = cmd;

  status = g_io_channel_write_chars (client->connection, (gchar *)cmd, 
      SIZE_OF_COMMAND(cmd), NULL, err);
}

void
client_send_command_archive (Client *client)
{
  struct command *cmd;
  GError *err = NULL;

  cmd = client_command_new (COMMAND_OP_ARCHIVE, 0, NULL);

  client_send_command(client, cmd, &err);

  if (err != NULL)
    {
      main_archive_response_cb (FALSE, err->message);
      g_clear_error (&err);
    }
}

void
client_send_command_get (Client *client, gchar *path)
{
  struct command *cmd;
  GError *err = NULL;

  if (path != NULL)
    {
      cmd = client_command_new (COMMAND_OP_GET, strlen (path)+1, path);

      client_send_command (client, cmd, &err);
    }
}

void
client_send_command_filestat (Client *client, gchar *path)
{
  struct command *cmd;
  GError *err = NULL;

  if (path != NULL)
    {
      cmd = client_command_new (COMMAND_OP_FILESTAT, strlen (path)+1, path);

      client_send_command (client, cmd, &err);
    }
}

void
client_send_command_start (Client *client)
{
  struct command *cmd;
  GError *err = NULL;

  cmd = client_command_new (COMMAND_OP_START, 0, NULL);

  client_send_command (client, cmd, &err);

  if (err != NULL)
    {
      main_start_response_cb (FALSE, err->message);
      g_clear_error (&err);
    }
}

void
client_send_command_config (Client *client, gchar *options)
{
  struct command *cmd;
  GError *err = NULL;

  if (options)
    cmd = client_command_new (COMMAND_OP_CONFIG, strlen(options) + 1, options);
  else
    cmd = client_command_new (COMMAND_OP_CONFIG, 0, NULL);

  client_send_command (client, cmd, &err);

  if (err != NULL)
    {
      main_start_response_cb (FALSE, err->message);
      g_clear_error (&err);
    }
}

void
client_send_command_reset (Client *client)
{
  struct command *cmd;
  GError *err = NULL;

  cmd = client_command_new (COMMAND_OP_RESET, 0, NULL);

  client_send_command (client, cmd, &err);

  if (err != NULL)
    {
      main_start_response_cb (FALSE, err->message);
      g_clear_error (&err);
    }
}



void
client_send_command_stop (Client *client)
{
  struct command *cmd;
  GError *err = NULL;

  cmd = client_command_new (COMMAND_OP_STOP, 0, NULL);

  client_send_command (client, cmd, &err);

  if (err != NULL)
    {
      main_stop_response_cb (FALSE, err->message);
      g_clear_error (&err);
    }

}

void
client_send_command_status (Client *client)
{
  struct command *cmd;
  GError *err = NULL;

  cmd = client_command_new (COMMAND_OP_STATUS, 0, NULL);

  client_send_command (client, cmd, &err);

  if (err != NULL)
    {
      main_status_response_cb (FALSE, err->message, 0);
      g_clear_error (&err);
    }
}

Client *
client_new (void)
{
  Client *client = NULL;
  client = g_new0 (Client,1);

  client->cur_cmd = NULL;
  client->connection = NULL;
  client->cur_response = NULL;

  return client;
}
