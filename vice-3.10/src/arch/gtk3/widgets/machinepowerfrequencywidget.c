/** \file   machinepowerfrequencywidget.c
 * \brief   Machine power frequency selection widget
 *
 * Widget to set the "MachinePowerFrequency" resource to 50 or 60 Hz.
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
 * $VICERES MachinePowerFrequency   -x64dtv -xpet
 */

#include "vice.h"

#include <gtk/gtk.h>
#include "vice_gtk3.h"

#include "machinepowerfrequencywidget.h"


enum {
    ROW_LABEL = 0,
    ROW_RADIOS
};

static const vice_gtk3_radiogroup_entry_t frequencies[] = {
    { "50Hz", 50 },
    { "60Hz", 60 },
    { NULL,   -1 }
};


/** \brief  Create machine power frequency widget
 *
 * Create widget to set the "MachinePowerFrequency" resource. A small radio
 * group is created with "50Hz" and "60Hz" selections.
 *
 * \return  GtkGrid
 */
GtkWidget *machine_power_frequency_widget_new(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *radios;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Power frequency</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    radios = vice_gtk3_resource_radiogroup_new("MachinePowerFrequency",
                                               frequencies,
                                               GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_start(radios, 8);

    gtk_grid_attach(GTK_GRID(grid), label,  0, ROW_LABEL,  1, 1);
    gtk_grid_attach(GTK_GRID(grid), radios, 0, ROW_RADIOS, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Synchronize machine power frequency widget with its resource
 *
 * \param[in]   widget  machin power frequency widget
 */
void machine_power_frequency_widget_sync(GtkWidget *widget)
{
    GtkWidget *radios;

    radios = gtk_grid_get_child_at(GTK_GRID(widget), 0, ROW_RADIOS);
    vice_gtk3_resource_radiogroup_sync(radios);
}


/** \brief  Add callback to be triggered when the user clicks a radio button
 *
 * \param[in]   widget      machine power frequency widget
 * \param[in]   callback    function to call when the user selects a radio button
 */
void machine_power_frequency_widget_add_callback(GtkWidget *widget,
                                                 void (*callback)(GtkWidget *, int))
{
    GtkWidget *radios;

    radios = gtk_grid_get_child_at(GTK_GRID(widget), 0, ROW_RADIOS);
    vice_gtk3_resource_radiogroup_add_callback(radios, callback);
}
