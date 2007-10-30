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

#include <config.h>
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
#include <assert.h>

#include "util.h"
#include "command.h"
#include "response.h"

#if WITH_AVAHI
#include "mdns.h"
#endif

/* Represents whether we are profiling or not */
static gboolean profiling = FALSE;

/* Represents whether we've been configured */
static gboolean configured = FALSE;

/* For the GOption parsing */
static gint port = 4224;
static gboolean verbose = FALSE;

static GOptionEntry entries[] = 
{
  { "port", 'p', 0, G_OPTION_ARG_INT, &port, 
    "Listen on <port> for connections. Default: 4224.", "<port>"},
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Enable verbose output",
    NULL},
  { NULL }
};

/* Setup the socket to listen on */
static int
server_setup_socket (int port)
{
  struct sockaddr_in addr;
  int server_fd = 0;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  server_fd = socket (AF_INET,SOCK_STREAM, 0);

  if (server_fd < 0)
    {
      const gchar *error = g_strerror (errno);
      g_error ("Could not open server socket: %s", error);
    }

  if (bind (server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      const gchar *error = g_strerror (errno);
      g_error ("Could not bind the server socket: %s", error);
    }

  if (listen (server_fd,1) < 0)
    {
      const gchar *error = g_strerror (errno);
      g_error ("Could not listen on the socket: %s", error);
    }

  return server_fd;
}

/* Setup the local domain socket to listen on */
static int
server_setup_socket_local ()
{
  struct sockaddr_un addr;
  int addr_len;
  int server_fd = 0;

  /* Remove any existing socket */
  unlink(OPROFILEUI_LOCAL_SOCKET);

  memset ((char *) &addr, sizeof(addr), 0);
  addr.sun_family = AF_UNIX;
  strcpy (addr.sun_path, OPROFILEUI_LOCAL_SOCKET);
  addr_len = strlen(addr.sun_path) + sizeof(addr.sun_family);

  server_fd = socket (AF_UNIX, SOCK_STREAM, 0);

  if (server_fd < 0)
    {
      const gchar *error = g_strerror (errno);
      g_error ("Could not open server domain socket: %s", error);
    }

  if (bind (server_fd, (struct sockaddr *)&addr, addr_len) < 0)
    {
      const gchar *error = g_strerror (errno);
      g_error ("Could not bind the server domain socket: %s", error);
    }

  if (listen (server_fd, 1) < 0)
    {
      const gchar *error = g_strerror (errno);
      g_error ("Could not listen on the socket: %s", error);
    }

  chmod (OPROFILEUI_LOCAL_SOCKET, 0777);

  return server_fd;
}


/* 
 * For a given opcode create a new response and allocate the appropriate
 * amount of memory for it.
 */
static struct response *
server_response_new (char opcode, size_t length, void *payload)
{
  struct response *buf;

  buf = g_malloc0 (sizeof (struct response) + length);

  buf->opcode = opcode;
  buf->length = length;

  if (length > 0)
    {
      assert (payload != NULL);
      memcpy(&(buf->payload[0]), payload, length);
    }

  return buf;
}

/* Generate the response for commands that don't need the output of stdout */
static struct response *
server_do_command_wrapper (gchar **args)
{
  gchar *stdout = NULL;
  gchar *stderr = NULL;
  gint exit_status;
  GError *err = NULL;
  gboolean success;

  struct response *reply;

  success = g_spawn_sync (
      NULL, /* working directory, inherit because NULL */
      args, /* argv[0] is program to run */

      NULL, /* environment, inherit because NULL */
      G_SPAWN_SEARCH_PATH, /* search for executable, no need for full path */
      NULL, /* child setup func */
      NULL, /* user_data for child setup func */
      &stdout,
      &stderr,
      &exit_status,
      &err);

  if (success)
    {
      /* This is the UNIX exit code */
      int exit_code = WEXITSTATUS (exit_status);
      if (WIFEXITED (exit_status) && exit_code==0)
        {
          /* Create a positive reply */
          reply = server_response_new (RESPONSE_OP_OK, 0, NULL);
        } else {

          gchar *err_string = g_strdup_printf (
              "Error whilst spawning: opcontrol exited with error code: %d",
              exit_code);

          /* Create an error reply. NB: strlen (...) + 1 to include \0 */
          reply = server_response_new (RESPONSE_OP_ERROR, 
              strlen (err_string) + 1, err_string);

          g_warning ("Error whilst spawning: %s", err_string);

          g_free(err_string);
        }
    } else {
      gchar *err_string = g_strdup_printf ("Error whilst spawning: %s",
          err->message);

      /* Send an error reply. As above wrt strlen. */
      reply = server_response_new (RESPONSE_OP_ERROR, strlen(err_string) + 1,
          err_string);

      g_free(err_string);
    }

  printf (stderr);
  g_free (stdout);
  g_free (stderr);
  g_free (err);
  return reply;
}

static void
server_handle_command_start (struct command *buf, int active_fd)
{
  struct response *reply;
  const gchar *args[] = {"opcontrol", "--start", NULL};

  if (verbose)
    g_debug ("Command received: START");

  /* Use the generic wrapper for this command */
  reply = server_do_command_wrapper ((gchar **)args);

  /* Send out the reply */
  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  /* Update internal state */
  if (reply->opcode == RESPONSE_OP_OK)
    profiling = TRUE;

  g_free (reply);
}

static void
server_handle_command_config (struct command *buf, int active_fd)
{
  struct response *reply;
  gchar **args;
  const gchar *default_args[] = {"opcontrol", "--start-daemon",
    "--separate=lib","-c 6", "--no-vmlinux", NULL};

  /* Examine the payload */
  gchar *str = buf->payload;
  g_strstrip (str);

  if (verbose)
    g_debug ("Command received: CONFIG: %s", str);

  /* Test to see if any arguments are supplied. Otherwise use defaults */
  if (buf->length == 0 || strlen (str) == 0)
    {
      args = (gchar **)default_args;
    } else {
      /* The arguments are supplied as a string */
      gchar **parts = g_strsplit (str, " ", 0);

      int i = 0;

      /* 
       * Allocate enough space to hold the content of parts plus 2 more cells
       * to add to the front. We must add one more on here since g_strv_length
       * does not count the final NULL.
       */
      args = g_malloc0 ((g_strv_length (parts) + 3) * sizeof (gchar *));

      /* Allocate these onto the heap to make freeing easier later */
      args[0] = g_strdup ("opcontrol");
      args[1] = g_strdup ("--start-daemon");

      /* Copy the pointers to the arguments from parts into args */
      for (i=2; i < g_strv_length (parts) + 3; i++)
        {
          args[i] = parts[i-2];
        }

      /* This only frees the array itself. */
      g_free (parts);
    }

  /* Use the generic wrapper for this command */
  reply = server_do_command_wrapper (args);

  /* Send out the reply */
  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  /* Update internal state */
  if (reply->opcode == RESPONSE_OP_OK)
    configured = TRUE;

  g_free (reply);

  if (args != (gchar **)default_args)
    {
      /* 
       * We had custom arguments so we should free the array of pointers we
       * used. This frees all the strings and the array itself.
       */
      g_strfreev (args);
    }
}

static void
server_handle_command_stop (struct command *buf, int active_fd)
{
  struct response *reply;
  const gchar *args[] = {"opcontrol","--stop",NULL};

  if (verbose)
    g_debug ("Command received: STOP");

  /* Use the generic wrapper for this command */
  reply = server_do_command_wrapper ((gchar **)args);

  /* Send out the reply */
  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  /* Update internal state */
  if (reply->opcode == RESPONSE_OP_OK)
    profiling = FALSE;

  g_free (reply);
}

static void
server_handle_command_status (struct command *buf, int active_fd)
{
  struct response_status status; 
  struct response *reply;

  if (verbose)
    g_debug ("Command received: STATUS");

  status.proto_version = OPROFILEUI_PROTO_VERSION;
  status.profiling = profiling;
  status.configured = configured;

  reply = server_response_new (RESPONSE_OP_STATUS, sizeof(status), &status);

  /* Send out the reply */
  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  g_free (reply);
}

static void
server_handle_command_reset (struct command *buf, int active_fd)
{
  struct response *reply;
  const gchar *args[] = {"opcontrol","--reset",NULL};

  if (verbose)
    g_debug ("Command received: RESET");

  /* Use the generic wrapper for this command */
  reply = server_do_command_wrapper ((gchar **)args);

  /* Send out the reply */
  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  /* Update internal state */
  if (reply->opcode == RESPONSE_OP_OK)
    profiling = FALSE;

  g_free (reply);
}

/*
 * Like server_handle_command_wrapper except that the stdout is included as
 * the payload in the response.
 */
static void
server_handle_command_archive (struct command *buf, int active_fd)
{
  gchar *stdout = NULL;
  gchar *stderr = NULL;
  gint exit_status;
  GError *err = NULL;
  gboolean success;

  const gchar *args[] = {"oparchive", "--list-files", NULL};
  struct response *reply;

  success = g_spawn_sync (
      NULL, /* working directory, inherit because NULL */
      (gchar **)args, /* argv[0] is program to run */

      NULL, /* environment, inherit because NULL */
      G_SPAWN_SEARCH_PATH, /* search for executable, no need for full path */
      NULL, /* child setup func */
      NULL, /* user_data for child setup func */
      &stdout,
      &stderr,
      &exit_status,
      &err);

  if (success)
    {

      /* This is the UNIX exit code */
      int exit_code = WEXITSTATUS (exit_status);
      if (WIFEXITED (exit_status) && exit_code==0)
        {
          /* Create a positive reply, and include the stdout as the payload */
          reply = server_response_new (RESPONSE_OP_ARCHIVE, strlen (stdout) + 1,
              stdout);

        } else {
          gchar *err_string;

          if (strstr (stderr,"No sample file found") != NULL)
            {
              err_string = g_strdup_printf ("No sample data available to download.");
            } else {
              err_string = g_strdup_printf ("oparchive reported error code: %d", exit_code);
            }

          /* Create an error response */
          reply = server_response_new (RESPONSE_OP_ERROR,
              strlen (err_string) + 1, err_string);

          g_free(err_string);
        }
    } else {

      gchar *err_string = g_strdup_printf ("Error whilst spawning: %s",
          err->message);

      /* Create an error response */
      reply = server_response_new (RESPONSE_OP_ERROR, strlen (err_string) + 1,
          err_string);

      g_free(err_string);
    }
  
  printf (stderr);

  g_free (stdout);
  g_free (stderr);
  g_free (err);

  /* Send the reply */
  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  g_free (reply);
}


/* 
 * Open the file pointed to by buf and return an open file descriptor
 * and stat information. Return an error response for invalid paths.
 */
static int
server_parse_open_file (struct command *buf, int active_fd, 
			struct stat *stat_details)
{
  int fd;
  gchar *path = NULL;
  struct response *reply;

  /* Prepare the path to fetch */
  path = buf->payload;
  g_strstrip (path);

  /* Check path is valid */
  if (buf->length == 0 || strlen (path) == 0)
    {
      g_warning ("ERROR: Filename absent for GET command");
      return -1;
    }

  fd = open (path, O_RDONLY);

  if (fd < 0)
    {
      /* Something went wrong when trying to open the file */
      const gchar *err_string = g_strerror (errno);

      /* Generate an error response */
      reply = server_response_new (RESPONSE_OP_ERROR, strlen (err_string) + 1,
          (gchar *)err_string);

      /* Send the error response */
      util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

      g_free (reply);
      return fd;
    }

  fstat (fd, stat_details);

  return fd;
}


static void
server_handle_command_get (struct command *buf, int active_fd)
{
  int fd;
  off_t size;
  struct stat stat_details;
  struct response *reply;

  if (verbose)
    g_debug ("Command recieved: GET: %s", buf->payload);

  fd = server_parse_open_file (buf, active_fd, &stat_details);
  if (fd < 0)
    return;

  /* Get the size of the file */
  size = stat_details.st_size;

  /* 
   * Create a response. We have to do it manually since we want to set the
   * length but not allocate any memory because we are going to sendfile ()
   * later.
   */
  reply = g_malloc0 (sizeof (struct response));

  reply->opcode = RESPONSE_OP_FILE;
  reply->length = size;

  /* Send the response (header) */
  util_write_in_full (active_fd, (char *)reply, sizeof (struct response));

  /* Use sendfile to stream the file to the client */
  util_sendfile_in_full (active_fd, fd, size);

  close (fd);

  g_free (reply);
}

static void
server_handle_command_filestat (struct command *buf, int active_fd)
{
  struct stat stat_details;
  struct response_filestat filestat;
  struct response *reply;
  int fd;

  if (verbose)
    g_debug ("Command recieved: FILESTAT: %s", buf->payload);

  fd = server_parse_open_file (buf, active_fd, &stat_details);

  if (fd < 0)
    return;

  close (fd);

  filestat.mtime = stat_details.st_mtime;
  filestat.size = stat_details.st_size;

  reply = server_response_new (RESPONSE_OP_FILESTAT, sizeof(filestat), &filestat);

  util_write_in_full (active_fd, (char *)reply, SIZE_OF_RESPONSE (reply));

  g_free (reply);
}

static void
server_handle_command_unknown (struct command *buf, int active_fd)
{
  if (verbose)
    g_debug ("UNKNOWN");
}


/* Demultiplex the command to figure out what to do */
static void
server_handle_command (struct command *buf, int active_fd)
{
  switch (buf->opcode)
  {
    case COMMAND_OP_START:
      server_handle_command_start (buf, active_fd);
      break;
    case COMMAND_OP_STOP:
      server_handle_command_stop (buf, active_fd);
      break;
    case COMMAND_OP_ARCHIVE:
      server_handle_command_archive (buf, active_fd);
      break;
    case COMMAND_OP_GET:
      server_handle_command_get (buf, active_fd);
      break;
    case COMMAND_OP_STATUS:
      server_handle_command_status (buf, active_fd);
      break;
    case COMMAND_OP_RESET:
      server_handle_command_reset (buf, active_fd);
      break;
    case COMMAND_OP_FILESTAT:
      server_handle_command_filestat (buf, active_fd);
      break;
    case COMMAND_OP_CONFIG:
      server_handle_command_config (buf, active_fd);
      break;
    default:
      server_handle_command_unknown (buf, active_fd);
  }
}

static gboolean
server_handle_connection (GIOChannel *source, GIOCondition condition, gpointer user_data)
{
  int active_fd;
  int res;
  struct command *buf;

  if (condition & G_IO_ERR) {
    g_warning (g_strerror (errno));
    return FALSE;
  }

  active_fd = accept (g_io_channel_unix_get_fd (source), NULL, 0);
  do
    {
      /* Allocate just enough for the header of the command */
      buf = g_malloc0 (sizeof (struct command));

      /* Read in the header of the command */
      res = read (active_fd, buf, sizeof (struct command));

      if (res > 0)
        {
          /* 
           * Provided that was successful check if there is a payload to read
           * in.
           */
          if (buf->length > 0)
            {
              /* Increase the size of the buffer if necessary */
              buf = g_realloc (buf, sizeof (struct command) + buf->length);

              /* Read in the payload */
              res = util_read_in_full (active_fd, &(buf->payload[0]),
                  buf->length);
            }

          /* If we have a complete response */
          if (res >= 0)
            {
              server_handle_command (buf, active_fd);
            }
        }

      g_free (buf);
    }
  while (res > 0);

  if (res < 0)
  {
      g_warning ("Error whilst reading on socket: %s", g_strerror (errno));
  }

  return TRUE;
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  GMainLoop *loop;
  GIOChannel *channel, *local_channel;

  context = g_option_context_new ("- OProfile control server");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    /* Should catch G_OPTION_ERROR_UNKNOWN_OPTION and display the help */
    g_error ("Cannot parse arguments: %s", error->message);
  }
  g_option_context_free (context);

  /* Check for root */
  if (geteuid() != 0)
    {
      printf ("The server needs to be run with root privileges\n");
      return 1;
    }

  loop = g_main_loop_new (NULL, TRUE);

  /* Add internet socket */
  channel = g_io_channel_unix_new (server_setup_socket (port));
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_add_watch (channel, G_IO_IN|G_IO_ERR, server_handle_connection, NULL);

  /* Add domain socket */
  local_channel = g_io_channel_unix_new (server_setup_socket_local ());
  g_io_channel_set_close_on_unref (local_channel, TRUE);
  g_io_add_watch (local_channel, G_IO_IN|G_IO_ERR, server_handle_connection, NULL);

#if WITH_AVAHI
  mdns_publish (port);
#endif
  g_io_channel_unref (channel);
  g_io_channel_unref (local_channel);

  if (verbose)
    g_debug ("Starting OProfile Server " VERSION);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  /* local_channel cleanup */
  unlink(OPROFILEUI_LOCAL_SOCKET);

  return 0;
}
