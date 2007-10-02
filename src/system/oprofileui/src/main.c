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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

#include "response.h"
#include "command.h"

#include "oprofileui.h"
#include "archive.h"
#include "archive_save_window.h"
#include "archive_window.h"

#include "view.h"
#include "report.h"
#include "client.h"
#include "main.h"

#if WITH_AVAHI
#include "avahi-ui.h"
#endif

static GladeXML *xml;

static enum oprofileui_state current_state = OPUI_INIT;

GtkWidget *main_window;

static Client *client;
static View *view;

struct oprofileui_config *opui_config;

#define SOURCE_GLADE "data/oprofile-viewer.glade"
#define INSTALLED_GLADE PKG_DATA_DIR"/oprofile-viewer.glade"

#define OPROFILEUI_GCONF_ROOT "/apps/oprofileui-viewer"
#define OPROFILEUI_GCONF_PREFS OPROFILEUI_GCONF_ROOT "/prefs"

#define OPROFILEUI_OPCONTROL_PARAMS_DEFAULT "--separate=lib -c 6"

gboolean is_connected()
{
  switch(current_state)
    {
      default:
      case OPUI_INIT:
      case OPUI_DISCONNECTING:
      case OPUI_DISCONNECTED:
      case OPUI_CONNECTING:
        return FALSE;
      case OPUI_CONNECTED_WAIT:
      case OPUI_CONNECTED_IDLE:
      case OPUI_CONNECTED_RUNNING:
        return TRUE;
    }
}

void
on_stop_clicked (GtkWidget *widget, gpointer user_data)
{
  main_set_state(OPUI_CONNECTED_WAIT);
  client_send_command_stop (client);

}

void
on_reset_clicked (GtkWidget *widget, gpointer user_data)
{
  main_set_state(OPUI_CONNECTED_WAIT);
  client_send_command_reset (client);
}

void
on_start_clicked (GtkWidget *widget, gpointer user_data)
{
  main_set_state(OPUI_CONNECTED_WAIT);
  client_send_command_start (client);
}

void
on_download_clicked (GtkWidget *widget, gpointer user_data)
{
  main_set_state(OPUI_CONNECTED_WAIT);
  client_send_command_archive (client);
}

void
main_generic_error (gchar *err_message)
{
  GtkWidget *dialog;
  guint res;

  g_assert (err_message != NULL);

  dialog = gtk_message_dialog_new (GTK_WINDOW(main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, 
      GTK_BUTTONS_CLOSE, "%s", err_message);

  res = gtk_dialog_run (GTK_DIALOG(dialog));

  gtk_widget_destroy (dialog);
}

/*
 * This is fired by the client when a response from the start command has been
 * processed. It allows the updating of the state/display of error messages.
 */
void
main_start_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
    {
      main_generic_error (err_message);
      main_set_state(OPUI_CONNECTED_IDLE);
    }
  else
    {
      main_set_state(OPUI_CONNECTED_RUNNING);
    }
}

void
main_stop_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
    {
      main_generic_error (err_message);
    } else {

      /* Start the archive retrieval process */
      archive_window_show (xml);
      client_send_command_archive (client);
    }
}

void
main_reset_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
    main_generic_error (err_message);

  client_send_command_status (client);
}

void
main_archive_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
  {
    main_generic_error (err_message);
    archive_window_finished ();
    client_send_command_status (client);
  } else {
    /* Start the archive retrieval process */
    archive_window_show (xml);
  }
}

void
main_archive_finished_cb (gboolean success, gchar *err_message)
{
  Report *report = NULL;

  GtkWidget *toolbar_save;
  GtkWidget *menu_save;

  GtkWidget *menu_group_by_application;
  GtkWidget *menu_group_by_module;

  GtkWidget *filter_hbox;

  gchar *archive_path;

  if (!success)
    main_generic_error (err_message);

  /* Generate the report and display it */

  archive_path = archive_get_archive_path ();
  report = report_new_from_archive_path (archive_path);
  g_object_set (view, "report", report, NULL);
  g_object_unref (report);
  view_set_display_mode (view, opui_config->group_by_application, opui_config->group_by_module);

  client_send_command_status (client);

  /* Turn on the save toolbar button */
  toolbar_save = glade_xml_get_widget (xml, "toolbar_save");
  gtk_widget_set_sensitive (toolbar_save, TRUE);

  /* Turn on the save menu item */
  menu_save = glade_xml_get_widget (xml, "menu_save");
  gtk_widget_set_sensitive (menu_save, TRUE);

  /* Turn on view menu options */
  menu_group_by_application = glade_xml_get_widget (xml, "menu_group_by_application");
  menu_group_by_module = glade_xml_get_widget (xml, "menu_group_by_module");

  gtk_widget_set_sensitive (menu_group_by_application, TRUE);
  gtk_widget_set_sensitive (menu_group_by_module, TRUE);

  /* Turn on the filter box */
  filter_hbox = glade_xml_get_widget (xml, "filter_hbox");
  gtk_widget_set_sensitive (filter_hbox, TRUE);
}

void
main_connect_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
    {
      main_generic_error (err_message);
      main_set_state(OPUI_DISCONNECTED);
    } else {
      client_send_command_status (client);
      main_set_state(OPUI_CONNECTED_WAIT);
    }
}

void
main_disconnect_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
    main_generic_error (err_message);

  main_set_state(OPUI_DISCONNECTED);
}

void
main_config_response_cb (gboolean success, gchar *err_message)
{
  if (!success)
    main_generic_error (err_message);

  client_send_command_status (client);
  main_set_state(OPUI_CONNECTED_WAIT);
}

void
main_send_command_config (Client *client)
{
  gchar *options;

  if (opui_config->vmlinux_location && (strlen (opui_config->vmlinux_location) > 0))
    options = g_strdup_printf ("%s --vmlinux=%s", opui_config->opcontrol_params, 
                               opui_config->vmlinux_location);
  else
    options = g_strdup_printf ("%s --no-vmlinux", opui_config->opcontrol_params);

  client_send_command_config (client, options);

  g_free(options);
}

void
main_status_response_cb (gboolean success, gchar *err_message, 
                         struct response_status *status)
{
  if (!success)
    {
      main_set_state (OPUI_DISCONNECTING);
      client_disconnect (client);
      main_generic_error (err_message);
    }
   else
    {
      if (!status->configured)
        {
          main_send_command_config (client);
        }
      else if (status->profiling)
        {
          main_set_state(OPUI_CONNECTED_RUNNING);
        }
      else 
        {
          main_set_state(OPUI_CONNECTED_IDLE);
        }
    }
}

void
main_connection_lost_cb ()
{
  main_set_state (OPUI_DISCONNECTED);

  main_generic_error ("Connection to remote server lost.");
}

void 
on_save_clicked (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Choose location to save archive",
      GTK_WINDOW(main_window),
      GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *filename;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      
      archive_save_window_setup (xml);

      /* Yield to gtk. */
      while (gtk_events_pending ())
        gtk_main_iteration ();

      gtk_widget_destroy (dialog);

      archive_save (filename);
      g_free (filename);
    }
}

void 
on_open_clicked (GtkWidget *widget, gpointer user_data)
{
  Report *report = NULL;
  GtkWidget *dialog;
  gchar *archive_path;

  dialog = gtk_file_chooser_dialog_new ("Choose archive",
      GTK_WINDOW(main_window),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *filename;
      GtkWidget *menu_group_by_application;
      GtkWidget *menu_group_by_module;
      GtkWidget *filter_hbox;
      
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      archive_load (filename);
      g_free (filename);

      /* Generate the report and display it */
      archive_path = archive_get_archive_path ();
      report = report_new_from_archive_path (archive_path);
      g_object_set (view, "report", report, NULL);
      g_object_unref (report);
      view_set_display_mode (view, opui_config->group_by_application, opui_config->group_by_module);

      /* Turn on view menu options */
      menu_group_by_application = glade_xml_get_widget (xml, "menu_group_by_application");
      menu_group_by_module = glade_xml_get_widget (xml, "menu_group_by_module");

      gtk_widget_set_sensitive (menu_group_by_application, TRUE);
      gtk_widget_set_sensitive (menu_group_by_module, TRUE);

      /* Turn on the filter box */
      filter_hbox = glade_xml_get_widget (xml, "filter_hbox");
      gtk_widget_set_sensitive (filter_hbox, TRUE);
    }

  gtk_widget_destroy (dialog);
}

void
on_connect_dialog_localhost_toggled (GtkToggleButton *button, gpointer user_data)
{
  GtkWidget *dialog_host;

  gboolean localhost = gtk_toggle_button_get_active (button);

  dialog_host = glade_xml_get_widget (xml, "connect_dialog_host_alignment");
  gtk_widget_set_sensitive (dialog_host, !localhost);
}

void
on_connect_clicked (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *dialog;
  GtkWidget *connect_dialog_host;
  GtkWidget *connect_dialog_port;
  GtkWidget *browse_button;
  guint response;

  dialog = glade_xml_get_widget (xml, "connect_dialog");
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (main_window));

  browse_button = glade_xml_get_widget (xml, "browse_button");

  connect_dialog_host = glade_xml_get_widget (xml, "connect_dialog_host");
  connect_dialog_port = glade_xml_get_widget (xml, "connect_dialog_port");

#if ! WITH_AVAHI
  gtk_widget_hide (browse_button);
#endif

  /* For some reason these properties are not set by glade on subsequent runs of
   * the dialog, so we have to set them manually here */
  gtk_entry_set_activates_default (GTK_ENTRY (connect_dialog_host), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (connect_dialog_port), TRUE);

  response = gtk_dialog_run (GTK_DIALOG(dialog));

  if (response == GTK_RESPONSE_OK) {
      GtkWidget *connect_dialog_localhost;
      GConfClient *gconf_client;

      connect_dialog_localhost = glade_xml_get_widget (xml, "connect_dialog_localhost");

      /* Must g_strdup here since the string returned is internal */
      g_free(opui_config->host);
      opui_config->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (connect_dialog_host)));
      opui_config->port = atoi (gtk_entry_get_text (GTK_ENTRY (connect_dialog_port)));
      opui_config->localhost = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (connect_dialog_localhost));

      /* Save options */ 
      gconf_client = gconf_client_get_default();
      gconf_client_set_bool(gconf_client, OPROFILEUI_GCONF_PREFS "/remotehost", !opui_config->localhost, NULL);
      gconf_client_set_int(gconf_client, OPROFILEUI_GCONF_PREFS "/port", opui_config->port, NULL);
      gconf_client_set_string(gconf_client, OPROFILEUI_GCONF_PREFS "/host", opui_config->host, NULL);

      main_set_state (OPUI_CONNECTING);

      if (opui_config->localhost)
        client_connect (client, OPROFILEUI_LOCAL_SOCKET, -1);
      else
        client_connect (client, opui_config->host, opui_config->port);
  }
#if WITH_AVAHI
else if (response == 1) {
    GtkWidget *browser;
    gchar *hostname;
    guint port;
    
    browser = aui_service_dialog_new ("", GTK_WINDOW (main_window),
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT,
                                     NULL);
    
    aui_service_dialog_set_browse_service_types (AUI_SERVICE_DIALOG (browser), "_oprofile._tcp", NULL);
    
    if (gtk_dialog_run (GTK_DIALOG (browser)) == GTK_RESPONSE_ACCEPT) {
      hostname = g_strdup (aui_service_dialog_get_host_name (AUI_SERVICE_DIALOG (browser)));
      port = aui_service_dialog_get_port (AUI_SERVICE_DIALOG (browser));
      
      main_set_state (OPUI_CONNECTING);

      client_connect (client, hostname, port);

      g_free(hostname);
    }
    
    gtk_widget_destroy (browser);
  }
#endif
  
  gtk_widget_hide (dialog);
}

void
on_options_clicked (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *dialog;
  GtkWidget *options_dialog_opcontrol_params;
  GtkWidget *options_dialog_binaries_location;
  GtkWidget *options_dialog_vmlinux_location;
  GConfClient *gconf_client;

  options_dialog_opcontrol_params = glade_xml_get_widget (xml, "options_dialog_opcontrol_params");
  options_dialog_binaries_location = glade_xml_get_widget (xml, "options_dialog_binaries_location");
  options_dialog_vmlinux_location = glade_xml_get_widget (xml, "options_dialog_vmlinux_location");

  dialog = glade_xml_get_widget (xml, "options_dialog");
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (main_window));

  gtk_dialog_run (GTK_DIALOG(dialog));

  /* Save options */ 
  gconf_client = gconf_client_get_default();

  g_free(opui_config->opcontrol_params);
  g_free(opui_config->binaries_location);
  g_free(opui_config->vmlinux_location);
  /* Must g_strdup here since the string returned is internal */
  opui_config->opcontrol_params = g_strdup (gtk_entry_get_text (GTK_ENTRY (options_dialog_opcontrol_params)));
  opui_config->binaries_location = g_strdup (gtk_entry_get_text (GTK_ENTRY (options_dialog_binaries_location)));
  opui_config->vmlinux_location = g_strdup (gtk_entry_get_text (GTK_ENTRY (options_dialog_vmlinux_location)));

  /* Save options */ 
  gconf_client = gconf_client_get_default();
  gconf_client_set_string(gconf_client, OPROFILEUI_GCONF_PREFS "/opcontrol_params", opui_config->opcontrol_params, NULL);
  gconf_client_set_string(gconf_client, OPROFILEUI_GCONF_PREFS "/binaries_location", opui_config->binaries_location, NULL);
  gconf_client_set_string(gconf_client, OPROFILEUI_GCONF_PREFS "/vmlinux_location", opui_config->vmlinux_location, NULL);
  
  gtk_widget_hide (dialog);

  if (is_connected ())
    main_send_command_config (client);
}

void
on_disconnect_clicked (GtkWidget *widget, gpointer user_data)
{
  main_set_state (OPUI_DISCONNECTING);
  client_disconnect (client);
}

static gboolean
on_main_window_delete_event (GtkWidget *widget, GdkEvent  *event, gpointer data)
{
    return FALSE;
}

static void
on_main_window_destroy (GtkWidget *widget, gpointer data)
{
    gtk_main_quit ();
}

static void
on_menu_group_by_application_toggled (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  gboolean new_val = gtk_check_menu_item_get_active (menuitem);

  if (new_val != opui_config->group_by_application)
    {
      opui_config->group_by_application = new_val;
      view_set_display_mode (view, opui_config->group_by_application, opui_config->group_by_module);
    }
}

static void
on_menu_group_by_module_toggled (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  gboolean new_val = gtk_check_menu_item_get_active (menuitem);

  if (new_val != opui_config->group_by_module)
    {
      opui_config->group_by_module = new_val;
      view_set_display_mode (view, opui_config->group_by_application, opui_config->group_by_module);
    }
}

static void
on_menu_quit_activate (GtkMenuItem *menuitem, gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (main_window));
}

static void
on_menu_about_activate (GtkMenuItem *menuitem, gpointer user_data)
{
  const gchar *authors[] = {"Robert Bradford <rob@openedhand.com>", NULL};

  gtk_show_about_dialog (GTK_WINDOW (main_window), 
      "name", "Oprofile Viewer", 
      "version", PACKAGE_VERSION, 
      "authors", authors,
      "comments", "A user interface for OProfile",
      "copyright", "(C) 2007 Nokia Corporation",
      NULL);

}

static void
on_filter_entry_activate (GtkEntry *entry, gpointer user_data)
{
  gchar *filter_str = NULL;
  filter_str = (gchar *)gtk_entry_get_text (entry);

  if (filter_str != NULL)
    {
      g_strstrip (filter_str);

      if (strlen (filter_str) > 0)
        {
          g_object_set (view, "filter-string", filter_str, NULL);
        } else {
          g_object_set (view, "filter-string", NULL, NULL);
        }
    }
}

static void
on_filter_clear_button_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *entry;

  entry = glade_xml_get_widget (xml, "filter_entry");
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  g_object_set (view, "filter-string", NULL, NULL);
}

static void
main_set_widget_sensitive (const char *name, gboolean sensitive)
{
  GtkWidget *widget;

  widget = glade_xml_get_widget (xml, name);
  gtk_widget_set_sensitive (widget, sensitive);
}

static void
main_set_control_widgets_sensitive (gboolean connect, gboolean disconnect,
                                    gboolean start, gboolean stop,
                                    gboolean download, gboolean reset)
{
   main_set_widget_sensitive("toolbar_connect", connect);
   main_set_widget_sensitive("menu_connect", connect);

   main_set_widget_sensitive("toolbar_disconnect", disconnect);
   main_set_widget_sensitive("menu_disconnect", disconnect);

   main_set_widget_sensitive("toolbar_start", start);
   main_set_widget_sensitive("menu_start", start);

   main_set_widget_sensitive("toolbar_stop", stop);
   main_set_widget_sensitive("menu_stop", stop);

   main_set_widget_sensitive("toolbar_download", download);
   main_set_widget_sensitive("menu_download", download);

   main_set_widget_sensitive("toolbar_reset", reset);
   main_set_widget_sensitive("menu_reset", reset);
}

void
main_set_state (enum oprofileui_state new_state)
{
  GdkCursor *cursor = NULL;
  if (current_state == new_state)
    return;

  switch(new_state)
    {
      case OPUI_INIT:
      case OPUI_DISCONNECTED:
        main_set_control_widgets_sensitive (TRUE, FALSE, FALSE, FALSE, FALSE, FALSE);
        break;
      case OPUI_DISCONNECTING:
        cursor = gdk_cursor_new (GDK_WATCH);
        main_set_control_widgets_sensitive (FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
        break;
      case OPUI_CONNECTING:
        cursor = gdk_cursor_new (GDK_WATCH);
        main_set_control_widgets_sensitive (FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
        break;
      case OPUI_CONNECTED_WAIT:
        cursor = gdk_cursor_new (GDK_WATCH);
        main_set_control_widgets_sensitive (FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
        break;
      case OPUI_CONNECTED_IDLE:
        main_set_control_widgets_sensitive (FALSE, TRUE, TRUE, FALSE, TRUE, TRUE);
        break;
      case OPUI_CONNECTED_RUNNING:
        main_set_control_widgets_sensitive (FALSE, TRUE, FALSE, TRUE, TRUE, TRUE);
        break;
    }

  current_state = new_state;
  
  gdk_window_set_cursor (main_window->window, cursor);

  if (cursor)
    gdk_cursor_unref (cursor);
}

static void
main_load_default_config (void)
{
  GtkWidget *connect_dialog_host;
  GtkWidget *connect_dialog_port;
  GtkWidget *connect_dialog_localhost;
  GtkWidget *connect_dialog_remotehost;
  GtkWidget *options_dialog_opcontrol_params;
  GtkWidget *options_dialog_binaries_location;
  GtkWidget *options_dialog_vmlinux_location;

  GConfClient *client;

  client = gconf_client_get_default();

  /* Load default configuration values */
  opui_config->group_by_application = TRUE;
  opui_config->group_by_module = TRUE;
  opui_config->localhost = !gconf_client_get_bool(client, OPROFILEUI_GCONF_PREFS "/remotehost", NULL);
  opui_config->port = gconf_client_get_int(client, OPROFILEUI_GCONF_PREFS "/port", NULL);
  opui_config->host = gconf_client_get_string(client, OPROFILEUI_GCONF_PREFS "/host", NULL);
  opui_config->opcontrol_params = gconf_client_get_string(client, OPROFILEUI_GCONF_PREFS "/opcontrol_params", NULL);
  opui_config->binaries_location = gconf_client_get_string(client, OPROFILEUI_GCONF_PREFS "/binaries_location", NULL);
  opui_config->vmlinux_location = gconf_client_get_string(client, OPROFILEUI_GCONF_PREFS "/vmlinux_location", NULL);

  if (opui_config->port == 0)
    opui_config->port = 4224;

  if (!opui_config->opcontrol_params)
    opui_config->opcontrol_params = g_strdup(OPROFILEUI_OPCONTROL_PARAMS_DEFAULT);

  /* Update the UI with the values */
  connect_dialog_host = glade_xml_get_widget (xml, "connect_dialog_host");
  connect_dialog_port = glade_xml_get_widget (xml, "connect_dialog_port");
  connect_dialog_localhost = glade_xml_get_widget (xml, "connect_dialog_localhost");
  connect_dialog_remotehost = glade_xml_get_widget (xml, "connect_dialog_remotehost");
  options_dialog_opcontrol_params = glade_xml_get_widget (xml, "options_dialog_opcontrol_params");
  options_dialog_binaries_location = glade_xml_get_widget (xml, "options_dialog_binaries_location");
  options_dialog_vmlinux_location = glade_xml_get_widget (xml, "options_dialog_vmlinux_location");

  gtk_entry_set_text (GTK_ENTRY (connect_dialog_host), opui_config->host);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (connect_dialog_port), opui_config->port);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (connect_dialog_localhost), opui_config->localhost);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (connect_dialog_remotehost), !opui_config->localhost);
  gtk_entry_set_text (GTK_ENTRY (options_dialog_opcontrol_params), opui_config->opcontrol_params);
  if (opui_config->binaries_location)
    gtk_entry_set_text (GTK_ENTRY (options_dialog_binaries_location), opui_config->binaries_location);
  if (opui_config->vmlinux_location)
    gtk_entry_set_text (GTK_ENTRY (options_dialog_vmlinux_location), opui_config->vmlinux_location);
}

int
main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  /* Set the window icon */
  gtk_window_set_default_icon_name ("oprofile-viewer");

  /* load the interface */
  if (g_file_test (SOURCE_GLADE, G_FILE_TEST_EXISTS) != FALSE) {
    xml = glade_xml_new (SOURCE_GLADE, NULL, NULL);
  } else {
    xml = glade_xml_new (INSTALLED_GLADE, NULL, NULL);
  }

  if (xml == NULL)
    {
      g_error ("Glade file not found.\n");
    }

  view = view_new (xml);

  main_window = glade_xml_get_widget (xml, "main_window");

  glade_xml_signal_connect_data (xml, "on_toolbar_start_clicked",
      G_CALLBACK(on_start_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_toolbar_stop_clicked",
      G_CALLBACK(on_stop_clicked), NULL);
  
  glade_xml_signal_connect_data (xml, "on_menu_start_activate",
      G_CALLBACK(on_start_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_menu_stop_activate",
      G_CALLBACK(on_stop_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_toolbar_save_clicked",
      G_CALLBACK(on_save_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_toolbar_open_clicked",
      G_CALLBACK(on_open_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_menu_save_activate",
      G_CALLBACK(on_save_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_menu_open_activate",
      G_CALLBACK(on_open_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_toolbar_connect_clicked",
      G_CALLBACK(on_connect_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_toolbar_disconnect_clicked",
      G_CALLBACK(on_disconnect_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_menu_connect_activate",
      G_CALLBACK(on_connect_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_menu_disconnect_activate",
      G_CALLBACK(on_disconnect_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_main_window_delete_event",
      G_CALLBACK(on_main_window_delete_event), NULL);
  glade_xml_signal_connect_data (xml, "on_main_window_destroy",
      G_CALLBACK(on_main_window_destroy), NULL);

  glade_xml_signal_connect_data (xml, "on_menu_group_by_application_toggled",
      G_CALLBACK(on_menu_group_by_application_toggled), NULL);
  glade_xml_signal_connect_data (xml, "on_menu_group_by_module_toggled",
      G_CALLBACK(on_menu_group_by_module_toggled), NULL);

  glade_xml_signal_connect_data (xml, "on_menu_quit_activate",
      G_CALLBACK(on_menu_quit_activate), NULL);

  glade_xml_signal_connect_data (xml, "on_menu_about_activate",
      G_CALLBACK(on_menu_about_activate), NULL);

  glade_xml_signal_connect_data (xml, "on_toolbar_download_clicked",
      G_CALLBACK(on_download_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_menu_download_activate",
      G_CALLBACK(on_download_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_toolbar_reset_clicked",
      G_CALLBACK(on_reset_clicked), NULL);
  glade_xml_signal_connect_data (xml, "on_menu_reset_activate",
      G_CALLBACK(on_reset_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_filter_entry_activate",
      G_CALLBACK(on_filter_entry_activate), NULL);

  glade_xml_signal_connect_data (xml, "on_filter_clear_button_clicked",
      G_CALLBACK(on_filter_clear_button_clicked), NULL);

  glade_xml_signal_connect_data (xml, "on_connect_dialog_localhost_toggled",
      G_CALLBACK(on_connect_dialog_localhost_toggled), NULL);

  glade_xml_signal_connect_data (xml, "on_menu_options_activate",
      G_CALLBACK(on_options_clicked ), NULL);

  opui_config = g_malloc (sizeof (struct oprofileui_config));

  main_load_default_config();

  main_set_state(OPUI_DISCONNECTED);

  client = client_new ();

  /* start the event loop */
  gtk_main();

  /* Cleanup the old archive */
  archive_full_cleanup ();

  if (gnome_vfs_initialized())
    gnome_vfs_shutdown ();

  g_free(opui_config->host);
  g_free(opui_config->opcontrol_params);
  g_free(opui_config->binaries_location);
  g_free(opui_config->vmlinux_location);
  g_free(opui_config);

  return 0;
}
