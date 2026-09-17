/** \file   settings_vicioram.c
 * \brief   Settings widget for VIC-20 I/O RAM
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES IO2RAM  xvic
 * $VICERES IO3RAM  xvic
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

#include "settings_vicioram.h"


/** \brief  Create widget to control VIC-20 IEEE-488 resources
 *
 * \param[in]   parent  parent widget, used for dialogs
 *
 * \return  GtkGrid
 */
GtkWidget *settings_vicioram_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *io2ram;
    GtkWidget *io3ram;

    grid = gtk_grid_new();

    io2ram = vice_gtk3_resource_check_button_new("IO2RAM",
                                                 "Enable IO-2 RAM Cartridge"
                                                 " ($9800-$9BFF)"),
    io3ram = vice_gtk3_resource_check_button_new("IO3RAM",
                                                 "Enable IO-3 RAM Cartridge"
                                                 " ($9C00-$9FFF)"),
    gtk_grid_attach(GTK_GRID(grid), io2ram, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), io3ram, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
