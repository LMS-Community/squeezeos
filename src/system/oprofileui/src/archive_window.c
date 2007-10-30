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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>

#include "response.h"
#include "command.h"

#include "archive.h"
#include "archive_window.h"

static GladeXML *xml;
static guint total_number_of_files;
static guint number_downloaded;

static gboolean
on_archive_window_delete_event (GtkWidget *widget, GdkEvent  *event, gpointer data)
{
    return TRUE;
}

void
archive_window_set_status_label (gchar *str)
{
  static GtkLabel *label = NULL;

  if (label == NULL)
    {
      label = GTK_LABEL (glade_xml_get_widget (xml, "label_archive_state"));
    }

  gtk_label_set_label (label, str);
}

void
archive_window_show (GladeXML *xml_in)
{
  GtkWindow *archive_window;
  static GtkProgressBar *archive_progress = NULL;

  xml = xml_in;
  archive_window = GTK_WINDOW (glade_xml_get_widget (xml, "archive_window"));

  glade_xml_signal_connect_data (xml, "on_archive_window_delete_event",
      G_CALLBACK(on_archive_window_delete_event), NULL);

  archive_window_set_status_label ("<i>Downloading list of files</i>");

  gtk_widget_show_all (GTK_WIDGET(archive_window));

  if (archive_progress == NULL)
    {
      archive_progress = GTK_PROGRESS_BAR (glade_xml_get_widget (xml, "archive_progress"));
    }

  gtk_progress_bar_set_fraction (archive_progress, 0);
  gtk_progress_bar_set_text (archive_progress, "");
}

void
archive_window_file_list_finished (guint count)
{
  gchar *label_string;

  label_string = g_strdup_printf("<i>Downloading files (%d in total)</i>", count);
  archive_window_set_status_label (label_string);

  total_number_of_files = count;
  number_downloaded = 0;
}

void
archive_window_file_get_started (gchar *filename)
{

  static GtkProgressBar *archive_progress = NULL;
  gchar *progress_text;
 
  if (archive_progress == NULL)
    {
      archive_progress = GTK_PROGRESS_BAR (glade_xml_get_widget (xml, "archive_progress"));
    }
  progress_text = g_strdup_printf ("Downloading file %d of %d", number_downloaded+1,total_number_of_files);
  gtk_progress_bar_set_text (archive_progress, progress_text);
}

void
archive_window_file_get_finished ()
{
  static GtkProgressBar *archive_progress = NULL;
  double progress_fraction;

  number_downloaded++;


  progress_fraction = (double)number_downloaded/(double)total_number_of_files;

  if (archive_progress == NULL)
    {
      archive_progress = GTK_PROGRESS_BAR (glade_xml_get_widget (xml, "archive_progress"));
    }

  gtk_progress_bar_set_fraction (archive_progress, progress_fraction);

}

void
archive_window_finished ()
{
  GtkWindow *archive_window;

  archive_window = GTK_WINDOW (glade_xml_get_widget (xml, "archive_window"));
  gtk_widget_hide_all (GTK_WIDGET (archive_window));
}
