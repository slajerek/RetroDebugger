/** \file   settings_ethernetcart.c
 * \brief   Settings widget to control ethernet cartridge settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES ETHERNETCART_ACTIVE     x64 x64s xscpu64 x128 xvic
 * $VICERES ETHERNETCARTMode        x64 x64s xscpu64 x128
 * $VICERES ETHERNETCARTBase        x64 x64s xscpu64 x128 xvic
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

#include "archdep.h"
#include "c64cart.h"
#include "log.h"
#include "machine.h"
#include "vice_gtk3.h"

#include "settings_ethernetcart.h"


#ifdef HAVE_RAWNET

/** \brief  List of Ethernet Cartridge emulation modes
 */
static const vice_gtk3_radiogroup_entry_t modes[] = {
    { "ETFE",   ETHERNETCART_MODE_TFE },
    { "RRNet",  ETHERNETCART_MODE_RRNET },
    { NULL,     -1 }
};

/** \brief  List of ethernet cart I/O base addresses for VIC-20 */
static int eth_base_list_vic20[] = {
    0x9800, 0x9810, 0x9820, 0x9830, 0x9840, 0x9850, 0x9860, 0x9870,
    0x9880, 0x9890, 0x98a0, 0x98b0, 0x98c0, 0x98d0, 0x98e0, 0x98f0,
    0x9c00, 0x9c10, 0x9c20, 0x9c30, 0x9c40, 0x9c50, 0x9c60, 0x9c70,
    0x9c80, 0x9c90, 0x9ca0, 0x9cb0, 0x9cc0, 0x9cd0, 0x9ce0, 0x9cf0,
    -1
};


/** \brief  Create widget to select the Ethernet Cartridge emulation mode
 *
 * \return  GtkGrid
 */
static GtkWidget *create_cartridge_mode_widget(void)
{
    GtkWidget *group;

    group = vice_gtk3_resource_radiogroup_new("ETHERNETCARTMode",
                                              modes,
                                              GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_set_column_spacing(GTK_GRID(group), 16);
    return group;
}

/** \brief  Create widget to create Ethernet Cartridge I/O base
 *
 * \return  GtkGrid
 */
static GtkWidget *create_cartridge_base_widget(void)
{
    GtkWidget *combo = NULL;

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fallthrough */
        case VICE_MACHINE_C64SC:    /* fallthrough */
        case VICE_MACHINE_C128:     /* fallthrough */
        case VICE_MACHINE_SCPU64:
            combo = vice_gtk3_resource_combo_hex_new_range("ETHERNETCARTBase",
                                                           0xde00, 0xe000, 0x10);
            break;

        case VICE_MACHINE_VIC20:
            combo = vice_gtk3_resource_combo_hex_new_list("ETHERNETCARTBase",
                                                          eth_base_list_vic20);
            break;
        default:
            log_error(LOG_DEFAULT, "%s(): should never get here!", __func__);
            break;
    }
    return combo;
}


/** \brief  Create widget to control generic Ethernet cartridget settings
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ethernetcart_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable_widget;
    GtkWidget *mode_widget;
    GtkWidget *mode_label;
    GtkWidget *base_widget;
    GtkWidget *base_label;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    enable_widget = vice_gtk3_resource_check_button_new("ETHERNETCART_ACTIVE",
                                                        "Enable ethernet cartridge");
    gtk_widget_set_margin_bottom(enable_widget, 24);
    gtk_grid_attach(GTK_GRID(grid), enable_widget, 0, row, 2, 1);
    row++;

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fallthrough */
        case VICE_MACHINE_C64SC:    /* fallthrough */
        case VICE_MACHINE_C128:     /* fallthrough */
        case VICE_MACHINE_SCPU64:
            mode_label  = gtk_label_new("Cartridge mode");
            mode_widget = create_cartridge_mode_widget();
            gtk_widget_set_halign(mode_label, GTK_ALIGN_START);
            gtk_grid_attach(GTK_GRID(grid), mode_label,  0, row, 1, 1);
            gtk_grid_attach(GTK_GRID(grid), mode_widget, 1, row, 1, 1);
            row++;
            break;

        default:
            break;
    }

    base_label  = gtk_label_new("Cartridge I/O base");
    base_widget = create_cartridge_base_widget();
    gtk_widget_set_halign(base_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), base_label,  0, row, 1,1);
    gtk_grid_attach(GTK_GRID(grid), base_widget, 1, row, 1, 1);

    gtk_widget_set_sensitive(grid, (gboolean)archdep_ethernet_available());
    gtk_widget_show_all(grid);
    return grid;
}
#endif
