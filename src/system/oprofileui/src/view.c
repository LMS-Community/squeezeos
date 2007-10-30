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


#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>
#include <math.h>

#define _GNU_SOURCE
#include <string.h>

#include "view.h"
#include "module.h"
#include "symbol.h"

#include "module_instance.h"
#include "symbol_instance.h"

G_DEFINE_TYPE (View, view, G_TYPE_OBJECT);

typedef struct
{
  GtkTreeView *treeview;
  gchar *filter_string;
  GtkTreeStore *child_model;
  GtkTreeModel *filtered_model;
  GtkTreeModel *model;
  GladeXML *xml;
  Report *report;
} ViewPrivate; 

enum
{
  PROP_REPORT = 1,
  PROP_FILTER_STRING
};

#define VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_VIEW, ViewPrivate))

static void
view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  ViewPrivate *priv = NULL;

  priv = VIEW_PRIVATE (object);

  switch (property_id) {
    case PROP_REPORT:
      g_value_set_object (value, priv->report);
      break;
    case PROP_FILTER_STRING:
      g_value_set_string (value, priv->filter_string);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  Report *report = NULL;
  ViewPrivate *priv = NULL;

  priv = VIEW_PRIVATE (object);

  switch (property_id) {
    case PROP_REPORT:
      if (priv->report)
        g_object_unref (priv->report);

      report = g_value_dup_object (value);

      priv->report = report;
      break;
    case PROP_FILTER_STRING:
      if (priv->filter_string)
        g_free (priv->filter_string);

      priv->filter_string = g_value_dup_string (value);

      if (priv->filtered_model != NULL)
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filtered_model));

      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    return;
  }
}

static gboolean
filter_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer userdata)
{
  ViewPrivate *priv = NULL;
  gchar *tmp = NULL;

  priv = VIEW_PRIVATE (userdata);

  if (priv->filter_string == NULL)
    return TRUE;

  gtk_tree_model_get (model, iter, 1, &tmp, -1);

  if (strstr (tmp, priv->filter_string) != NULL)
    {
      g_free (tmp);
      return TRUE;
    } else {
      g_free (tmp);
      return FALSE;
    }
}

static gboolean
search_comparison_func (GtkTreeModel *model, gint column, const gchar *key, 
    GtkTreeIter *iter, gpointer userdata)
{
  gchar *tmp = NULL;

  gtk_tree_model_get (model, iter, column, &tmp, -1);

  if (strstr (tmp, key) != NULL)
    {
      g_free (tmp);
      return FALSE;
    } else {
      g_free (tmp);
      return TRUE;
    }
}

static void
cell_colourer (GtkTreeViewColumn *col, GtkCellRenderer *cell,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer userdata)
{
  float percentage;
  GdkColor color;
  guint val;

  gtk_tree_model_get(model, iter, 2, &percentage, -1);

  if (percentage > 0.001)
    {
      val = (guint)roundf(65535.0 - (log(1000 * (percentage)) / log(100000.0) * 65535.0));
    } else {
      val = 65535;
    }

  color.red = 65535;
  color.green = val;
  color.blue = val;
  g_object_set (cell, "cell-background-set", TRUE, "cell-background-gdk", &color, NULL);
}

static void
view_show_details_symbol_common_tv_setup (GtkTreeView *tv)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;

  /* Enable search for the name field */
  gtk_tree_view_set_enable_search (tv, TRUE);
  gtk_tree_view_set_search_column (tv, 0); 
  gtk_tree_view_set_search_equal_func (tv, 
      (GtkTreeViewSearchEqualFunc)search_comparison_func, NULL, NULL); 

  /* First column for percentage */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 2);

  /* Make it sortable */
  gtk_tree_view_column_set_sort_column_id (col, 2);

  /* Use funky colourer */
  gtk_tree_view_column_set_cell_data_func (col, renderer, cell_colourer, 
      NULL, NULL);

  gtk_tree_view_append_column(tv, col);

  /* Second column for the image name */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(tv, col);

  /* Third column for the count */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
  gtk_tree_view_column_set_title(col, "Count");
  gtk_tree_view_append_column(tv, col);
}

void
view_show_details_symbol_common (View *view, Symbol *symbol, GtkListStore *callers, GtkListStore *callees)
{
  ViewPrivate *priv = NULL;
  GtkLabel *label;
  gchar *text;

  GtkTreeView *treeview = NULL;
  GtkListStore *store;
  GtkTreeViewColumn *col;

  GtkWidget *notebook;
  GtkWidget *symbol_notebook;

  GtkTreeView *tv_callers = NULL;
  GtkTreeView *tv_callees = NULL;

  priv = VIEW_PRIVATE (view);

  treeview = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_symbol_instances"));

  tv_callers = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_symbol_callers"));
  tv_callees = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_symbol_callees"));

  gtk_tree_view_set_model (tv_callers, GTK_TREE_MODEL(callers));
  gtk_tree_view_set_model (tv_callees, GTK_TREE_MODEL(callees));

  /* Set the label for the symbol name */
  text = g_strdup_printf ("<big><b>Symbol: %s</b></big>", symbol->name);
  label = GTK_LABEL (glade_xml_get_widget (priv->xml, "symbol_label_name"));
  gtk_label_set_label (label, text);

  /* Set the label for symbol statistics */
  text = g_strdup_printf("Total count: %d\nGlobal percentage: %f%%",
      symbol->count,
      (float)symbol->count/(float)symbol->report->count * 100.0);

  if (symbol->module != NULL)
    {
      text = g_strdup_printf("%s\nModule percentage: %f%%\nModule: %s",
          text,
          (float)symbol->count/(float)symbol->module->count * 100.0,
          symbol->module->name);
    }

  label = GTK_LABEL (glade_xml_get_widget (priv->xml, "symbol_label_statistics"));
  gtk_label_set_label (label, text);

 
  /* Get the model for this symbol */
  store = report_get_symbol_details_liststore (priv->report, symbol);
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL(store));

  /* Set the model up so that it is sorted in descending for percentage */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 2,
      GTK_SORT_DESCENDING);

  /* Get the first column. This is the one we are using for the percentage. */
  col = gtk_tree_view_get_column (treeview, 0);

  /* Force the sorting on the first column to descending */
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_DESCENDING);

  /* Must show the symbol_notebook before we can change page */
  symbol_notebook = glade_xml_get_widget (priv->xml, "symbol_notebook");
  gtk_widget_show_all (symbol_notebook);

  /* Set this to the first page */
  gtk_notebook_set_current_page (GTK_NOTEBOOK(symbol_notebook), 0);

  /* Chage the page in the notebook */
  notebook = glade_xml_get_widget (priv->xml, "notebook");
  gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 0);
}

void
view_show_details_symbol_instance (View *view, SymbolInstance *symbol_instance)
{
  ViewPrivate *priv = NULL;
  Symbol *symbol = symbol_instance->symbol;
  GtkListStore *callers;
  GtkListStore *callees;

  priv = VIEW_PRIVATE (view);

  callers = report_get_symbol_instance_callers (priv->report, symbol_instance);
  callees = report_get_symbol_instance_callees (priv->report, symbol_instance);

  view_show_details_symbol_common (view, symbol, callers, callees);
}

void
view_show_details_symbol (View *view, Symbol *symbol)
{
  ViewPrivate *priv = NULL;

  GtkListStore *callers;
  GtkListStore *callees;

  priv = VIEW_PRIVATE (view);

  callers = report_get_symbol_callers (priv->report, symbol);
  callees = report_get_symbol_callees (priv->report, symbol);

  view_show_details_symbol_common (view, symbol, callers, callees);
}

void
view_show_details_module (View *view, Module *module)
{
  ViewPrivate *priv = NULL;
  GtkLabel *label;
  gchar *text;

  GtkTreeView *treeview = NULL;
  GtkListStore *store;
  GtkTreeViewColumn *col;

  GtkWidget *notebook;
  GtkWidget *module_notebook;

  priv = VIEW_PRIVATE (view);

  treeview = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_module_instances"));

  /* Set the label for the module name */
  text = g_strdup_printf ("<big><b>Module: %s</b></big>", module->name);
  label = GTK_LABEL (glade_xml_get_widget (priv->xml, "module_label_name"));
  gtk_label_set_label (label, text);

  /* Set the label for module statistics */
  text = g_strdup_printf("Total count: %d\nGlobal percentage: %f%%",
      module->count,
      (float)module->count/(float)module->report->count * 100.0);

  label = GTK_LABEL (glade_xml_get_widget (priv->xml, "module_label_statistics"));
  gtk_label_set_label (label, text);

  /* Get the model for this module */
  store = report_get_module_details_liststore (priv->report, module);
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL(store));

  /* Set the model up so that it is sorted in descending for percentage */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 2,
      GTK_SORT_DESCENDING);

  /* Get the first column. This is the one we are using for the percentage. */
  col = gtk_tree_view_get_column (treeview, 0);

  /* Force the sorting on the first column to descending */
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_DESCENDING);

  /* Must show the symbol_frame before we can change page */
  module_notebook = glade_xml_get_widget (priv->xml, "module_notebook");
  gtk_widget_show_all (module_notebook);

  /* Set this to the first page */
  gtk_notebook_set_current_page (GTK_NOTEBOOK(module_notebook), 0);

  /* Chage the page in the notebook */
  notebook = glade_xml_get_widget (priv->xml, "notebook");
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 1);
}

static void
on_tree_selection_changed (GtkTreeSelection *selection, gpointer userdata)
{
  View *view = NULL;
  ViewPrivate *priv = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkNotebook *notebook = NULL;

  view = VIEW (userdata);
  priv = VIEW_PRIVATE (view);

  notebook = GTK_NOTEBOOK (glade_xml_get_widget (priv->xml, "notebook"));

  gtk_widget_hide_all (GTK_WIDGET (notebook));

  if (gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gpointer data;
    gtk_tree_model_get (model, &iter, 3, &data, -1);

    if (IS_SYMBOL_INSTANCE (data) || IS_SYMBOL (data))
      {
        Symbol *symbol;

        if (IS_SYMBOL_INSTANCE (data))
          {
            SymbolInstance *symbol_instance = SYMBOL_INSTANCE (data);
            symbol = symbol_instance->symbol;
            view_show_details_symbol_instance (view, symbol_instance);
          } else {
            symbol = SYMBOL (data);
            view_show_details_symbol (view, symbol);
          }
        
        gtk_widget_show_all (GTK_WIDGET (notebook));

      }
    if (IS_MODULE_INSTANCE (data) || IS_MODULE (data))
      {
        Module *module;

        if (IS_MODULE_INSTANCE (data))
          {
            ModuleInstance *module_instance = MODULE_INSTANCE (data);
            module = module_instance -> module;
          } else {
            module = MODULE (data);
          }

        view_show_details_module(view, module);
        gtk_widget_show_all (GTK_WIDGET (notebook));
      }
  }
}

static void
view_setup_symbol_instances_tv (View *view)
{
  ViewPrivate *priv = NULL;
  GtkCellRenderer *renderer;
  GtkTreeView *treeview = NULL;
  GtkTreeViewColumn *col = NULL;

  priv = VIEW_PRIVATE (view);

  treeview = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_symbol_instances"));

  /* Enable search for the name field */
  gtk_tree_view_set_enable_search (treeview, TRUE);
  gtk_tree_view_set_search_column (treeview, 0); 
  gtk_tree_view_set_search_equal_func (treeview, 
      (GtkTreeViewSearchEqualFunc)search_comparison_func, NULL, NULL); 

  /* First column for percentage */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 2);

  /* Make it sortable */
  gtk_tree_view_column_set_sort_column_id (col, 2);

  /* Use funky colourer */
  gtk_tree_view_column_set_cell_data_func (col, renderer, cell_colourer, 
      NULL, NULL);

  gtk_tree_view_append_column(treeview, col);

  /* Second column for the image name */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(treeview, col);

  /* Third column for the count */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
  gtk_tree_view_column_set_title(col, "Count");
  gtk_tree_view_append_column(treeview, col);
}

static void
view_setup_module_instances_tv (View *view)
{
  ViewPrivate *priv = NULL;
  GtkCellRenderer *renderer;
  GtkTreeView *treeview = NULL;
  GtkTreeViewColumn *col = NULL;

  priv = VIEW_PRIVATE (view);

  treeview = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_module_instances"));

  /* Enable search for the name field */
  gtk_tree_view_set_enable_search (treeview, TRUE);
  gtk_tree_view_set_search_column (treeview, 0); 
  gtk_tree_view_set_search_equal_func (treeview, 
      (GtkTreeViewSearchEqualFunc)search_comparison_func, NULL, NULL); 

  /* First column for percentage */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 2);

  /* Make it sortable */
  gtk_tree_view_column_set_sort_column_id (col, 2);

  /* Use funky colourer */
  gtk_tree_view_column_set_cell_data_func (col, renderer, cell_colourer, 
      NULL, NULL);

  gtk_tree_view_append_column(treeview, col);

  /* Second column for the image name */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(treeview, col);

  /* Third column for the count */
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
  gtk_tree_view_column_set_title(col, "Count");
  gtk_tree_view_append_column(treeview, col);
}

static void
view_setup (View *view, GladeXML *xml)
{
  ViewPrivate *priv = NULL;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkTreeSelection *tree_selection;
  GtkTreeView *tv_callers = NULL;
  GtkTreeView *tv_callees = NULL;

  priv = VIEW_PRIVATE (view);
  priv->xml = xml;

  priv->treeview = GTK_TREE_VIEW (glade_xml_get_widget (xml, "treeview"));

  /* Enable search for the name field */
  gtk_tree_view_set_enable_search (priv->treeview, TRUE);
  gtk_tree_view_set_search_column (priv->treeview, 1); 
  gtk_tree_view_set_search_equal_func (priv->treeview, 
      (GtkTreeViewSearchEqualFunc)search_comparison_func, NULL, NULL); 

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 2);

  /* Set the model column that this column sorts */
  gtk_tree_view_column_set_sort_column_id (col, 2);

  gtk_tree_view_column_set_cell_data_func (col, renderer, 
      cell_colourer, NULL, NULL);

  gtk_tree_view_append_column(priv->treeview, col);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
  gtk_tree_view_column_set_title (col, "Name");
  gtk_tree_view_append_column(priv->treeview, col);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
  gtk_tree_view_column_set_title (col, "Count");
  gtk_tree_view_append_column(priv->treeview, col);

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
  g_signal_connect (tree_selection, "changed", G_CALLBACK (on_tree_selection_changed), view);

  tv_callers = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_symbol_callers"));
  tv_callees = GTK_TREE_VIEW (glade_xml_get_widget (priv->xml, "tv_symbol_callees"));
  view_show_details_symbol_common_tv_setup (tv_callees);
  view_show_details_symbol_common_tv_setup (tv_callers);

  view_setup_symbol_instances_tv (view);
  view_setup_module_instances_tv (view);
}

void
view_set_display_mode (View *view, gboolean group_by_application, gboolean group_by_module)
{
  ViewPrivate *priv = NULL;
  GtkTreeViewColumn *tvcolumn;

  priv = VIEW_PRIVATE (view);

  /* Get the new model */
  priv->child_model = report_get_treestore (priv->report, group_by_application, group_by_module);

  if (priv->child_model == NULL)
    return;

  /* Wrap the derived model in a filter */
  priv->filtered_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->child_model), NULL);

  /* Add the filter function to the filter */
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filtered_model), 
      (GtkTreeModelFilterVisibleFunc)filter_func, view, NULL);

  /* Filter it */
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filtered_model));

  /* Wrap again to make it sortable */
  priv->model = gtk_tree_model_sort_new_with_model (priv->filtered_model);

  /* Set the new model */
  gtk_tree_view_set_model (priv->treeview, GTK_TREE_MODEL (priv->model));
  
  /* Set the model up so that it is sorted in descending for percentage */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->model), 2,
      GTK_SORT_DESCENDING);

  /* Get the first column. This is the one we are using for the percentage. */
  tvcolumn = gtk_tree_view_get_column (priv->treeview, 0);

  /* Force the sorting on the first column to descending */
  gtk_tree_view_column_set_sort_order (tvcolumn, GTK_SORT_DESCENDING);
}

static void
view_class_init (ViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (ViewPrivate));

  object_class->get_property = view_get_property;
  object_class->set_property = view_set_property;

  pspec = g_param_spec_object ("report", "report", "report",
      TYPE_REPORT, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_REPORT, pspec);

  pspec = g_param_spec_string ("filter-string", "filter-string", "filter-string",
      NULL, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_FILTER_STRING, pspec);
}

static void
view_init (View *view)
{
}

View *
view_new (GladeXML *xml)
{
  View *view = NULL;

  view = g_object_new (TYPE_VIEW, NULL);
  view_setup (view, xml);

  return view;
}
