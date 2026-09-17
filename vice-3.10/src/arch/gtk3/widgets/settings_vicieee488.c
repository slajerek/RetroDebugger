/** \file   settings_vicieee488.c
 * \brief   Setting widget for the VIC-1112 IEEE-488 Interface
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES IEEE488     xvic
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

#include "cartridge.h"
#include "vice_gtk3.h"

#include "settings_vicieee488.h"


/** \brief  Create widget to control VIC-20 IEEE-488 resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_vicieee488_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *check;

    grid  = gtk_grid_new();
    check = vice_gtk3_resource_check_button_new("IEEE488",
                                                "Enable " CARTRIDGE_VIC20_NAME_IEEE488
                                                " emulation");
    gtk_grid_attach(GTK_GRID(grid), check, 0, 0, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
