/**\file    cbm2memorysizewidget.c
 * \brief   CBM-II memory size widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES RamSize     xcbm5x0 xcbm2
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
#include "machine.h"
#include "resources.h"

#include "cbm2memorysizewidget.h"


/** \brief  List of RAM sizes for 5x0 in KiB
 */
static const vice_gtk3_radiogroup_entry_t ram_sizes_cbm5x0[] = {
    { "64KiB",     64 },
    { "128KiB",   128 },
    { "256KiB",   256 },
    { "512KiB",   512 },
    { "1024KiB", 1024 },
    { NULL,        -1 }
};

/** \brief  List of RAM sizes for 6x0/7x0 in KiB
 */
static const vice_gtk3_radiogroup_entry_t ram_sizes_cbm6x0[] = {
    { "128KiB",   128 },
    { "256KiB",   256 },
    { "512KiB",   512 },
    { "1024KiB", 1024 },
    { NULL,        -1 }
};


/** \brief  Create CBM-II memory size widget
 *
 * \return  GtkGrid
 */
GtkWidget *cbm2_memory_size_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *radio_group;
    const vice_gtk3_radiogroup_entry_t *ram_sizes;

    if (machine_class == VICE_MACHINE_CBM5x0) {
        ram_sizes = ram_sizes_cbm5x0;
    } else {
        ram_sizes = ram_sizes_cbm6x0;
    }

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "RAM size", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);
    radio_group = vice_gtk3_resource_radiogroup_new("RamSize",
                                                    ram_sizes,
                                                    GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(radio_group, 8);
    gtk_grid_attach(GTK_GRID(grid), radio_group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set used-defined callback to trigger when the RAM size changes
 *
 * \param[in,out]   widget      cbm2 memory size widget
 * \param[in]       callback    user-defined callback
 */
void cbm2_memory_size_widget_set_callback(GtkWidget *widget,
                                          void (*callback)(GtkWidget*, int))
{
    GtkWidget *group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    if (group != NULL) {
        vice_gtk3_resource_radiogroup_add_callback(group, callback);
    }
}


/** \brief  Update/sync widget via its resource
 *
 * \param[in]   widget  cbm2 memory size widget
 */
void cbm2_memory_size_widget_update(GtkWidget *widget)
{
    GtkWidget *group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    if (group != NULL) {
        vice_gtk3_resource_radiogroup_sync(group);
    }
}
