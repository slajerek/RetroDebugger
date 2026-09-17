/** \file   resourcecheckbutton.c
 * \brief   Check button bound to a resource
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This widget presents a check button that controls a boolean resource. During
 * construction the current resource value is stored in the widget to allow
 * resetting to default.
 *
 * \section resource_check_button_example
 * \code{.c}
 *
 *  GtkWidget *check;
 *
 *  // create a widget
 *  check = vice_gtk3_resource_check_button_create("SomeResource");
 *  // any state change of the widget will now update the resource
 *
 *  // restore widget & resource to their initial state
 *  vice_gtk3_resource_check_button_reset(check);
 *
 *  // restore widget & resource to the resource's factory value
 *  vice_gtk3_resource_check_button_factory(check);
 *
 * \endcode
 *
 * Extra GObject data used:
 * (do not overwrite these unless you know what you're doing)
 *
 *  - set via mediator_new(): "ResourceMeditator"
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
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "resourcehelpers.h"
#include "resourcewidgetmediator.h"

#include "resourcecheckbutton.h"


/** \brief  Handler for the 'toggled' event of the check button
 *
 * \param[in]   self    check button
 * \param[in]   data    extra event data (unused)
 */
static void on_check_button_toggled(GtkWidget *self, gpointer data)
{
    gboolean    active;
    mediator_t *mediator;

    mediator = mediator_for_widget(self);
    active   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self));
    /* set resource, update state and trigger callback if present */
    if (!mediator_update_boolean(mediator, active)) {
        /* revert check button to previous state */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self),
                                     mediator_get_current_boolean(mediator));
    }
}


/** \brief  Create check button to toggle \a resource
 *
 * Creates a check button to toggle \a resource. Makes a heap-allocated copy
 * of the resource name so initializing this widget with a constructed/temporary
 * resource name works as well.
 *
 * \param[in]   resource    resource name
 * \param[in]   label       label of the check button (optional)
 *
 * \note    The resource name is stored in the "ResourceName" property.
 *
 * \return  new check button
 */
GtkWidget *vice_gtk3_resource_check_button_new(const char *resource,
                                               const char *label)
{
    GtkWidget  *check;
    mediator_t *mediator;
    gulong      handler;

    /* make label optional */
    if (label != NULL) {
        check = gtk_check_button_new_with_label(label);
    } else {
        check = gtk_check_button_new();
    }

    /* initialize and attach mediator */
    mediator = mediator_new(check, resource, G_TYPE_BOOLEAN);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                 mediator_get_initial_boolean(mediator));

    /* connect signal handler and register with mediator so the signal can
     * be blocked and unblocked through the mediator */
    handler = g_signal_connect(check,
                               "toggled",
                               G_CALLBACK(on_check_button_toggled),
                                NULL);
    mediator_set_handler(mediator, handler);
    return check;
}


/** \brief  Create check button to toggle a resource
 *
 * Creates a check button to toggle a resource. Makes a heap-allocated copy
 * of the resource name so initializing this widget with a constructed/temporary
 * resource name works as well. The resource name can be constructed with a
 * printf() format string.
 *
 * \param[in]   fmt         resource name format string
 * \param[in]   label       label of the check button and optional printf args
 *
 * \note    The resource name is stored in the "ResourceName" property.
 *
 * \return  new check button
 */
GtkWidget *vice_gtk3_resource_check_button_new_sprintf(const char *fmt,
                                                       const char *label,
                                                       ...)
{
    char    resource[256];
    va_list args;

    va_start(args, label);
    g_vsnprintf(resource, sizeof resource, fmt, args);
    va_end(args);
    return vice_gtk3_resource_check_button_new(resource, label);
}


/** \brief  Set new \a value for \a widget
 *
 * \param[in,out]   widget  resource check button widget
 * \param[in]       value   new value
 *
 * \return  TRUE
 */
gboolean vice_gtk3_resource_check_button_set(GtkWidget *widget, gboolean value)
{
    /* the 'toggled' event handler will update the resouce if required */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
    return TRUE;
}


/** \brief  Reset check button to factory state
 *
 * \param[in,out]   widget  resource check button widget
 *
 * \return  TRUE if the resource was reset to its factory value
 */
gboolean vice_gtk3_resource_check_button_factory(GtkWidget *widget)
{
    mediator_t *mediator;
    gboolean    factory;

    mediator = mediator_for_widget(widget);
    factory  = mediator_get_factory_boolean(mediator);
    return vice_gtk3_resource_check_button_set(widget, factory);
}


/** \brief  Reset \a widget to the state it was when it was created
 *
 * \param[in,out]   widget  resource check button widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_check_button_reset(GtkWidget *widget)
{
    mediator_t *mediator;
    gboolean    initial;

    mediator = mediator_for_widget(widget);
    initial  = mediator_get_initial_boolean(mediator);
    return vice_gtk3_resource_check_button_set(widget, initial);
}


/** \brief  Synchronize the \a widget state with its resource
 *
 * \param[in,out]   widget  resource check button
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_check_button_sync(GtkWidget *widget)
{
    mediator_t *mediator;
    gboolean    value;
    gboolean    active;

    mediator = mediator_for_widget(widget);
    value    = mediator_get_resource_boolean(mediator);
    active   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (active != value) {
        /* block signal handler to avoid triggering useless resource update */
        mediator_handler_block(mediator);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
        mediator_handler_unblock(mediator);
    }
    return TRUE;
}


/** \brief  Add user callback to resource checkbutton
 *
 * \param[in]   widget      resource checkbutton
 * \param[in]   callback    function to call when the checkbutton state changes
 */
void vice_gtk3_resource_check_button_add_callback(GtkWidget *widget,
                                                  void (*callback)(GtkWidget*, gboolean))
{
    mediator_set_callback_boolean_w(widget, callback);
}
