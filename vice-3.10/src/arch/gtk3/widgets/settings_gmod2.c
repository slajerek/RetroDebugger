/** \file   settings_gmod2widget.c
 * \brief   Settings widget to control GMod2 resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES GMod2EEPROMImage    x64 x64sc xscpu64 x128
 * $VICERES GMod2EEPROMRW       x64 x64sc xscpu64 x128
 * $VICERES GMod2FlashWrite     x64 x64sc xscpu64 x128
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

#include "settings_gmod2.h"


/** \brief  Create widget to control GMOD2 resources
 *
 * \param[in]   parent  parent widget, used for dialogs
 *
 * \return  GtkGrid
 */
GtkWidget *settings_gmod2_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *primary;
    GtkWidget *secondary;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 32);

    /* primary image: 512KB flash */
    primary = cart_image_widget_new(CARTRIDGE_GMOD2,        /* cart id */
                                    CARTRIDGE_NAME_GMOD2,   /* cart name */
                                    CART_IMAGE_PRIMARY,     /* image number */
                                    "cartridge",            /* image tag */
                                    NULL,                   /* resource name */
                                    TRUE,                   /* flush button */
                                    TRUE                    /* save button */
                                    );
    cart_image_widget_append_check(primary,
                                   "GMod2FlashWrite",
                                   "Save image when changed");

    /* secondary image: 2KB eeprom */
    secondary = cart_image_widget_new(CARTRIDGE_GMOD2,
                                      CARTRIDGE_NAME_GMOD2,
                                      CART_IMAGE_SECONDARY,
                                      "EEPROM",
                                      "GMod2EEPROMImage",
                                      TRUE,
                                      TRUE);
    cart_image_widget_append_check(secondary,
                                   "GMod2EEPROMRW",
                                   "Enable writes to " CARTRIDGE_NAME_GMOD2
                                   " EEPROM image");

    gtk_grid_attach(GTK_GRID(grid), primary,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), secondary, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
