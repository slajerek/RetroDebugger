/** \file   settings_reu.c
 * \brief   Settings widget to control REU resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES REU             x64 x64sc xscpu64 x128
 * $VICERES REUsize         x64 x64sc xscpu64 x128
 * $VICERES REUfilename     x64 x64sc xscpu64 x128
 * $VICERES REUImageWrite   x64 x64sc xscpu64 x128
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

#include "settings_reu.h"


/** \brief  List of supported RAM sizes in KiB */
static int ram_sizes[] = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384, -1 };


/** \brief  Handler for the 'toggled' event of the "Enable" check button
 *
 * Update sensitivity of the cart image widget's buttons when enabling/disabling
 * REU emulation.
 *
 * \param[in]   self    check button (ignored)
 * \param[in]   image   cartridge image widget
 */
static void on_enable_activate(GtkWidget *self, gpointer image)
{
    cart_image_widget_update_sensitivity(GTK_WIDGET(image));
}

/** \brief  Create radio button group to determine REU RAM size
 *
 * \return  GtkGrid
 */
static GtkWidget *create_reu_size_widget(void)
{
    return ram_size_radiogroup_new("REUsize",
                                   CARTRIDGE_NAME_REU " Size",
                                   ram_sizes);
}

/** \brief  Create widget to load/save REU image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_reu_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_REU,
                                  CARTRIDGE_NAME_REU,
                                  CART_IMAGE_PRIMARY,
                                  "RAM",
                                  "REUfilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "REUImageWrite",
                                   "Write image on detach/emulator exit");
    return image;
}


/** \brief  Create widget to control RAM Expansion Module resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_reu_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *size;
    GtkWidget *image;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    enable = carthelpers_create_enable_check_button(CARTRIDGE_NAME_REU,
                                                    CARTRIDGE_REU);
    image  = create_reu_image_widget();
    size   = create_reu_size_widget();

    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), image,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), size,   0, 2, 1, 1);

    /* set up signal handler to update sensitivity of save/flush buttons when
     * toggling REU emulation */
    g_signal_connect(G_OBJECT(enable),
                     "toggled",
                     G_CALLBACK(on_enable_activate),
                     (gpointer)image);

    gtk_widget_show_all(grid);
    return grid;
}
