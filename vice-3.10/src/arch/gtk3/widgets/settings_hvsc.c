/** \file   settings_hvsc.c
 * \brief   High Voltage SID Collection settings widget for VSID
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

/*
 * $VICERES HVSCRoot    vsid
 */

#include "vice.h"
#include <gtk/gtk.h>
#include <stdlib.h>

#include "debug_gtk3.h"
#include "hvsc.h"
#include "resourceentry.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_hvsc.h"


/** \brief  Reference to the entry box for the HVSC root dir
 *
 * This is a "full" resource entry widget, which means the resource is only
 * updated when the user pushes Enter or the widget looses focus.
 */
static GtkWidget *hvsc_root_entry;


/** \brief  Callback for the directory-select dialog
 *
 * \param[in]   dialog      directory-select dialog
 * \param[in]   filename    filename (NULL if canceled)
 * \param[in]   param       extra data (unused)
 */
static void browse_callback(GtkDialog *dialog, gchar *filename, gpointer param)
{
    if (filename != NULL) {
        vice_gtk3_resource_entry_set(GTK_WIDGET(hvsc_root_entry), filename);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'clicked' event of the HVSC root "browse" button
 *
 * \param[in]   widget  browse button (ignored)
 * \param[in]   data    extra event data (ignored)
 */
static void on_browse_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget  *dialog;
    const char *current = NULL;

    /* try to get the current HVSC root dir */
    resources_get_string("HVSCRoot", &current);

    /* pop up dialog */
    dialog = vice_gtk3_select_directory_dialog("Select HVSC root directory",
                                               current,
                                               FALSE,
                                               NULL,
                                               browse_callback,
                                               NULL);
    gtk_widget_show(dialog);
}


/** \brief  Create HVSC settings widget
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_hvsc_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *browse;
    gchar buffer[1024];
    char *hvsc_base = getenv("HVSC_BASE");

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    label = gtk_label_new("HVSC root directory");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    hvsc_root_entry = vice_gtk3_resource_entry_new("HVSCRoot");
    gtk_widget_set_hexpand(hvsc_root_entry, TRUE);
    browse = gtk_button_new_with_label("Browse ...");

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hvsc_root_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), browse, 2, 0, 1, 1);

    g_snprintf(buffer,
               sizeof(buffer),
               "Leave empty to use the <tt>HVSC_BASE</tt> environment variable.\n"
               "(Current value: <tt>%s</tt>)",
               hvsc_base != NULL && *hvsc_base != '\0' ? hvsc_base : "&lt;none&gt;");
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), buffer);
    gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 2, 1);

    g_signal_connect(browse, "clicked", G_CALLBACK(on_browse_clicked), NULL);

    gtk_widget_show_all(grid);
    return grid;
}
