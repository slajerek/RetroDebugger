/** \file   settings_dqbb.c
 * \brief   Widget to control Double Quick Brown Box resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES DQBB            x64 x64sc xscpu64 x128
 * $VICERES DQBBfilename    x64 x64sc xscpu64 x128
 * $VICERES DQBBImageWrite  x64 x64sc xscpu64 x128
 * $VICERES DQBBSize        x64 x64sc xscpu64 x128
 * $VICERES DQBBMode        x64 x64sc xscpu64 x128
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
#include "c64cart.h"
#include "vice_gtk3.h"

#include "settings_dqbb.h"

/** \brief  List of supported RAM sizes */
static int ram_sizes[] = { 16, 32, 64, 128, 256, -1 };

/** \brief  List of supported modes */
static int dqbb_modes[] = { DQBB_MODE_C64, DQBB_MODE_C128, -1 };

/** \brief  Create radio button group to determine DQBB RAM size
 *
 * \return  GtkGrid
 */
static GtkWidget *create_dqbb_size_widget(void)
{
    return ram_size_radiogroup_new("DQBBsize",
                                   CARTRIDGE_NAME_DQBB " Size",
                                   ram_sizes);
}

/** \brief  Create radio button group to determine DQBB mode
 *
 * \return  GtkGrid
 */
static GtkWidget *create_dqbb_mode_widget(void)
{
    return ram_size_radiogroup_new("DQBBmode",
                                   CARTRIDGE_NAME_DQBB " Mode",
                                   dqbb_modes);
}

/** \brief  Create widget to load/save Double Quick Brown Box image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_dqbb_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_DQBB,
                                  CARTRIDGE_NAME_DQBB,
                                  CART_IMAGE_PRIMARY,
                                  "RAM",
                                  "DQBBfilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "DQBBImageWrite",
                                   "Write image on detach/emulator exit");
    return image;
}


/** \brief  Create widget to control Double Quick Brown Box resources
 *
 * \param[in]   parent  parent widget, used for dialogs
 *
 * \return  GtkGrid
 */
GtkWidget *settings_dqbb_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *dqbb_enable_widget; /* dqbb_enable shadows */
    GtkWidget *dqbb_image;
    GtkWidget *dqbb_size;
    GtkWidget *dqbb_mode;

    grid = vice_gtk3_grid_new_spaced(8, 8);

    dqbb_enable_widget = carthelpers_create_enable_check_button(CARTRIDGE_NAME_DQBB,
                                                                CARTRIDGE_DQBB);
    gtk_grid_attach(GTK_GRID(grid), dqbb_enable_widget, 0, 0, 1, 1);

    dqbb_image = create_dqbb_image_widget();
    dqbb_size = create_dqbb_size_widget();
    dqbb_mode = create_dqbb_mode_widget();

    gtk_widget_set_margin_top(dqbb_image, 8);
    gtk_grid_attach(GTK_GRID(grid), dqbb_image, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), dqbb_size,  0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), dqbb_mode,  1, 2, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
