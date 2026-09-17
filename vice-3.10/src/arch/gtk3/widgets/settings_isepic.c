/** \file   settings_isepic.c
 * \brief   Settings widget to control ISEPIC resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES IsepicCartridgeEnabled  x64 x64sc xscpu64 x128
 * $VICERES Isepicfilename          x64 x64sc xscpu64 x128
 * $VICERES IsepicSwitch            x64 x64sc xscpu64 x128
 * $VICERES IsepicImageWrite        x64 x64sc xscpu64 x128
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

#include "settings_isepic.h"


/** \brief  Create ISEPIC switch button
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_isepic_switch_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    label = gtk_label_new(CARTRIDGE_NAME_ISEPIC " enable switch");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    button = vice_gtk3_resource_switch_new("IsepicSwitch");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);
    gtk_widget_show_all(grid);

    return grid;
}


/** \brief  Create widget to load/save ISEPIC image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_isepic_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_ISEPIC,
                                  CARTRIDGE_NAME_ISEPIC,
                                  CART_IMAGE_PRIMARY,
                                  "RAM",
                                  "Isepicfilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "IsepicImageWrite",
                                   "Write image on detach/emulator exit");
    return image;
}


/** \brief  Create widget to control ISEPIC resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_isepic_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *image;
    GtkWidget *toggle;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 32);

    enable = carthelpers_create_enable_check_button(CARTRIDGE_NAME_ISEPIC,
                                                    CARTRIDGE_ISEPIC);
    toggle = create_isepic_switch_widget();
    image  = create_isepic_image_widget();
    gtk_widget_set_hexpand(toggle, TRUE);
    gtk_widget_set_halign(toggle, GTK_ALIGN_END);

    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), toggle, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), image,  0, 1, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}
