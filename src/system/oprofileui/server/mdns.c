/*
 * OProfile User Interface
 *
 * Copyright (C) 2007 Nokia Corporation. All rights reserved.
 *
 * Author: Ross Burton <ross@openedhand.com>
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
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

/* Bit nasty */
static int service_port;
static char *name;
static AvahiEntryGroup *group = NULL;

static void create_services (AvahiClient *c);

static void
entry_group_callback (AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata) {

  g_assert (g == group || group == NULL);

  /* Called whenever the entry group state changes */
  
  switch (state) {
  case AVAHI_ENTRY_GROUP_ESTABLISHED :
    /* The entry group has been established successfully */
    break;
    
  case AVAHI_ENTRY_GROUP_COLLISION : {
    char *n;
    
    /* A service name collision happened. Let's pick a new name */
    n = avahi_alternative_service_name (name);
    g_free (name);
    name = n;
    
    /* And recreate the services */
    create_services (avahi_entry_group_get_client (g));
    break;
  }
    
  case AVAHI_ENTRY_GROUP_FAILURE :
    g_warning ("Entry group failure: %s",
               avahi_strerror (avahi_client_errno (avahi_entry_group_get_client (g))));
    break;

  case AVAHI_ENTRY_GROUP_UNCOMMITED:
  case AVAHI_ENTRY_GROUP_REGISTERING:
    ;
  }
}

static void
create_services (AvahiClient *c)
{
  int ret;

  g_assert (c);

  if (name == NULL) {
    /* Set a default name */
    name = g_strdup_printf ("OProfile on %s", avahi_client_get_host_name (c));
  }

  /* If this is the first time we're called, let's create a new entry group */
  if (group == NULL)

    if (! (group = avahi_entry_group_new (c, entry_group_callback, NULL))) {
      g_warning ("avahi_entry_group_new() failed: %s", avahi_strerror (avahi_client_errno (c)));
      return;
    }
  
  if ((ret = avahi_entry_group_add_service (group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0,
                                            name,
                                           "_oprofile._tcp", NULL, NULL, service_port, NULL)) < 0) {
    g_warning ("Failed to add _oprofile._tcp service: %s", avahi_strerror (ret));
    return;
  }
  
  /* Tell the server to register the service */
  if ((ret = avahi_entry_group_commit (group)) < 0) {
    g_warning ("Failed to commit entry_group: %s", avahi_strerror (ret));
    return;
  }
}

/* Called whenever the client or server state changes */
static void
avahi_client_callback(AvahiClient *c, AvahiClientState state, void * userdata)
{
  g_assert (c);
  
  switch (state) {
  case AVAHI_CLIENT_S_RUNNING:
    /* The server has startup successfully and registered its host
     * name on the network, so it's time to create our services */
    if (!group)
      create_services (c);
    break;
    
  case AVAHI_CLIENT_FAILURE:
    g_warning ("Avahi error: %s", avahi_strerror (avahi_client_errno (c)));
    break;
    
  case AVAHI_CLIENT_S_COLLISION:
    /* Let's drop our registered services. When the server is back in
     * AVAHI_SERVER_RUNNING state we will register them again with the new host
     * name. */
  case AVAHI_CLIENT_S_REGISTERING:
    /* The server records are now being established. This might be caused by a
     * host name change. We need to wait for our own records to register until
     * the host name is properly esatblished. */
    if (group)
      avahi_entry_group_reset (group);
    break;
    
  case AVAHI_CLIENT_CONNECTING:
    break;
  }
}

void
mdns_publish (const int port)
{
  int error;
  const AvahiPoll *poll_api;
  AvahiGLibPoll *glib_poll;
  AvahiClient *client = NULL;

  /* Save the port number for later */
  service_port = port;

  /* Avahi, please use the GLib allocator */
  avahi_set_allocator (avahi_glib_allocator ());

  /* Tie Avahi intp the GLib main loop */
  glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
  poll_api = avahi_glib_poll_get (glib_poll);
    
  /* Create the Avahi client, which will register the record for us */
  client = avahi_client_new (poll_api, 0, avahi_client_callback, NULL, &error);
  if (!client) {
    g_warning ("Failed to create avahi client: %s", avahi_strerror (error));
    return;
  }
}
