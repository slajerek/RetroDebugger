/** \file   petramsizewidget.c
 * \brief   Widget to set the PET RAM size
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES RamSize     xpet
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

#include "basewidgets.h"
#include "widgethelpers.h"
#include "debug_gtk3.h"
#include "resources.h"

#include "petramsizewidget.h"


/** \brief  Data for the radio buttons group
 */
static const vice_gtk3_radiogroup_entry_t ram_sizes[] = {
    { "4KiB",     4 },
    { "8KiB",     8 },
    { "16KiB",   16 },
    { "32KiB",   32 },
    { "96KiB",   96 },
    { "128KiB", 128 },
    { NULL,      -1 }
};


/** \brief  User-defined callback function
 *
 * Set with pet_ram_size_widget_set_callback()
 */
static void (*glue_func)(int);


/** \brief  Callback for the radio group widget
 *
 * \param[in]   widget  radio group widget
 * \param[in]   size    RAM size in KiB
 */
static void ram_size_callback(GtkWidget *widget, int size)
{
    if (glue_func != NULL) {
        glue_func(size);
    }
}


/** \brief  Create PET RAM size widget
 *
 * Creates a widget to control the PET's RAM size
 *
 * \return  GtkGrid
 */
GtkWidget *pet_ram_size_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *group;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Memory size", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);
    group = vice_gtk3_resource_radiogroup_new("RamSize",
                                              ram_sizes,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(group, 8);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set callback on RAM size change
 *
 * \param[in]   widget  PET RAM size widget
 * \param[in]   func    callback function
 */
void pet_ram_size_widget_set_callback(GtkWidget *widget,
                                      void (*func)(int))
{
    GtkWidget *group;

    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    if (group != NULL) {
        vice_gtk3_resource_radiogroup_add_callback(group,
                                                   ram_size_callback);
        glue_func = func;
    }
}


/** \brief  Syncronize widget with its resource
 *
 * \param[in,out]   widget  PET RAM size widget
 */
void pet_ram_size_widget_sync(GtkWidget *widget)
{
    GtkWidget *group;

    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    vice_gtk3_resource_radiogroup_sync(group);
}
