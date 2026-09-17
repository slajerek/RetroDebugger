/** \file   resourcenumericstring.c
 * \brief   Numeric string connected to a resource
 *
 * Used for resources that store their numeric value as a string.
 *
 * Supports using suffixes 'K', 'M', and 'G' for KiB, MiB and GiB respectively.
 * Digits are accepted as well a navigation keys (left, right, home, end,
 * insert, delete) and Tab, Return and Enter to accept input.
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
#include <stdlib.h>

#include "resources.h"
#include "resourcewidgetmediator.h"
#include "vice_gtk3.h"

#include "types.h"
#include "resourcenumericstring.h"


/** \brief  CSS rule for the widget to show the current contents are invalid
 */
#define CSS_INVALID \
    "entry { color: red; }"

/** \brief  CSS rule for the widget to show the current contents are alid
 */
#define CSS_VALID \
    "entry { color: green; }"


/** \brief  Object holding unit (suffix, factor) pairs
 */
typedef struct numstr_unit_s {
    int         suffix; /**< suffix, in upper case */
    uint64_t    factor; /**< number to multiply numeric string with */
} numstr_unit_t;


/** \brief  Object holding additional widget info
 */
typedef struct numstr_info_s {
    GtkCssProvider *provider;
    guint64         minimum;
    guint64         maximum;
    gboolean        has_limits;
    gboolean        allow_zero;
} numstr_info_t;


/** \brief  List of suffixes and their factors
 */
static const numstr_unit_t units[] = {
    { 'K',  1<<10 },
    { 'M',  1<<20 },
    { 'G',  1<<30 },
#if 0
    { 'T',  1U<<40U },
#endif
    { -1,   0 }
};


/** \brief  List of valid keys for the widget
 *
 * This excludes GKD_Enter since that's used to attempt to set the entry.
 */
static const gint valid_keys[] = {
    GDK_KEY_0, GDK_KEY_1, GDK_KEY_2, GDK_KEY_3, GDK_KEY_4,
    GDK_KEY_5, GDK_KEY_6, GDK_KEY_7, GDK_KEY_8, GDK_KEY_9,
    GDK_KEY_k, GDK_KEY_K,   /* KiB */
    GDK_KEY_m, GDK_KEY_M,   /* MiB */
    GDK_KEY_g, GDK_KEY_G,   /* GiB */
    GDK_KEY_x, GDK_KEY_X,   /* 0x -> hex */
    GDK_KEY_BackSpace,
    GDK_KEY_Delete,
    GDK_KEY_Insert,
    GDK_KEY_Left,
    GDK_KEY_Right,
    GDK_KEY_Home,
    GDK_KEY_End,
    -1
};


/** \brief  Free additional data object
 *
 * Called during widget destruction by the mediator.
 *
 * \param[in]   info    info object to free
 */
static void free_info(void *info)
{
    numstr_info_t *i = info;
    if (i != NULL) {
        g_object_unref(G_OBJECT(i->provider));
        g_free(i);
    }
}

/** \brief  Check \a value against limits set on \a widget
 *
 * \param[in]   widget  resource numeric digits widget
 * \param[in]   value   64-bit unsigned value to check against limits
 *
 * \return  value is valid for the given (or missing) limits
 */
static gboolean value_is_valid(GtkEntry *self, guint64 value)
{
    numstr_info_t *info = mediator_get_data_w(GTK_WIDGET(self));

    if (info->has_limits) {
        if (info->allow_zero && value == 0) {
            return TRUE;
        }
        return (gboolean)(value >= info->minimum && value <= info->maximum);
    }
    return TRUE;
}

/** \brief  Validate input
 *
 * \param[in]   widget  GtkEntry instance
 *
 * \return  `TRUE` if the current text in \a self is valid
 */
static gboolean input_is_valid(GtkEntry *self)
{
    long long   result;
    char       *endptr;
    const char *text;
    guint64     factor = 1;

    text = gtk_entry_get_text(self);
    if (*text == '\0') {
        /* special: 0 means no fixed size */
        return TRUE;
    }

    result = strtoull(text, &endptr, 0);
    if (*endptr != '\0') {
        /* possible suffix*/
        int i;

        if (endptr == text) {
            /* suffix without preceeding digits, fail */
            return FALSE;
        }

        for (i = 0; units[i].factor > 0; i++) {
            if (g_ascii_toupper(*endptr) == units[i].suffix) {
                factor = units[i].factor;
                break;
            }
        }
        if (units[i].suffix < 0) {
            /* exhausted list of suffixes, fail */
            return FALSE;
        }

        if (endptr[1] != '\0') {
            /* text after suffix, fail */
            return FALSE;
        }
    }
    return value_is_valid(self, result * factor);
}

/** \brief  Update resource with text of entry
 *
 * Attempt to update the resource with the text of \a self, if that fails revert
 * to previous valid resource value.
 *
 * \param[in]   self    entry widget
 *
 * \return  `TRUE` on succesfully updating the resource
 */
static gboolean update_resource_value(GtkEntry *self)
{
    mediator_t *mediator;
    const char *entry_val;
    const char *res_val;

    mediator  = mediator_for_widget(GTK_WIDGET(self));
    entry_val = gtk_entry_get_text(self);
    res_val   = mediator_get_resource_string(mediator);
    if (g_strcmp0(entry_val, res_val) != 0) {
        if (!mediator_update_string(mediator, entry_val)) {
            /* revert */
            gtk_entry_set_text(self, res_val);
            return FALSE;
        }
    }
    return TRUE;
}

/** \brief  Event handler for the 'changed' event
 *
 * Gets triggered after accepting/refusing any key input. so we check the input
 * for validity here, and also add a visual hint via CSS.
 *
 * \param[in,out]   self    entry widget
 * \param[in]       data    extra event data (unused)
 */
static void on_entry_changed(GtkEntry *self, gpointer data)
{
    numstr_info_t  *info;
    GtkCssProvider *provider;

    info = mediator_get_data_w(GTK_WIDGET(self));
    provider = info->provider;

    if (!input_is_valid(self)) {
        vice_gtk3_css_provider_add(GTK_WIDGET(self), provider);
    } else {
        vice_gtk3_css_provider_remove(GTK_WIDGET(self), provider);
    }
}

/** \brief  Handler for the "focus-out" event
 *
 * \param[in]   self    entry widget (unused)
 * \param[in]   event   event object (unused)
 * \param[in]   data    extra event data (unused)
 *
 * \return  GDK_EVENT_PROPAGATE
 */
static gboolean on_focus_out_event(GtkEntry *self,
                                   GdkEvent *event,
                                   gpointer  data)
{
    update_resource_value(self);
    return GDK_EVENT_PROPAGATE;
}


/** \brief  Handler for the 'on-key-press' event
 *
 * Filter certain keys from being used in the entry widget.
 *
 * \param[in]   self    entry widget
 * \param[in]   event   event object
 * \param[in]   data    unused
 *
 * \return  TRUE if Enter was pushed, FALSE otherwise (makes the pushed key
 *          propagate to the entry)
 */
static gboolean on_key_press_event(GtkEntry *self,
                                   GdkEvent *event,
                                   gpointer  data)
{
    GdkEventKey *keyev = (GdkEventKey *)event;

    if (keyev->type == GDK_KEY_PRESS) {
        gint keyval = keyev->keyval;

        /* Shift+Tab is mapped by X11 to Iso_Left_Tab for some reason, so it's
         * not caught with GDK_KEY_Tab */
        if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter ||
                keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab) {
            update_resource_value(self);
            return GDK_EVENT_PROPAGATE; /* propagete further */
        } else {
            int i;

            for (i = 0; valid_keys[i] >= 0; i++) {
                if (valid_keys[i] == keyval) {
                    return GDK_EVENT_PROPAGATE; /* we accept this key */
                }
            }
            /* we don't accept this key */
            return GDK_EVENT_STOP;
        }
    }
    return GDK_EVENT_PROPAGATE;
}


/** \brief  Create numeric string entry box for \a resource
 *
 * Create a widget that accepts numeric strings with suffixes (K, M, G).
 *
 * \param[in]   resource    resource name
 *
 * \return  GtkEntry
 */
GtkWidget *vice_gtk3_resource_numeric_string_new(const char *resource)
{
    GtkWidget      *entry;
    mediator_t     *mediator;
    numstr_info_t  *info;
    GtkCssProvider *provider;

    entry    = gtk_entry_new();
    mediator = mediator_new(entry, resource, G_TYPE_STRING);
    provider = vice_gtk3_css_provider_new(CSS_INVALID);
    info     = g_malloc0(sizeof *info);
    info->provider   = provider;
    info->minimum    = 0;
    info->maximum    = G_MAXUINT64;
    info->has_limits = FALSE;
    info->allow_zero = TRUE;
    mediator_set_data(mediator, info, free_info);

    /* set entry to resource value */
    gtk_entry_set_text(GTK_ENTRY(entry),
                       mediator_get_resource_string(mediator));
    /* set preferrence to upper case (doesn't work for shit) */
    gtk_entry_set_input_hints(GTK_ENTRY(entry), GTK_INPUT_HINT_UPPERCASE_CHARS);

    g_signal_connect(entry,
                     "changed",
                     G_CALLBACK(on_entry_changed),
                     NULL);
    g_signal_connect(entry,
                     "key-press-event",
                     G_CALLBACK(on_key_press_event),
                     NULL);
    g_signal_connect(entry,
                     "focus-out-event",
                     G_CALLBACK(on_focus_out_event),
                     NULL);

    gtk_widget_show_all(entry);
    return entry;
}


/** \brief  Set limits on the widget's valid values
 *
 * These limits are by default set to 0 to UINT64_MAX. The \a allow_zero argument
 * is meant to allow using 0 to indicate a special case.
 *
 * \param[in,out]   widget      resource numeric string widget
 * \param[in]       minimum     minimum value
 * \param[in]       maximum     maximum value
 * \param[in]       allow_zero  allow zero as a special (Nul) value
 */
void vice_gtk3_resource_numeric_string_set_limits(GtkWidget *widget,
                                                  guint64    minimum,
                                                  guint64    maximum,
                                                  gboolean   allow_zero)
{
    numstr_info_t *info;

    info = mediator_get_data_w(widget);
    info->minimum    = minimum;
    info->maximum    = maximum;
    info->has_limits = TRUE;
    info->allow_zero = allow_zero;
}
