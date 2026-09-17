/** \file   csshelpers.c
 *  \brief  Helper/wrapper functions for using CSS in Gtk3 code
 *
 *  This file contains some CSS-related functions to avoid boilerplate code in
 *  other files.
 *
 *  \author Bas Wassink <b.wassink@ziggo.nl>
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
#include "log.h"

#include "csshelpers.h"


/** \brief  Create a new CSS provider and set it to \a css
 *
 * Use this function if you need a CSS provider you'll apply multiple times,
 * if you need to apply CSS to a widget only once, there's a wrapper function
 * #vice_gtk3_css_add().
 *
 * \param[in]   css CSS string
 *
 * \return  new provider or NULL on error
 */
GtkCssProvider *vice_gtk3_css_provider_new(const char *css)
{
    GtkCssProvider *provider;
    GError *err = NULL;

    /* instanciate CSS provider */
    provider = gtk_css_provider_new();
    /* attempt to load CSS from string */
    gtk_css_provider_load_from_data(provider, css, -1, &err);
    if (err != NULL) {
        log_error(LOG_DEFAULT, "CSS error: %s", err->message);
        g_error_free(err);
        return NULL;
    }
    return provider;
}


/** \brief  Add CSS \a provider to \a widget
 *
 * \param[in,out]   widget      widget to add \a provider to
 * \param[in]       provider    CSS provider
 *
 * \return  bool
 */
gboolean vice_gtk3_css_provider_add(GtkWidget *widget,
                                    GtkCssProvider *provider)
{
    GtkStyleContext *context;

    /* try to get style context */
    context = gtk_widget_get_style_context(widget);
    if (context == NULL) {
        log_error(LOG_DEFAULT, "Couldn't get style context of widget");
        /* don't free the context, it's owned by the widget */
        return FALSE;
    }

    /* add provider */
    gtk_style_context_add_provider(context,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);
    return TRUE;
}


/** \brief  Remove \a provider from \a widget
 *
 * \param[in,out]   widget      gtk widget
 * \param[in]       provider    gtk css provider to remove
 *
 * \return  gboolean
 */
gboolean vice_gtk3_css_provider_remove(GtkWidget *widget,
                                       GtkCssProvider *provider)
{
    GtkStyleContext *context;

    /* try to get style context */
    context = gtk_widget_get_style_context(widget);
    if (context == NULL) {
        log_error(LOG_DEFAULT, "Couldn't get style context of widget");
        /* don't free the context, it's owned by the widget */
        return FALSE;
    }

    gtk_style_context_remove_provider(context, GTK_STYLE_PROVIDER(provider));
    return TRUE;
}



/** \brief  Add CSS string \a css to \a widget
 *
 * Only use this when adding CSS to a single widget, if you need to add the
 * same CSS to multiple widgets use #vice_gtk3_css_provider_new() to create
 * a CSS provider once and #vice_gtk3_css_provider_add() to add it multiple
 * times.
 *
 * \param[in,out]   widget  widget to add CSS to
 * \param[in]       css     CSS string
 *
 * \return  bool
 */
gboolean vice_gtk3_css_add(GtkWidget *widget, const char *css)
{
    GtkCssProvider *provider;

    provider = vice_gtk3_css_provider_new(css);
    if (provider != NULL) {
        return vice_gtk3_css_provider_add(widget, provider);
    }
    return FALSE;
}
