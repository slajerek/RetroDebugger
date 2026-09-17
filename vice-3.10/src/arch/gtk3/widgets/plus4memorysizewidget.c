/** \file   plus4memorysizewidget.c
 * \brief   Plus4 memory size widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES RamSize xplus4
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

#include "plus4memorysizewidget.h"


/** \brief  List of RAM sizes in KiB
 */
static const vice_gtk3_radiogroup_entry_t ram_sizes[] = {
    { "16KiB",  16 },
    { "32KiB",  32 },
    { "64KiB",  64 },
    { NULL,     -1 }
};


/** \brief  Reference to the memory size widget */
static GtkWidget *memory_size_widget = NULL;


/** \brief  Create Plus/4 memory size widget
 *
 * \return  GtkGrid
 */
GtkWidget *plus4_memory_size_widget_create(void)
{
    GtkWidget *grid;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Memory size", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);
    memory_size_widget = vice_gtk3_resource_radiogroup_new("RamSize",
                                                           ram_sizes,
                                                           GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(memory_size_widget, 8);
    gtk_grid_attach(GTK_GRID(grid), memory_size_widget, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set extra callback to trigger when the resource changes
 *
 * \param[in]   callback    function to trigger
 */
void plus4_memory_size_widget_add_callback(void (*callback)(GtkWidget *, int))
{
    vice_gtk3_resource_radiogroup_add_callback(memory_size_widget, callback);
}


/** \brief  Synchronize the widget with its resource
 *
 * \return  TRUE if the widget was synchronized
 */
gboolean plus4_memory_size_widget_sync(void)
{
    return vice_gtk3_resource_radiogroup_sync(memory_size_widget);
}
