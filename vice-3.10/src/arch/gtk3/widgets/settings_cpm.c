/** \file   settings_cpm.c
 * \brief   Settings widget to control CP/M resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CPMCart     x64 x64sc
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

#include "settings_cpm.h"


/** \brief  Create widget to control CP/M cart resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_cpm_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;

    /* only a single widget in this grid, but perhaps some day there will be
     * more =) */
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    enable = carthelpers_create_enable_check_button(CARTRIDGE_NAME_CPM,
                                                    CARTRIDGE_CPM);
    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
