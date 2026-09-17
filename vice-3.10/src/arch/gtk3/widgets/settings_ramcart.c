/** \file   settings_ramcart.c
 * \brief   Settings widget to control RamCart resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES RAMCART             x64 x64sc xscpu64 x128
 * $VICERES RAMCARTsize         x64 x64sc xscpu64 x128
 * $VICERES RAMCARTfilename     x64 x64sc xscpu64 x128
 * $VICERES RAMCARTImageWrite   x64 x64sc xscpu64 x128
 * $VICERES RAMCART_RO          x64 x64sc xscpu64 x128
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

#include "settings_ramcart.h"


/** \brief  List of supported RAM sizes in KiB */
static int ram_sizes[] = { 64, 128, -1 };


/** \brief  Create RamCart enable check button
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_ramcart_enable_widget(void)
{
    return vice_gtk3_resource_check_button_new("RAMCART",
                                               "Enable " CARTRIDGE_NAME_RAMCART
                                               " emulation");
}

/** \brief  Create radio button group to determine RamCart size
 *
 * \return  GtkGrid
 */
static GtkWidget *create_ramcart_size_widget(void)
{
    return ram_size_radiogroup_new("RAMCARTSize",
                                   CARTRIDGE_NAME_RAMCART " size",
                                   ram_sizes);
}

/** \brief  Create widget for RamCart image file
 *
 * Widge to load/save image with check buttons for "write-on-detach" and
 * "read-only".
 *
 * \return  GtkGrid
 */
static GtkWidget *create_ramcart_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_RAMCART,
                                  CARTRIDGE_NAME_RAMCART,
                                  CART_IMAGE_PRIMARY,
                                  "RAM",
                                  "RAMCARTfilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "RAMCARTImageWrite",
                                   "Write image on detach/emulator exit");
    cart_image_widget_append_check(image,
                                   "RAMCART_RO",
                                   "Image contents are read-only");
    return image;
}


/** \brief  Create widget to control RamCart resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ramcart_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *image;
    GtkWidget *size;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    enable = create_ramcart_enable_widget();
    image  = create_ramcart_image_widget();
    size   = create_ramcart_size_widget();

    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), image,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), size,   0, 2, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
