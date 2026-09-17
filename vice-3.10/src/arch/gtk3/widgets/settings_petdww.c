/** \file   settings_petdww.c
 * \brief   Settings widget for PET DWW hi-res graphics
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES PETDWW          xpet
 * $VICERES PETDWWfilename  xpet
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

#include "resources.h"
#include "vice_gtk3.h"

#include "settings_petdww.h"


/* references to widgets to be able to toggle sensitive state depending on the
 * DWW Enable check button
 */

/** \brief  DWW image file chooser widget */
static GtkWidget *chooser = NULL;


/** \brief  Handler for the 'toggled' event of the DWW Enable check button
 *
 * Toggles sensitive state of other widgets
 *
 * \param[in]   widget      DWW check button
 * \param[in]   user_data   unused
 */
static void on_dww_toggled(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *parent;
    gboolean   active;
    int        io_size = 0;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    resources_get_int("IOSize", &io_size);
    parent = gtk_widget_get_toplevel(widget);
    if (!GTK_IS_WINDOW(widget)) {
        parent = NULL;  /* default to current emulator window */
    }

    /* only enable when I/O size is 2048 bytes */
    if (active && (io_size < 2048)) {
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                "Cannot enable DWW",
                                "To be able to use DWW, the I/O size of the machine "
                                " needs to be 2048 bytes."
                                " The current I/O size is %d bytes.\n\n"
                                "Use the model settings dialog to set I/O size",
                                io_size);
        active = FALSE;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
    } else {
        resources_set_int("PETDWW", active ? 1 : 0);
    }

    gtk_widget_set_sensitive(chooser, active);
}


/** \brief  Create PETDWW Enable check button
 *
 * Checks if the I/O size is 2048 bytes before setting the resource to TRUE and
 * pops up an error message if the I/O size less than 2048 bytes.
 * Also toggles the sensitivity of the text entry and browse button.
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_dww_check_button(void)
{
    GtkWidget *check;
    int        enabled = 0;
    int        io_size = 0;

    check = gtk_check_button_new_with_label("Enable DWW hi-res graphics");

    resources_get_int("PETDWW", &enabled);
    resources_get_int("IOSize", &io_size);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                 (enabled && io_size >= 2048));

    g_signal_connect(G_OBJECT(check),
                     "toggled",
                     G_CALLBACK(on_dww_toggled), NULL);
    return check;
}


/** \brief  Create widget to control PET DWW settings
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_petdww_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *label;
    gboolean   active;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    /* DWW enable */
    enable = create_dww_check_button();
    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 2, 1);

    /* DWW filename */
    label = gtk_label_new("DWW RAM image file");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    chooser = vice_gtk3_resource_filechooser_new("PETDWWfilename",
                                                 GTK_FILE_CHOOSER_ACTION_SAVE);
    vice_gtk3_resource_filechooser_set_custom_title(chooser,
                                                    "Select DWW image file");
    gtk_grid_attach(GTK_GRID(grid), label,   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chooser, 1, 1, 1, 1);

    /* set initial sensitive state of widgets */
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enable));
    gtk_widget_set_sensitive(chooser, active);

    gtk_widget_show_all(grid);
    return grid;
}
