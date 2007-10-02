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


#include <libxml/parser.h>
#include <glib.h>

#include "oprofileui.h"
#include "types.h"
#include "image.h"
#include "symbol_instance.h"
#include "module_instance.h"
#include "callee_symbol_instance.h"
#include "report.h"
#include "symbol.h"
#include "module.h"
#include "parser.h"

static gchar **params = NULL;
struct oprofileui_config *opui_config;

static GOptionEntry option_entries[] = 
{
  { G_OPTION_REMAINING, NULL, 0, G_OPTION_ARG_FILENAME_ARRAY, &params, "", "" },
  { NULL }
};


static void
testparser_show_output (Report *report)
{
  GSList *cur_image;
  GSList *cur_symbol;
  GSList *cur_module;
  GSList *cur_sub_symbol;

  printf ("---- Image, module, symbol (w/ callers/callees) ----\n");

  for (cur_image = report->images; cur_image != NULL; 
      cur_image=cur_image->next)
    {
      Image *image = (Image *)(cur_image->data);
      printf ("%s %d\n", image->path, image->count);

      for (cur_symbol = image->symbol_instances; cur_symbol != NULL; 
          cur_symbol=cur_symbol->next)
        {
          SymbolInstance *symbol_instance = (SymbolInstance *)cur_symbol->data;
          Symbol *symbol = (Symbol *)symbol_instance->symbol;

          for (cur_sub_symbol = symbol_instance->callers; cur_sub_symbol != NULL;
                cur_sub_symbol = g_slist_next (cur_sub_symbol))
            {
              SymbolInstance *caller = SYMBOL_INSTANCE (cur_sub_symbol->data);
              Symbol *symbol = caller->symbol;

              printf ("\t\t Caller: %s %d\n", symbol->name, caller->count);
            }

          printf ("\t%s %d\n", symbol->name, symbol_instance->count);

          for (cur_sub_symbol = symbol_instance->callees; cur_sub_symbol != NULL;
                cur_sub_symbol = g_slist_next (cur_sub_symbol))
            {
              SymbolInstance *callee = SYMBOL_INSTANCE (cur_sub_symbol->data);
              Symbol *symbol = callee->symbol;


              if (CALLEE_SYMBOL_INSTANCE (callee)->is_self)
                printf ("\t\t Callee: %s %d [self]\n", symbol->name, callee->count);
              else
                printf ("\t\t Callee: %s %d\n", symbol->name, callee->count);
            }
        }

      for (cur_module = image->module_instances; cur_module != NULL; 
          cur_module = cur_module->next)
        {
          ModuleInstance *module_instance = (ModuleInstance *)cur_module->data;
          Module *module = (Module *)module_instance->module;

          printf ("\t%s %d\n", module->name, module_instance->count);

          for (cur_symbol = module_instance->symbol_instances; cur_symbol != NULL;
              cur_symbol=cur_symbol->next)
            {
              SymbolInstance *symbol_instance = (SymbolInstance *)cur_symbol->data;
              Symbol *symbol = (Symbol *)symbol_instance->symbol;

              for (cur_sub_symbol = symbol_instance->callers; cur_sub_symbol != NULL;
                    cur_sub_symbol = g_slist_next (cur_sub_symbol))
                {
                  SymbolInstance *caller = SYMBOL_INSTANCE (cur_sub_symbol->data);
                  Symbol *symbol = caller->symbol;

                  printf ("\t\t Caller: %s %d\n", symbol->name, caller->count);
                }

              printf ("\t%s %d\n", symbol->name, symbol_instance->count);

              for (cur_sub_symbol = symbol_instance->callees; cur_sub_symbol != NULL;
                cur_sub_symbol = g_slist_next (cur_sub_symbol))
                {
                  SymbolInstance *callee = SYMBOL_INSTANCE (cur_sub_symbol->data);
                  Symbol *symbol = callee->symbol;

                  if (CALLEE_SYMBOL_INSTANCE (callee)->is_self)
                    printf ("\t\t Callee: %s %d [self]\n", symbol->name, callee->count);
                  else
                    printf ("\t\t Callee: %s %d\n", symbol->name, callee->count);
                }
            }
        }
    }

  printf ("---- Module, symbol (w/o callers/callees) ----\n");
  for (cur_module = report->modules; cur_module != NULL; cur_module = cur_module->next)
    {
      Module *module = (Module *)cur_module->data;
      printf ("%s %d\n", module->name, module->count);

      for (cur_symbol = module->symbols; cur_symbol != NULL;
          cur_symbol=cur_symbol->next)
        {
          Symbol *symbol = (Symbol *)cur_symbol->data;
          printf ("\t%s %d\n", symbol->name, symbol->count);
        }
    }

    printf ("---- Symbol (w/ callers/callees) ----\n");

    for (cur_symbol = report->symbols; cur_symbol != NULL;
        cur_symbol=cur_symbol->next)
      {
        GSList *cur_child;

        Symbol *symbol = (Symbol *)cur_symbol->data;

        printf ("\n");
        
        for (cur_child = symbol->callers; cur_child != NULL; cur_child = g_slist_next (cur_child))
          {
            SymbolInstance *symbol_instance = SYMBOL_INSTANCE (cur_child->data);
            printf ("\tCaller: %s %d\n", symbol_instance->symbol->name, symbol_instance->count);
          }
        
        printf ("%s %d\n", symbol->name, symbol->count);

        for (cur_child = symbol->callees; cur_child != NULL; cur_child = g_slist_next (cur_child))
          {
            SymbolInstance *symbol_instance = SYMBOL_INSTANCE (cur_child->data);
            printf ("\tCallee: %s %d\n", symbol_instance->symbol->name, symbol_instance->count);
          }
      }
}

int
main (int argc, char **argv)
{
  Report *report = NULL;
  gchar *filename;

  GError *error = NULL;
  GOptionContext *context;

  opui_config->group_by_application = TRUE;
  opui_config->group_by_module = TRUE;
  opui_config->localhost = TRUE;

  context = g_option_context_new (" FILENAME - test the XML parser using this file");
  g_option_context_add_main_entries (context, option_entries, NULL);
  g_option_context_parse (context, &argc, &argv, &error);

  if (params == NULL || params[0] == NULL)
    {
      printf ("Must include the filename of the XML file to test as the parameter.\n");
      exit (1);
    }

  filename = params[0];
  g_type_init ();
  report = parser_get_report_from_file (filename);

  if (report != NULL)
    {
      testparser_show_output (report);
      g_object_unref (report);
    }

  return 0;
}


