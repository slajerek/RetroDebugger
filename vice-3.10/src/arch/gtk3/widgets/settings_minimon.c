/** \file   settings_minimon.c
 * \brief   Settings widget to control MINIMON resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Groepaz
 */

/*
 * $VICERES MinimonEnabled           xvic
 * $VICERES MinimonFilename          xvic
 * $VICERES MinimonPgmSwitch         xvic
 * $VICERES MinimonIoSwitch          xvic
 * $VICERES MinimonImageWrite        xvic
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

#include "settings_minimon.h"


/** \brief  Create MINIMON switch button
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_minimon_switch_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    label = gtk_label_new(" RO/PGM");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    button = vice_gtk3_resource_switch_new("MinimonPgmSwitch");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);
    gtk_widget_show_all(grid);

    return grid;
}

/** \brief  Create MINIMON switch button
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_minimon_ioswitch_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    label = gtk_label_new(" CART/MON");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    button = vice_gtk3_resource_switch_new("MinimonIoSwitch");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);
    gtk_widget_show_all(grid);

    return grid;
}


/** \brief  Create widget to load/save MINIMON image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_minimon_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_VIC20_MINIMON,
                                  CARTRIDGE_VIC20_NAME_MINIMON,
                                  CART_IMAGE_PRIMARY,
                                  "ROM",
                                  "MinimonFilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "MinimonImageWrite",
                                   "Write image on detach/emulator exit");
    return image;
}


/** \brief  Create widget to control MINIMON resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_minimon_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *image;
    GtkWidget *toggle;
    GtkWidget *toggle2;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 32);

    enable = carthelpers_create_enable_check_button(CARTRIDGE_VIC20_NAME_MINIMON,
                                                    CARTRIDGE_VIC20_MINIMON);
    toggle = create_minimon_switch_widget();
    toggle2 = create_minimon_ioswitch_widget();
    image  = create_minimon_image_widget();
    gtk_widget_set_hexpand(toggle, TRUE);
    gtk_widget_set_halign(toggle, GTK_ALIGN_END);

    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), toggle, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), toggle2, 2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), image,  0, 1, 3, 1);

    gtk_widget_show_all(grid);
    return grid;
}
