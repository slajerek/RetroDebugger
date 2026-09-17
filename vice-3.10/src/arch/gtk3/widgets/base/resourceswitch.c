/** \file   resourceswitch.c
 * \brief   GtkSwitch button connected to a resource
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This widget presents a check button that controls a boolean resource. During
 * construction the current resource value is stored in the widget to allow
 * resetting to default.
 *
 * \code{.c}
 *
 *  GtkWidget *sw;
 *
 *  // create a widget
 *  sw = vice_gtk3_resource_switch_new("SomeResource");
 *  // any state change of the widget will now update the resource
 *
 *  // restore widget & resource to their initial state
 *  vice_gtk3_resource_switch_reset(sw);
 *
 * \endcode
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include <gtk/gtk.h>
#include <stdarg.h>

#include "debug_gtk3.h"
#include "log.h"
#include "resources.h"
#include "resourcehelpers.h"
#include "resourcewidgetmediator.h"

#include "resourceswitch.h"


/** \brief  Handler for the 'state-set' event of the switch
 *
 * \param[in]   widget  resource switch
 * \param[in]   data    extra event data (unused)
 */
static void on_switch_notify_active(GtkWidget *self, gpointer data)
{
    gboolean    active;
    mediator_t *mediator;

    active   = gtk_switch_get_active(GTK_SWITCH(self));
    mediator = mediator_for_widget(self);
    if (!mediator_update_boolean(mediator, active)) {
        gtk_switch_set_active(GTK_SWITCH(self),
                              mediator_get_current_boolean(mediator));
    }
}


/** \brief  Create switch to toggle \a resource
 *
 * Creates a switch to toggle \a resource. Makes a heap-allocated copyof the
 * resource name so that initializing this widget with a constructed/temporary
 * resource name works as well.
 *
 * \param[in]   resource    resource name
 *
 * \note    The resource name is stored in the "ResourceName" property.
 *
 * \return  GtkSwitch
 */
GtkWidget *vice_gtk3_resource_switch_new(const char *resource)
{
    GtkWidget  *widget;
    mediator_t *mediator;
    gulong      handler;

    widget = gtk_switch_new();
    gtk_widget_set_hexpand(widget, FALSE);
    gtk_widget_set_vexpand(widget, FALSE);

    mediator = mediator_new(widget, resource, G_TYPE_BOOLEAN);
    gtk_switch_set_active(GTK_SWITCH(widget),
                          mediator_get_current_boolean(mediator));

    /* connect signal handler and store ID in mediator */
    handler = g_signal_connect(widget,
                               "notify::active",
                               G_CALLBACK(on_switch_notify_active),
                               NULL);
    mediator_set_handler(mediator, handler);

    return widget;
}


/** \brief  Create switch to toggle a resource
 *
 * Creates a switch to toggle a resource. Makes a heap-allocated copy
 * of the resource name so initializing this widget with a constructed/temporary
 * resource name works as well. The resource name can be constructed with a
 * printf() format string.
 *
 * \param[in]   fmt         resource name format string
 *
 * \note    The resource name is stored in the "ResourceName" property.
 *
 * \return  new switch
 */
GtkWidget *vice_gtk3_resource_switch_new_sprintf(const char *fmt, ...)
{
    char    resource[256];
    va_list args;

    va_start(args, fmt);
    g_vsnprintf(resource, sizeof resource, fmt, args);
    va_end(args);
    return vice_gtk3_resource_switch_new(resource);
}


/** \brief  Set new \a value for \a widget
 *
 * \param[in,out]   widget  switch
 * \param[in]       value   new value
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_switch_set(GtkWidget *widget, gboolean value)
{
    gtk_switch_set_active(GTK_SWITCH(widget), value);
    return TRUE;
}


/** \brief  Reset state to the value during instanciation
 *
 * \param[in,out]   widget  resource switch
 *
 * \return  TRUE if the widget was reset to its original state
 */
gboolean vice_gtk3_resource_switch_reset(GtkWidget *widget)
{
    mediator_t *mediator;
    gboolean    initial;

    mediator = mediator_for_widget(widget);
    initial  = mediator_get_initial_boolean(mediator);
    return vice_gtk3_resource_switch_set(widget, initial);
}


/** \brief  Reset switch to factory value
 *
 * \param[in,out]   widget  check button
 *
 * \return  TRUE if the widget was set to its factory value
 */
gboolean vice_gtk3_resource_switch_factory(GtkWidget *widget)
{
    mediator_t *mediator;
    gboolean    factory;

    mediator = mediator_for_widget(widget);
    factory  = mediator_get_factory_boolean(mediator);
    return vice_gtk3_resource_switch_set(widget, factory);
}


/** \brief  Synchronize \a widget with its associated resource value
 *
 * \param[in,out]   widget  resource switch widget
 *
 * \return  TRUE if the widget was synchronized
 */
gboolean vice_gtk3_resource_switch_sync(GtkWidget *widget)
{
    mediator_t *mediator;
    gboolean    value;
    gboolean    active;

    mediator = mediator_for_widget(widget);
    value    = mediator_get_resource_boolean(mediator);
    active   = gtk_switch_get_active(GTK_SWITCH(widget));
    if (active != value) {
        /* block signal handler to avoid triggering useless resource update */
        mediator_handler_block(mediator);
        gtk_switch_set_active(GTK_SWITCH(widget), value);
        mediator_handler_unblock(mediator);
    }
    return TRUE;
}


/** \brief  Add user callback to resource switch
 *
 * \param[in]   widget      resource switch
 * \param[in]   callback    function to call on succesfull resource changes
 */
void vice_gtk3_resource_switch_add_callback(GtkWidget *widget,
                                            void (*callback)(GtkWidget*, gboolean))
{
    mediator_set_callback_boolean_w(widget, callback);
}
