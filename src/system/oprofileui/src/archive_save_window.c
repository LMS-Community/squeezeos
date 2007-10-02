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

static GladeXML *xml = NULL;
static GtkProgressBar *progress_bar = NULL;
static GtkWindow *window = NULL;
static GtkLabel *state_label;

static gboolean
on_archive_save_window_delete_event (GtkWidget *widget, GdkEvent  *event, 
    gpointer data)
{
    return TRUE;
}

void
archive_save_window_setup (GladeXML *xml_in)
{
  xml = xml_in;
  
  glade_xml_signal_connect_data (xml, "on_archive_save_window_delete_event",
      G_CALLBACK(on_archive_save_window_delete_event), NULL);
}

void
archive_save_window_show ()
{
  window = GTK_WINDOW (glade_xml_get_widget (xml, "archive_save_window"));
  progress_bar = GTK_PROGRESS_BAR (glade_xml_get_widget (xml, "save_progress"));
  state_label = GTK_LABEL (glade_xml_get_widget (xml, "label_save_state"));
  gtk_widget_show_all (GTK_WIDGET (window));
}

void
archive_save_window_finished ()
{
  gtk_widget_hide_all (GTK_WIDGET (window));
}

void
archive_save_window_progress (gulong index, gulong count)
{
  gdouble fraction = 0.0;
  gchar *description;

  if (count != 0 && index !=0)
    {
      gchar *progress_text;
      fraction = (double)index/(double)count;

      gtk_progress_bar_set_fraction (progress_bar, fraction);

      progress_text = g_strdup_printf ("Copying file %lu of %lu", index, count);
      gtk_progress_bar_set_text (progress_bar, progress_text);
      description = g_strdup_printf ("Copying files (%lu in total)", count);
    } else {
      description = g_strdup ("Preparing to copy files");
    }

    gtk_label_set_label (state_label, g_strdup_printf ("<i>%s</i>", description));
}
