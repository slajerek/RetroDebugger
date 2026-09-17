/** \file   settings_easyflash.c
 * \brief   Settings widget to control Easy Flash resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES EasyFlashJumper         x64 x64sc xscpu64 x128
 * $VICERES EasyFlashWriteCRT       x64 x64sc xscpu64 x128
 * $VICERES EasyFlashOptimizeCRT    x64 x64sc xscpu64 x128
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

#include "settings_easyflash.h"


/** \brief  Create widget for EasyFlash primary image
 *
 * \return  GtkGrid
 */
static GtkWidget *create_primary_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_EASYFLASH,
                                  CARTRIDGE_NAME_EASYFLASH,
                                  CART_IMAGE_PRIMARY,
                                  "cartridge",
                                  NULL,
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "EasyFlashWriteCRT",
                                   "Save image when changed");
    cart_image_widget_append_check(image,
                                   "EasyFlashOptimizeCRT",
                                   "Optimize image when saving");
    return image;
}


/** \brief  Create EasyFlash widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_easyflash_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *primary;
    GtkWidget *label;
    GtkWidget *jumper;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    primary = create_primary_image_widget();
    label   = gtk_label_new(CARTRIDGE_NAME_EASYFLASH " jumper");
    jumper  = vice_gtk3_resource_switch_new("EasyFlashJumper");
    gtk_widget_set_hexpand(jumper, TRUE);
    gtk_widget_set_halign(jumper, GTK_ALIGN_START);
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), primary, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), label,   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), jumper,  1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
