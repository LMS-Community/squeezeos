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
#include <time.h>
#include <stdlib.h>
#include <utime.h>

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libgnomevfs/gnome-vfs.h>

#include "oprofileui.h"
#include "response.h"
#include "command.h"
#include "client.h"
#include "archive_window.h"
#include "archive_save_window.h"
#include "main.h"
#include "archive.h"

/* The current file downloading */
static gchar *archive_path;
static gchar *archive_file;
static uint64_t archive_mtime;

static gchar *archive_basepath;
static GSList *archive_files;
static guint archive_count;
static gboolean already_saved = FALSE;
static gboolean user_specified = FALSE; /* Whether the current path is user specified */

static void archive_copyfile (gchar *src, gchar *dest)
{
  int ret;

  /* Initially try to hardlink the file */
  g_mkdir_with_parents (g_path_get_dirname (dest), 00700);
  g_remove (dest);
  ret = link (src, dest);
  if (ret < 0)
    {
      printf ("link failed %d (%s to %s)", ret, src, dest);

      /* Use gnomevfs to copy the file as a fallback */
      GnomeVFSURI *src_uri, *dst_uri;
      GnomeVFSResult res;

      src_uri = gnome_vfs_uri_new (gnome_vfs_get_uri_from_local_path(src));
      dst_uri = gnome_vfs_uri_new (gnome_vfs_get_uri_from_local_path(dest));
      res = gnome_vfs_xfer_uri (src_uri, dst_uri, 0, 
          GNOME_VFS_XFER_ERROR_MODE_ABORT,
          GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
          NULL, NULL);
      if (res != GNOME_VFS_OK)
        {
          const gchar *err_string = gnome_vfs_result_to_string (res);

          printf ("GNOME-VFS: error %s (%s to %s)", err_string, src, dest);
        }
    }
}

static void archive_removedir(gchar *path)
{
  GnomeVFSResult res;
  GnomeVFSURI *src_uri;
  GList uri_list;

  if (path == NULL)
    return;

  gnome_vfs_init ();

  src_uri = gnome_vfs_uri_new (gnome_vfs_get_uri_from_local_path(path));

  uri_list.data = src_uri;
  uri_list.next = NULL;
  uri_list.prev = NULL;

  res = gnome_vfs_xfer_delete_list (&uri_list,
      GNOME_VFS_XFER_ERROR_MODE_ABORT,
      GNOME_VFS_XFER_EMPTY_DIRECTORIES,
      NULL, NULL);

  if (res != GNOME_VFS_OK)
    {
      const gchar *err_string = gnome_vfs_result_to_string (res);

      printf ("GNOME-VFS: error %s\n", err_string);
    }
}

gchar *
archive_get_archive_basepath ()
{
  static const gchar *tmpdir;

  if (archive_basepath)
    return archive_basepath;

  if (tmpdir == NULL)
    {
      tmpdir = g_getenv("TMPDIR");
      if (tmpdir == NULL)
        {
	  tmpdir = "/tmp";
	}
    }

  /* TODO: Add hostname/port to path here */
  archive_basepath = g_strdup_printf ("%s/oprofile-archive-%d", tmpdir, getpid());

  return archive_basepath;
}

void
archive_clear_archive_basepath (void)
{
  g_free (archive_basepath);
  archive_basepath = NULL;
}

gchar *
archive_get_archive_path ()
{
  if (archive_path)
    return archive_path;

  archive_path = g_strdup_printf ("%s/active", archive_get_archive_basepath());
  user_specified = FALSE;

  return archive_path;
}

void
archive_set_archive_path (gchar *path)
{
  g_free (archive_path);

  if (path == NULL)
    archive_path = NULL;
  else
    archive_path = g_strdup (path);

  user_specified = TRUE;
}

void
archive_fully_downloaded ()
{
  archive_window_finished ();
  g_slist_free (archive_files);
  archive_files=NULL;

  main_archive_finished_cb (TRUE, NULL);
}

void
archive_get_next_file (Client *client)
{
  static GSList *cur_entry = NULL;

  if (cur_entry == NULL)
    {
      cur_entry = archive_files;
    }
  else if (g_slist_next (cur_entry) == NULL)
    {
      /* Important reset the cur_entry pointer */
      cur_entry = NULL;
      archive_fully_downloaded ();
      return;
    } 
  else
    {
      cur_entry = g_slist_next (cur_entry);
    }

  /* Fire off a request to download the next file */
  archive_file = cur_entry->data;
  archive_window_file_get_started (archive_file);

  client_send_command_filestat (client, archive_file);

  /* Yield to gtk. */
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

void
archive_handle_response (Client *client, struct response *reply)
{
  gchar **tmp;
  int i;

  gnome_vfs_init ();

  tmp = g_strsplit (reply->payload, "\n", 0);

  for (i=0; i < g_strv_length (tmp); i++)
    {
      gchar *str = g_strdup(tmp[i]);

      g_strstrip (str);

      if (!g_str_equal (str, ""))
        archive_files = g_slist_append (archive_files, str);
    }

  archive_count = g_slist_length (archive_files);
  archive_window_file_list_finished (archive_count); 

  /* Bunch of initialisation stuff */

  /* Delete the old archive */
  archive_cleanup ();

  already_saved = FALSE;

  archive_set_archive_path (NULL);
  archive_get_next_file (client);
  g_strfreev (tmp);
}

void
archive_file_got (Client *client)
{
  struct utimbuf utime_buf;
  struct stat stat_details;
  gchar *src, *dest;
  int ret;

  archive_window_file_get_finished ();

  src = g_strdup_printf("%s%s", archive_get_archive_path(), archive_file);
  ret = stat (src, &stat_details);

  /* Only proceed if the file exists */
  if (ret >= 0)
    {
      /* Set the mtime to match the original file */
      utime_buf.actime = archive_mtime;
      utime_buf.modtime = archive_mtime;
      utime (src, &utime_buf);

      /* Try to hardlink a cache copy of the downloaded file */
      dest = g_strdup_printf("%s%s", archive_get_archive_basepath(), archive_file);
      archive_copyfile(src, dest);
      utime (dest, &utime_buf);
      g_free(dest);
    }
 
  g_free(src);

  archive_get_next_file (client);
}

void archive_filestat_got (Client *client, uint64_t mtime, uint64_t size)
{
  struct stat stat_details;
  gboolean needfile = TRUE, needcopy = TRUE;
  gchar *cache, *dest;
  int ret;

  cache = g_strdup_printf("%s%s", archive_get_archive_basepath(), archive_file);
  archive_mtime = mtime;

  /* Do we need to download the file? */

  /* Can we use the binary stash? */
  if (strncmp(archive_file, "/var/lib/oprofile", strlen("/var/lib/oprofile")))
    {
      if (opui_config->localhost)
        {
          needfile = FALSE;
          needcopy = FALSE;
        }
      else
        {
          gchar *cache2 = g_strdup_printf("%s%s", opui_config->binaries_location, archive_file);
          ret = stat (cache2, &stat_details);
          if ((ret >= 0) && (stat_details.st_mtime == mtime) && (stat_details.st_size == size))
            {
              needfile = FALSE;
              needcopy = FALSE;
            }
          g_free(cache2);      
        }
    }

  /* Can we use our cache? */
  if (needfile)
    {
      ret = stat (cache, &stat_details);
      if ((ret >= 0) && (stat_details.st_mtime == mtime) && (stat_details.st_size == size))
        {
          needfile = FALSE;
        }
    }

  /* Act accordingly */
  if (needfile)
    {
      client_send_command_get (client, archive_file);
    }
  else
    {
      if (needcopy)
        {
          dest = g_strdup_printf("%s%s", archive_get_archive_path(), archive_file);
          archive_copyfile(cache, dest);
          g_free(dest);
        }
      archive_window_file_get_finished ();
      archive_get_next_file (client);
    }

  g_free(cache);
}

gint
archive_save_progress_cb (GnomeVFSXferProgressInfo *info, gpointer user_data)
{
  archive_save_window_progress (info->file_index, info->files_total);

  /* Yield to gtk. */
  while (gtk_events_pending ())
    gtk_main_iteration ();
  
  return 1;
}

void
archive_save (gchar *path)
{
  GnomeVFSResult res;
  GnomeVFSURI *src_uri;
  GnomeVFSURI *dst_uri;

  gchar *archive_path = archive_get_archive_path ();

  gnome_vfs_init ();

  archive_save_window_show ();

  dst_uri = gnome_vfs_uri_new (gnome_vfs_get_uri_from_local_path(path));
  src_uri = gnome_vfs_uri_new (gnome_vfs_get_uri_from_local_path(archive_path));


  if (already_saved)
    {
      /* If we have saved before then we copy it */
      res = gnome_vfs_xfer_uri (src_uri, dst_uri, 
          GNOME_VFS_XFER_RECURSIVE, 
          GNOME_VFS_XFER_ERROR_MODE_ABORT,
          GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
          archive_save_progress_cb, NULL);
    } else {
      /* Otherwise we can move it as an optimisation */
      res = gnome_vfs_xfer_uri (src_uri, dst_uri, 
          GNOME_VFS_XFER_RECURSIVE | GNOME_VFS_XFER_REMOVESOURCE, 
          GNOME_VFS_XFER_ERROR_MODE_ABORT,
          GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
          archive_save_progress_cb, NULL);
    }

  /* 
   * TODO: Add in files from binaries_location or the cache from a 
   * g_list created in the functions above 
   */

  if (res != GNOME_VFS_OK)
    {
      const gchar *err_string = gnome_vfs_result_to_string (res);

      printf ("GNOME-VFS: error %s", err_string);
    }

  archive_save_window_finished ();

  /* Set the new archive path */
  archive_set_archive_path (path);

  /* Mark that we have done one save already */
  already_saved = TRUE;
}

void
archive_load (gchar *path)
{
  /* Cleanup the old archive */
  archive_cleanup ();

  archive_set_archive_path (path);
}

void
archive_cleanup ()
{
  if (archive_path == NULL)
    return;

  if (user_specified)
    return;

  archive_removedir (archive_path);
  archive_set_archive_path (NULL);
}

void
archive_full_cleanup ()
{
  archive_cleanup ();

  if (archive_basepath == NULL)
    return;

  archive_removedir (archive_basepath);
  archive_clear_archive_basepath ();
}
