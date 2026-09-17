/** \file   petiosizewidget.c
 * \brief   Widget to set the PET I/O area size (PET 8296 only)
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES IOSize  xpet
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

#include "vice_gtk3.h"
#include "resources.h"

#include "petiosizewidget.h"


/** \brief  Available I/O sizes
 */
static const vice_gtk3_radiogroup_entry_t io_sizes[] = {
    { "256 bytes", 256 },
    { "2048 bytes", 2048 },
    { NULL, -1 }
};


/** \brief  Function to call on 'toggle' events of the radio buttons
 *
 * Allows the PET settings glue logic to update related widgets.
 */
static void (*user_callback)(int) = NULL;


/** \brief  Handler for the 'toggled' event of the radio buttons
 *
 * Triggers the optional callback with the current radio button ID.
 *
 * \param[in]   widget  radio button (unused)
 * \param[in]   id      ID of the radio button
 */
static void on_io_size_changed(GtkWidget *widget, int id)
{
    if (user_callback != NULL) {
        user_callback(id);
    }
}


/** \brief  Create PET I/O area size widget
 *
 * \return  GtkGrid
 */
GtkWidget *pet_io_size_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *group;

    user_callback = NULL;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "I/O area size", 1);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    group = vice_gtk3_resource_radiogroup_new("IOSize", io_sizes,
            GTK_ORIENTATION_VERTICAL);
    vice_gtk3_resource_radiogroup_add_callback(group, on_io_size_changed);
    gtk_widget_set_margin_start(group, 16);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set callback for radio button 'toggle' events
 *
 * \param[in]   func    function to trigger
 */
void pet_io_size_widget_set_callback(void (*func)(int))
{
    user_callback = func;
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  PET I/O size widget
 */
void pet_io_size_widget_sync(GtkWidget *widget)
{
    GtkWidget *group;
    int size;

    resources_get_int("IOSize", &size);
    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    vice_gtk3_resource_radiogroup_set(group, size);
}
