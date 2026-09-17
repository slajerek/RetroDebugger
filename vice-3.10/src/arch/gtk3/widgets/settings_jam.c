/** \file   settings_jam.c
 * \brief   Settings widget to control JAM behaviour - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES JAMAction   all
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
 */

#include "vice.h"
#include <gtk/gtk.h>

#include "machine.h"
#include "vice_gtk3.h"

#include "settings_jam.h"


/** \brief  List of possible actions on a CPU JAM
 */
static const vice_gtk3_radiogroup_entry_t actions[] = {
    { "Show dialog",            MACHINE_JAM_ACTION_DIALOG },
    { "Continue emulation",     MACHINE_JAM_ACTION_CONTINUE },
    { "Start monitor",          MACHINE_JAM_ACTION_MONITOR },
    { "Reset machine CPU",      MACHINE_JAM_ACTION_RESET_CPU },
    { "Power cycle machine",    MACHINE_JAM_ACTION_POWER_CYCLE },
    { "Quit emulator",          MACHINE_JAM_ACTION_QUIT },
    { NULL,                     -1 }
};


/** \brief  Create widget to control the "JAMAction" resource
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_jam_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Action on CPU JAM</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    group = vice_gtk3_resource_radiogroup_new("JAMAction",
                                              actions,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
