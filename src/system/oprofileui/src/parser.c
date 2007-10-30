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

#include "types.h"
#include "image.h"
#include "symbol_instance.h"
#include "callee_symbol_instance.h"
#include "caller_symbol_instance.h"
#include "module_instance.h"
#include "report.h"
#include "symbol.h"
#include "module.h"

struct _parser_state 
{
  gboolean inBinary;
  gboolean inCount;
  gboolean inModule;
  gboolean inSymbol;

  gboolean inCallees;
  gboolean inCalleeSymbol;
  gboolean inCallers;
  gboolean inCallerSymbol;

  Image *curImage;
  SymbolInstance *curSymbolInstance;
  CalleeSymbolInstance *curCalleeSymbolInstance;
  CallerSymbolInstance *curCallerSymbolInstance;

  ModuleInstance *curModuleInstance;
  guint tmpCount;

  GHashTable *symbol_table;
  GHashTable *module_table;

  Report *report;
};

typedef struct _parser_state parser_state;

static parser_state *
parser_state_new ()
{
  parser_state *state;
  state = g_new0 (parser_state, 1);

  state->symbol_table = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_object_unref);
  state->module_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  state->report = g_object_new (TYPE_REPORT, NULL);
  
  return state;
}

static Symbol *
lookup_in_symbol_table (parser_state *state, guint id)
{
  Symbol *symbol;
  guint *value;

  symbol = g_hash_table_lookup (state->symbol_table, &id);

  if (symbol != NULL)
    return symbol;

  symbol = symbol_new ();

  /* Copy the int so that it can be used as a key */
  value = g_malloc0 (sizeof(guint));
  *value = id;

  g_hash_table_insert(state->symbol_table, value, symbol);

  report_add_symbol (state->report, symbol);

  return symbol;
}

static Module *
lookup_in_module_table (parser_state *state, gchar *name)
{
  Module *module;
  
  module = g_hash_table_lookup (state->module_table, name);

  if (module != NULL)
    return module;

  module = module_new ();
  module->name = g_strdup(name);

  g_hash_table_insert(state->module_table, g_strdup(name), module);

  /* Add the module to the report */
  report_add_module (state->report, module);

  return module;
}

static const xmlChar *
lookup_attribute (const xmlChar **attrs, gchar *name)
{
  int i = 0;
  for (i = 0; attrs[i]!=NULL; i++)
    {
      if (g_str_equal (name, attrs[i]))
        {
          return attrs[i+1];
        }
    }

  return NULL;
}

void
parser_characters (void *user_data, const xmlChar *ch, int len)
{
  parser_state *state = (parser_state *)user_data;
  gchar *tmp;

  tmp = g_strndup ((gchar *)ch, len);
  g_strstrip (tmp);

  if (!g_str_equal(tmp, ""))
  {
    state->tmpCount = strtol(tmp, NULL, 0);
  }

  g_free(tmp);
}

void
parser_startElement (void *user_data, const xmlChar *name, const xmlChar **attrs)
{
  parser_state *state = (parser_state *)user_data;

  if (g_str_equal (name, "binary"))
    {
      gchar *tmp;

      state->inBinary = TRUE;

      if ((tmp = (gchar *)lookup_attribute (attrs, "name")) != NULL)
        {
          state->curImage = image_new ();
          state->curImage->path = g_strdup (tmp);
        }
    }

  if (g_str_equal (name, "count"))
    {
      state->inCount = TRUE;
    }

  if (g_str_equal (name, "symbol"))
    {
      gchar *tmp = NULL;

      if (state->inCallees || state->inCallers)
        {
          SymbolInstance *symbol_instance;

          if (state->inCallees)
            {
              gchar *tmp;

              state->inCalleeSymbol = TRUE;

              /* Create a new instance of this symbol */
              symbol_instance = SYMBOL_INSTANCE (callee_symbol_instance_new ());
              
              if ((tmp = (gchar *)lookup_attribute (attrs, "self")) != NULL)
                {
                  if (g_str_equal (tmp, "true"))
                    {
                      CALLEE_SYMBOL_INSTANCE (symbol_instance)->is_self = TRUE;
                    }
                }
              /* Update the internal state */
              state->curCalleeSymbolInstance = CALLEE_SYMBOL_INSTANCE (symbol_instance);
            }

          if (state->inCallers)
            {
              state->inCallerSymbol = TRUE;

              /* Create a new instance of this symbol */
              symbol_instance = SYMBOL_INSTANCE (caller_symbol_instance_new ());

              /* Update the internal state */
              state->curCallerSymbolInstance = CALLER_SYMBOL_INSTANCE (symbol_instance);
            }

          if ((tmp = (gchar *)lookup_attribute (attrs, "idref"))!=NULL)
            {
              guint idref;
              Symbol *symbol;

              /* Get symbol from the table */
              idref = atoi (tmp);
              symbol = lookup_in_symbol_table (state, idref);

              /* Set the symbol pointer */
              symbol_instance_set_symbol (SYMBOL_INSTANCE(symbol_instance), symbol);

              /* Set the image backpointer */
              symbol_instance_set_image (SYMBOL_INSTANCE(symbol_instance), state->curImage);

              /* Set the back pointer to the report. */
              symbol->report = state->report;
            }

        } else {

          state->inSymbol = TRUE;

          if ((tmp = (gchar *)lookup_attribute (attrs, "idref"))!=NULL)
            {
              guint idref;
              Symbol *symbol;
              SymbolInstance *symbol_instance;

              /* Get symbol from the table */
              idref = atoi (tmp);
              symbol = lookup_in_symbol_table (state, idref);

              /* Create a new instance of this symbol */
              symbol_instance = symbol_instance_new ();
          
              /* Set the symbol pointer */
              symbol_instance_set_symbol (symbol_instance, symbol);

              /* Set the image backpointer */
              symbol_instance_set_image (symbol_instance, state->curImage);

              /* Set the back pointer to the report. */
              symbol->report = state->report;

              /* Set the pointer to the module */
              if (symbol->module == NULL && state->inModule)
                symbol_set_module (symbol, state->curModuleInstance->module);

              /* Update the internal state */
              state->curSymbolInstance = symbol_instance;
            }
        }
    }

  if (g_str_equal (name, "module"))
    {
      gchar *tmp = NULL;

      /* 
       * If we are inside a symbol back out. Since we must be coming up to one
       * of the nested module entries.
       */
      if (state->inSymbol)
        return;

      state->inModule = TRUE;

      if ((tmp = (gchar *)lookup_attribute (attrs, "name")) != NULL)
        {
          Module *module = NULL;
          ModuleInstance *module_instance = NULL;

          /* Get module from the table */
          module = lookup_in_module_table (state, tmp);

          /* Create a new instance of this module */
          module_instance = module_instance_new ();
          module_instance_set_module (module_instance, module);

          /* Set the report backpointer */
          if (module->report == NULL)
            module->report = state->report;

          /* Setthe image back pointer */
          module_instance_set_image (module_instance, state->curImage);

          /* Update the internal state */
          state->curModuleInstance = module_instance;
        }

    }

  if (g_str_equal (name, "symboldata"))
    {
      /* We can now equip the symbol with a name! */

      gchar *tmp;
      if ((tmp = (gchar *)lookup_attribute (attrs, "id")) != NULL)
        {
          guint idref;
          Symbol *symbol;

          /* Get symbol from the table */
          idref = atoi (tmp);
          symbol = lookup_in_symbol_table (state, idref);

          /* Get the symbol name and update the symbol */
          gchar *name = (gchar *)lookup_attribute (attrs, "name");
          symbol->name = g_strdup (name);
        }
    }

  if (g_str_equal (name, "callees"))
    {
      /* We are in the block of callees */
      state->inCallees = TRUE;
    }

  if (g_str_equal (name, "callers"))
    {
      /* We are in the block of callers */
      state->inCallers = TRUE;
    }
}

void 
parser_endElement (void *user_data, const xmlChar *name)
{
  parser_state *state = (parser_state *)user_data;

  if (g_str_equal (name, "count"))
    {
      guint count;

      state->inCount = FALSE;

      count = state->tmpCount;
      state->tmpCount = 0;

      if (state->inCalleeSymbol)
        {
          /* Update the count for the instance */
          SYMBOL_INSTANCE (state->curCalleeSymbolInstance)->count = count;

          if (!state->curCalleeSymbolInstance->is_self)
            {
              /* 
              * Update the count for this symbol as a callee of the top-level
              * symbol. Potentially adding it in the process.
              */
              symbol_callee_increment_count (state->curSymbolInstance->symbol,
                  SYMBOL_INSTANCE (state->curCalleeSymbolInstance)->symbol,
                  count);
            }

          /* But we don't update the count for the underlying symbol...*/
          return;
        }

      if (state->inCallerSymbol)
        {
          /* Update the count for the instance */
          SYMBOL_INSTANCE (state->curCallerSymbolInstance)->count = count;

          /* 
           * Update the count for this symbol as a caller of the top-level
           * symbol. Potentially adding it in the process.
           */
          symbol_caller_increment_count (state->curSymbolInstance->symbol,
              SYMBOL_INSTANCE (state->curCallerSymbolInstance)->symbol,
              count);

          /* But we don't update the count for the underlying symbol...*/
          return;
        }

      if (state->inSymbol)
        {
          /* Update the count for the instance ... */
          state->curSymbolInstance->count = count;
          /* ... and the underlying symbol */
          state->curSymbolInstance->symbol->count += count;
          return;
        }
      
      if (state->inModule)
        {
          /* Update the count for the instance ... */
          state->curModuleInstance->count = count;
          /* ... and the underlying module */
          state->curModuleInstance->module->count += count;
          return;
        }

      if (state->inBinary)
        {
          /* Set the count for the image */
          state->curImage->count = count;

          /* Update the count for the report */
          state->report->count += count;
          return;
        }
    }

  if (g_str_equal (name, "symbol"))
    {
      SymbolInstance *symbol_instance = state->curSymbolInstance;
      Symbol *symbol = symbol_instance->symbol;

      if (state->inCalleeSymbol)
        {
          CalleeSymbolInstance *callee_symbol_instance = state->curCalleeSymbolInstance;

          state->inCalleeSymbol = FALSE;

          /* 
           * Add this callee symbol instance to the list of callees for the
           * symbol instance at the top level
           */
          symbol_instance_add_callee (symbol_instance, callee_symbol_instance);

          g_object_unref (callee_symbol_instance);

          return; /* Important, we don't want to do the other stuff */
        }

      if (state->inCallerSymbol)
        {
          CallerSymbolInstance *caller_symbol_instance = state->curCallerSymbolInstance;

          state->inCallerSymbol = FALSE;

          /* 
           * Add this caller symbol instance to the list of callers for the
           * symbol instance at the top level
           */
          symbol_instance_add_caller (symbol_instance, caller_symbol_instance);

          g_object_unref (caller_symbol_instance);

          return; /* Important, we don't want to do the other stuff */
        }

      state->inSymbol = FALSE;

      /* Add this instance to the list of instances for the symbol */
      symbol_add_instance (symbol, symbol_instance);

      if (state->inBinary && !state->inModule)
        {
          Image *image = state->curImage;

          /* Add symbol instance that occur outside a module to the image */
          image_add_symbol_instance (image, symbol_instance);

          g_object_unref (symbol_instance);

          return;
        }

      if (state->inModule)
        {
          ModuleInstance *module_instance = state->curModuleInstance;
          Module *module = module_instance->module;
          Symbol *symbol = symbol_instance->symbol;

          /* Add symbol instance that occurs within a module */
          module_instance_add_symbol_instance (module_instance, symbol_instance);

          module_add_symbol (module, symbol);

          g_object_unref (symbol_instance);

          return;
      }
    }

  if (g_str_equal (name, "binary"))
    {
      Report *report = state->report;

      state->inBinary = FALSE;

      /* Add the image to the report */
      report_add_image (report, state->curImage);

      g_object_unref (state->curImage);
      state->curImage = NULL;
    }

  if (g_str_equal (name, "module"))
    {
      /* Back out if we are inside a symbol block */
      if (!state->inSymbol)
      {
        Image *image = state->curImage;
        Module *module = state->curModuleInstance->module;


        state->inModule = FALSE;

        /* Add this module to the list of modules for this image */
        image_add_module_instance (image, state->curModuleInstance);

        /* Add this instance to the list of instances for this module */
        module_add_instance (module, state->curModuleInstance);

        g_object_unref (state->curModuleInstance);
        state->curModuleInstance = NULL;
      }
    }

  if (g_str_equal (name, "callees"))
    {
      /* We are leaving the block of callees */
      state->inCallees = FALSE;
    }

  if (g_str_equal (name, "callers"))
    {
      /* We are leaving the block of callers */
      state->inCallers = FALSE;
    }
}

xmlSAXHandler handler = {
  NULL,    /* internalSubset */
  NULL,    /* isStandalone */
  NULL,    /* hasInternalSubset */
  NULL,    /* hasExternalSubset */
  NULL,    /* resolveEntity */
  NULL,    /* getEntity */
  NULL,    /* entityDecl */
  NULL,    /* notationDecl */
  NULL,    /* attributeDecl */
  NULL,    /* elementDecl */
  NULL,    /* unparsedEntityDecl */
  NULL,    /* setDocumentLocator */
  NULL,    /* startDocument */
  NULL,    /* endDocument */
  parser_startElement,  /* startElement */
  parser_endElement,  /* endElement */
  NULL,    /* reference */
  parser_characters,  /* characters */
  NULL,    /* ignorableWhitespace */
  NULL,    /* processingInstruction */
  NULL,    /* comment */
  NULL,    /* xmlParserWarning */
  NULL,    /* xmlParserError */
  NULL,    /* xmlParserError */
  NULL,    /* getParameterEntity */
  NULL,    /* cdataBlock; */
  NULL,    /* externalSubset; */
  1, NULL, NULL,  /* startElementNs */
  NULL,    /* endElementNs */
  NULL    /* xmlStructuredErrorFunc */
};

Report *
parser_get_report_from_file (gchar *filename)
{
  parser_state *state = parser_state_new ();

  if (xmlSAXUserParseFile(&handler, state, filename) < 0)
    {
      return NULL;
    } else {

      /* Not available before glib 2.12 */

#if GLIB_CHECK_VERSION(2,12,0)

      g_hash_table_remove_all (state->symbol_table);
      g_hash_table_remove_all (state->module_table);

#endif /* GLIB_CHECK_VERSION(2,12,0) */

      return state->report;
    }
}

Report *
parser_get_report_from_buffer (void *buffer, int length)
{
  parser_state *state = parser_state_new ();

  if (xmlSAXUserParseMemory(&handler, state, buffer, length) < 0)
    {
      return NULL;
    } else {
      /* Not available before glib 2.12 */

#if GLIB_CHECK_VERSION(2,12,0)

      g_hash_table_remove_all (state->symbol_table);
      g_hash_table_remove_all (state->module_table);

#endif /* GLIB_CHECK_VERSION(2,12,0) */

      return state->report;
    }
}

