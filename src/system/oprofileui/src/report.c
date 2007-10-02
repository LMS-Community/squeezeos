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

#include <glib.h>
#include <wait.h>
#include <gtk/gtk.h>
#include <string.h>

#include "oprofileui.h"
#include "parser.h"
#include "types.h"
#include "image.h"
#include "symbol_instance.h"
#include "module_instance.h"
#include "callee_symbol_instance.h"
#include "caller_symbol_instance.h"
#include "report.h"
#include "symbol.h"
#include "module.h"
#include "types.h"

G_DEFINE_TYPE (Report, report, G_TYPE_OBJECT);

#define REPORT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_REPORT, ReportPrivate))

typedef struct
{
  GtkTreeStore *store_cache[2][2];
} ReportPrivate;

static void
report_dispose (GObject *object)
{
  Report *report = REPORT (object);

  if (report->count == 0)
    return;

  report->count = 0;

  g_slist_foreach (report->images, (GFunc)g_object_unref, NULL);
  g_slist_foreach (report->symbols, (GFunc)g_object_unref, NULL);
  g_slist_foreach (report->modules, (GFunc)g_object_unref, NULL);

  g_slist_free (report->images);
  g_slist_free (report->symbols);
  g_slist_free (report->modules);

  if (G_OBJECT_CLASS (report_parent_class)->dispose)
    G_OBJECT_CLASS (report_parent_class)->dispose (object);
}

static void
report_finalize (GObject *object)
{
  G_OBJECT_CLASS (report_parent_class)->finalize (object);
}

static void
report_class_init (ReportClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ReportPrivate));

  object_class->dispose = report_dispose;
  object_class->finalize = report_finalize;
}

void
report_add_symbol (Report *report, Symbol *symbol)
{
  g_object_ref (symbol);

  /* Add the symbol to the report */
  report->symbols = g_slist_append (report->symbols, symbol);
}

void
report_add_image (Report *report, Image *image)
{
  g_object_ref (image);

  /* Add image to the report */
  report->images = g_slist_append (report->images, image);
}

void
report_add_module (Report *report, Module *module)
{
  g_object_ref (module);

  /* Add module to the report */
  report->modules = g_slist_append (report->modules, module);
}

static void
report_init (Report *report)
{
}

static void
report_convert_archive (gchar *archive_path)
{
  gchar *args[] = {"oparchconv", NULL, NULL};

  gchar *stdout = NULL;
  gchar *stderr = NULL;
  gint exit_status;
  GError *err = NULL;
  gboolean success;

  /* No need to convert for localhost */
  if (opui_config->localhost)
    return;

  args[1] = g_strdup_printf ("%s", archive_path);

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
      int exit_code = WEXITSTATUS (exit_status);
      if (WIFEXITED (exit_status) && exit_code==0)
        {

        } else {
          printf ("%s\n",stderr);
        }
    } else {
      printf ("%s\n", err->message);
    }

  g_free (stdout);
  g_free (stderr);
  g_free (err);
}

static Report *
report_generate_from_archive (gchar *archive_path)
{
  Report *report = NULL;
  gchar *args[] = {"opreport", "-Xc", NULL, NULL, NULL};

  gchar *stdout = NULL;
  gchar *stderr = NULL;
  gint exit_status;
  GError *err = NULL;
  gboolean success;

  args[2] = g_strdup_printf ("archive:%s", archive_path);

  if (!opui_config->localhost && opui_config->binaries_location 
          && (strlen(opui_config->binaries_location) > 0))
    {
      args[3] = g_strdup_printf("--image-path=%s", opui_config->binaries_location);
    }

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
      int exit_code = WEXITSTATUS (exit_status);
      if (WIFEXITED (exit_status) && exit_code==0)
        {
          report = parser_get_report_from_buffer (stdout, strlen (stdout));
        } else {
          printf ("%s\n",stderr);
        }
    } else {
      printf ("%s\n", err->message);
    }

  g_free (stdout);
  g_free (stderr);
  g_free (err);

  return report;
}

Report *
report_new_from_archive_path (gchar *archive_path)
{
  Report *report = NULL;

  report_convert_archive (archive_path);
  report = report_generate_from_archive (archive_path);

  return report;
}

GtkTreeStore *
report_get_treestore (Report *report, gboolean group_by_application, gboolean group_by_module)
{
  ReportPrivate *priv = NULL;
  GtkTreeStore *store = NULL;

  if (!report)
    return NULL;

  priv = REPORT_PRIVATE (report);

  store = priv->store_cache[group_by_application][group_by_module];

  if (store != NULL)
    return store;

  store = gtk_tree_store_new (4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_POINTER);

  if (group_by_application)
    {
      GSList *cur_image;

      for (cur_image = report->images; cur_image != NULL; cur_image = cur_image->next)
        {
          Image *image = (Image *)cur_image->data;
          GSList *cur_module;
          GSList *cur_symbol;
          GtkTreeIter image_iter;
          GtkTreeIter module_iter;
          GtkTreeIter symbol_iter;

          GtkTreeIter parent_iter;

          float percentage;

          /* Add the image entry to the treeview as the parent */
          gtk_tree_store_append (store, &image_iter, NULL);
          gtk_tree_store_set (store, &image_iter, 0, image->count, 1, image->path, -1);

          /* Set the percentage */
          percentage = (float)image->count/(float)report->count*100.0;
          gtk_tree_store_set (store, &image_iter, 2, percentage, -1);

          /* Add a pointer to the image */
          gtk_tree_store_set (store, &image_iter, 3, NULL, -1);

          /* Add all the symbol instances that are not embedded in a module */
          for (cur_symbol = image->symbol_instances; cur_symbol != NULL; cur_symbol = cur_symbol->next)
            {
              SymbolInstance *symbol_instance = (SymbolInstance *)cur_symbol->data;
              Symbol *symbol = symbol_instance->symbol;

              /* Add using image as parent */
              gtk_tree_store_append (store, &symbol_iter, &image_iter);
              gtk_tree_store_set (store, &symbol_iter, 0, symbol_instance->count, 1, symbol->name, -1);

              /* Set the percentage */
              percentage = (float)symbol_instance->count/(float)image->count*100.0;
              gtk_tree_store_set (store, &symbol_iter, 2, percentage, -1);

              /* Add a pointer to the symbol_instance */
              gtk_tree_store_set (store, &symbol_iter, 3, symbol_instance, -1);

            }

          /* Add all the module that part of this image */
          for (cur_module = image->module_instances; cur_module != NULL; cur_module = cur_module->next)
            {
              ModuleInstance *module_instance = (ModuleInstance *)cur_module->data;
              Module *module = module_instance->module;

              /* 
               * If this is set then the symbol instances will be added as
               * children of the module entries that are also added.
               */
              if (group_by_module)
                {
                  /* Add using image as parent */
                  gtk_tree_store_append (store, &module_iter, &image_iter);
                  gtk_tree_store_set (store, &module_iter, 0, module_instance->count, 1, module->name, -1);

                  /* Set the percentage */
                  percentage = (float)module_instance->count/(float)image->count*100.0;
                  gtk_tree_store_set (store, &module_iter, 2, percentage, -1);

                  /* Add a pointer to the module_instance */
                  gtk_tree_store_set (store, &module_iter, 3, module_instance, -1);

                  parent_iter = module_iter;
                } else {
                  parent_iter = image_iter;
                }

              /* Add the symbols that appear in this module */
              for (cur_symbol = module_instance->symbol_instances; cur_symbol != NULL; cur_symbol = cur_symbol->next)
                {
                  SymbolInstance *symbol_instance = (SymbolInstance *)cur_symbol->data;
                  Symbol *symbol = symbol_instance->symbol;

                  /* Add using parent_iter as parent */
                  gtk_tree_store_append (store, &symbol_iter, &parent_iter);
                  gtk_tree_store_set (store, &symbol_iter, 0, symbol_instance->count, 1, symbol->name, -1);

                  /* Set the percentage */
                  
                  if (group_by_module)
                    {
                      percentage = (float)symbol_instance->count/(float)module_instance->count*100.0;
                    } else {
                      percentage = (float)symbol_instance->count/(float)image->count*100.0;
                    }
                  gtk_tree_store_set (store, &symbol_iter, 2, percentage, -1);

                  /* Add a pointer to the symbol_instance */
                  gtk_tree_store_set (store, &symbol_iter, 3, symbol_instance, -1);
                }
            }
        }
    } else {
      if (group_by_module)
        {
          GSList *cur_module;
          float percentage;

          for (cur_module = report->modules; cur_module != NULL; cur_module = cur_module->next)
            {
              GSList *cur_symbol;
              Module *module = (Module *)cur_module->data;
              GtkTreeIter module_iter;

              /* Add modules at the top level to the store */
              gtk_tree_store_append (store, &module_iter, NULL);
              gtk_tree_store_set (store, &module_iter, 0, module->count, 1, module->name, -1);

              /* Set the percentage */
              percentage = (float)module->count/(float)report->count*100.0;
              gtk_tree_store_set (store, &module_iter, 2, percentage, -1);

              /* Add a pointer to the module */
              gtk_tree_store_set (store, &module_iter, 3, module, -1);

              /* Now add the symbols that are found in the module */
              for (cur_symbol = module->symbols; cur_symbol != NULL; cur_symbol = cur_symbol->next)
                {
                  Symbol *symbol = (Symbol *)cur_symbol->data;
                  GtkTreeIter symbol_iter;

                  /* Add the symbol as a child of the module */
                  gtk_tree_store_append (store, &symbol_iter, &module_iter);
                  gtk_tree_store_set (store, &symbol_iter, 0, symbol->count, 1, symbol->name, -1);

                  /* Set the percentage */
                  percentage = (float)symbol->count/(float)module->count*100.0;
                  gtk_tree_store_set (store, &symbol_iter, 2, percentage, -1);

                  /* Add a pointer to the symbol */
                  gtk_tree_store_set (store, &symbol_iter, 3, symbol, -1);
                }
            }
        } else {
          GSList *cur_symbol;
          float percentage;

          for (cur_symbol = report->symbols; cur_symbol != NULL; cur_symbol = cur_symbol->next)
            {
              Symbol *symbol = cur_symbol->data;
              GtkTreeIter symbol_iter;

              /* Add the symbol as a child of the module */
              gtk_tree_store_append (store, &symbol_iter, NULL);
              gtk_tree_store_set (store, &symbol_iter, 0, symbol->count, 1, symbol->name, -1);

              /* Set the percentage */
              percentage = (float)symbol->count/(float)report->count*100.0;
              gtk_tree_store_set (store, &symbol_iter, 2, percentage, -1);

              /* Add a pointer to the symbol */
              gtk_tree_store_set (store, &symbol_iter, 3, symbol, -1);

            }
        }
    }


  priv->store_cache[group_by_application][group_by_module] = store;

  return store;
}

GtkListStore *
report_get_symbol_details_liststore (Report *report, Symbol *symbol)
{
  GtkListStore *store = NULL;
  GSList *cur_entry;
  float percentage;

  store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT);

  for (cur_entry = symbol->instances; cur_entry != NULL; 
      cur_entry = cur_entry->next)
  {
    GtkTreeIter iter;
    SymbolInstance *symbol_instance = (SymbolInstance *)cur_entry->data;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, 
        symbol_instance->image->path, 1, symbol_instance->count, -1);

    percentage = (float)symbol_instance->count/(float)symbol->count * 100.0;
    gtk_list_store_set (store, &iter, 2, percentage, -1);
  }

  return store;
}

static GtkListStore *
report_update_child_list_model (Report *report, GSList *child_list)
{
  GtkListStore *store = NULL;
  GSList *cur_entry = NULL;
  float percentage;
  guint total = 0;
  GtkTreeIter iter;

  store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_FLOAT);

  for (cur_entry = child_list; cur_entry != NULL; cur_entry = g_slist_next (cur_entry))
    {
      SymbolInstance *symbol_instance = SYMBOL_INSTANCE (cur_entry->data);
      Symbol *symbol = symbol_instance->symbol;

      /* 
       * Just add the name and the count, leave the percentage for later since
       * we need the total.
       */

      gtk_list_store_append (store, &iter);

      if (IS_CALLEE_SYMBOL_INSTANCE (symbol_instance) && 
          CALLEE_SYMBOL_INSTANCE (symbol_instance)->is_self)
        {
          gtk_list_store_set (store, &iter, 0, g_strdup_printf ("%s [self]", symbol->name), -1);
        } else {
          gtk_list_store_set (store, &iter, 0, symbol->name, -1);
        }

      gtk_list_store_set (store, &iter, 1, symbol_instance->count, -1);

      total += symbol_instance->count;
    }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
    {
      do {
        guint count = 0;

        gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 1, &count, -1);
        percentage = (float)count/(float)total * 100;

        gtk_list_store_set (store, &iter, 2, percentage, -1);

      } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
    }

  return store;
}

GtkListStore *
report_get_symbol_instance_callers (Report *report, SymbolInstance *symbol_instance)
{
  return report_update_child_list_model (report, symbol_instance->callers);
}

GtkListStore *
report_get_symbol_instance_callees (Report *report, SymbolInstance *symbol_instance)
{
  return report_update_child_list_model (report, symbol_instance->callees);
}

GtkListStore *
report_get_symbol_callers (Report *report, Symbol *symbol)
{
  return report_update_child_list_model (report, symbol->callers);
}

GtkListStore *
report_get_symbol_callees (Report *report, Symbol *symbol)
{
  return report_update_child_list_model (report, symbol->callees);
}

GtkListStore *
report_get_module_details_liststore (Report *report, Module *module)
{
  GtkListStore *store = NULL;
  GSList *cur_entry;
  float percentage;

  store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT);

  for (cur_entry = module->instances; cur_entry != NULL; 
      cur_entry = cur_entry->next)
  {
    GtkTreeIter iter;
    ModuleInstance *module_instance = (ModuleInstance *)cur_entry->data;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, 
        module_instance->image->path, 1, module_instance->count, -1);

    percentage = (float)module_instance->count/(float)module->count * 100.0;
    gtk_list_store_set (store, &iter, 2, percentage, -1);
  }

  return store;
}


