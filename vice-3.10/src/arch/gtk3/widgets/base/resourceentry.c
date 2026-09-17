/** \file   resourceentry.c
 * \brief   Text entry connected to a resource
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
#include "resourcewidgetmediator.h"

#include "resourceentry.h"


/*
 * Resource entry box that only responds to 'full' changes
 *
 * This entry box only updates its resource when either Enter is pressed or
 * the widget looses focus (user pressing tab, clicking somewhere else). This
 * avoids setting the connected resource with every key pressed, resulting, for
 * example, in rom files 'a', 'ab', 'abc' and 'abcd' while entering 'abcd'.
 */

/** \brief  Update bound resource
 *
 * When \a text differs from the resource value (or when the GtkEntry's text
 * differs in case \a text is `NULL`) we update the resource and store the new
 * value as the current valid value in the mediator.
 *
 * \param[in]   entry   full resource entry box
 * \param[in]   text    text for resource, pass `NULL` to get text from \a entry
 *
 * \return  TRUE when the entry was updated
 */
static gboolean resource_entry_update_resource(GtkEntry *entry, const char *text)
{
    mediator_t *mediator;
    const char *res_val;

    mediator  = mediator_for_widget(GTK_WIDGET(entry));
    res_val   = mediator_get_resource_string(mediator);
    if (text == NULL) {
        text = gtk_entry_get_text(entry);
    }
    if (res_val == NULL) {
        res_val = "";
    }
    if (g_strcmp0(res_val, text) != 0) {
        if (!mediator_update_string(mediator, text)) {
            /* failed, revert */
            const char *current = mediator_get_current_string(mediator);
            gtk_entry_set_text(entry, current != NULL ? current : "");
            return FALSE;
        }
    }
    return TRUE;
}

/** \brief  Handler for the 'focus-out' event
 *
 * \param[in,out]   entry   entry box
 * \param[in]       event   event object (unused)
 * \param[in]       data    extra event data (unused)
 *
 * \return  FALSE to propagate the event further
 */
static gboolean on_focus_out_event(GtkEntry *entry,
                                   GdkEvent *event,
                                   gpointer  data)
{
    resource_entry_update_resource(entry, NULL);
    return FALSE;
}

/** \brief  Handler for the 'on-key-press' event
 *
 * \param[in,out]   entry   entry box
 * \param[in]   event   event object
 * \param[in]   data    unused
 *
 * \return  TRUE if Enter was pushed, FALSE otherwise (makes the pushed key
 *          propagate to the entry)
 */
static gboolean on_key_press_event(GtkEntry *entry,
                                   GdkEvent *event,
                                   gpointer  data)
{
    GdkEventKey *keyev = (GdkEventKey *)event;

    if (keyev->type == GDK_KEY_PRESS && keyev->keyval == GDK_KEY_Return) {
        /*
         * We handled Enter/Return for Gtk3/GLib, whether or not the
         * resource actually gets updated is another issue.
         */
        resource_entry_update_resource(entry, NULL);
        return TRUE;
    }
    return FALSE;
}


/** \brief  Create resource entry box that only reacts to 'full' entries
 *
 * Creates a resource-connected entry box that only updates the resource when
 * the either the widget looses focus (due to Tab or mouse click somewhere else
 * in the UI) or when the user presses 'Enter'. This behaviour differs from the
 * other resource entry which updates its resource on every key press.
 *
 * \param[in]   resource    resource name
 *
 * \return  GtkEntry
 */
GtkWidget *vice_gtk3_resource_entry_new(const char *resource)
{
    GtkWidget  *entry;
    mediator_t *mediator;
    const char *current;

    entry    = gtk_entry_new();
    mediator = mediator_new(entry, resource, G_TYPE_STRING);
    current  = mediator_get_current_string(mediator);
    /* debug_gtk3("resource = '%s', current = '%s'", resource, current); */
    gtk_entry_set_text(GTK_ENTRY(entry), current != NULL ? current : "");

    g_signal_connect(entry,
                     "focus-out-event",
                     G_CALLBACK(on_focus_out_event),
                     NULL);
    g_signal_connect(entry,
                     "key-press-event",
                     G_CALLBACK(on_key_press_event),
                     NULL);

    return entry;
}


/** \brief  Create resource entry box that only reacts to 'full' entries
 *
 * Creates a resource-connected entry box that only updates the resource when
 * the either the widget looses focus (due to Tab or mouse click somewhere else
 * in the UI) or when the user presses 'Enter'. This behaviour differs from the
 * other resource entry which updates its resource on every key press.
 *
 * This is a variant of vice_gtk3_resource_entry_new() that allows using
 * a printf format string to specify the resource name.
 *
 * \param[in]   fmt     resource name format string (printf-style)
 * \param[in]   ...     format string arguments
 *
 * \return  GtkEntry
 */
GtkWidget *vice_gtk3_resource_entry_new_sprintf(const char *fmt, ...)
{
    char     resource[256];
    va_list  args;

    va_start(args, fmt);
    g_vsnprintf(resource, sizeof resource, fmt, args);
    va_end(args);
    return vice_gtk3_resource_entry_new(resource);
}


/** \brief  Reset the widget to the original resource value
 *
 * Resets the widget and the connect resource to the value the resource
 * contained when the widget was created.
 *
 * \param[in,out]   widget  resource entry box
 *
 * \return  TRUE when the entry was reset to its original value
 */
gboolean vice_gtk3_resource_entry_reset(GtkWidget *widget)
{
    mediator_t *mediator;
    const char *initial;

    mediator = mediator_for_widget(widget);
    initial  = mediator_get_initial_string(mediator);
    return vice_gtk3_resource_entry_set(widget, initial);
}


/** \brief  Set new string for resource entry
 *
 * Sets \a value as the new text for \a widget and also updates the connected
 * resource. Returns TRUE on success, FALSE on failure. It is assumed that a
 * failure to set a resource in only due to some registered resource handler
 * failing, not due to an invalid resource name.
 *
 * \param[in,out]   widget  resource entry
 * \param[in]       text    new text for \a entry
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_entry_set(GtkWidget *widget, const char *text)
{
    if (text == NULL) {
        text = "";
    }
    gtk_entry_set_text(GTK_ENTRY(widget), text);
    return resource_entry_update_resource(GTK_ENTRY(widget), text);
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  resource entry
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_entry_sync(GtkWidget *widget)
{
    mediator_t *mediator;
    const char *res_val;

    mediator = mediator_for_widget(widget);
    res_val  = mediator_get_resource_string(mediator);

    /* no need to block, events won't trigger */
    gtk_entry_set_text(GTK_ENTRY(widget), res_val == NULL ? "" : res_val);
    return TRUE;
}


/** \brief  Reset \a entry to its resource factory value
 *
 * \param[in]   widget  resource entry
 *
 * \return  TRUE when the entry was restored to its factory value
 */
gboolean vice_gtk3_resource_entry_factory(GtkWidget *widget)
{
    mediator_t *mediator;
    const char *factory;

    mediator = mediator_for_widget(widget);
    factory  = mediator_get_factory_string(mediator);
    return vice_gtk3_resource_entry_set(widget, factory);
}
