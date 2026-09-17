/** \file   settings_digimax.c
 * \brief   Setting widget controlling DigiMAX resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES DIGIMAX         x64 x64sc xscpu64 x128 xvic
 * $VICERES DIGIMAXbase     x64 x64sc xscpu64 x128 xvic
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
#include "machine.h"
#include "vice_gtk3.h"

#include "settings_digimax.h"


/** \brief  Cart IO area base start C64/C128 */
#define IOBASE_C64_START    0xde00
/** \brief  Cart IO area base end (exclusive) on C64/C128 */
#define IOBASE_C64_END      0xe000

/** \brief  Cart IO area base start on VIC-20 */
#define IOBASE_VIC20_START  0x9800
/** \brief  Cart IO area base end (exclusive) on VIC-20 */
#define IOBASE_VIC20_END    0x9900

/** \brief  Size of an IO area segment */
#define IOBASE_STEP         0x20


/** \brief  Create DIGIMAX widget
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_digimax_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *label;
    GtkWidget *address;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    enable = vice_gtk3_resource_check_button_new("DIGIMAX",
                                                 "Enable " CARTRIDGE_NAME_DIGIMAX " emulation");
    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 2, 1);

    label = gtk_label_new("DigiMAX I/O base");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    /* all addresses are in a nice, simple range */
    if (machine_class == VICE_MACHINE_VIC20) {
        address = vice_gtk3_resource_combo_hex_new_range("DIGIMAXbase",
                                                         IOBASE_VIC20_START,
                                                         IOBASE_VIC20_END,
                                                         IOBASE_STEP);
    } else {
        address = vice_gtk3_resource_combo_hex_new_range("DIGIMAXbase",
                                                         IOBASE_C64_START,
                                                         IOBASE_C64_END,
                                                         IOBASE_STEP);
    }

    gtk_grid_attach(GTK_GRID(grid), label,   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), address, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
