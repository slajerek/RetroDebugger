/** \file   settings_megacart.c
 * \brief   Settings widget controlling VIC-20 Mega Cart resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES MegaCartNvRAMfilename   xvic
 * $VICERES MegaCartNvRAMWriteBack  xvic
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

#include "settings_megacart.h"

/** \brief  Create widget to load/save NVRAM image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_secondary_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_VIC20_MEGACART,
                                  CARTRIDGE_VIC20_NAME_MEGACART,
                                  CART_IMAGE_SECONDARY,
                                  "NvRAM", /* dialog header tag */
                                  "MegaCartNvRAMfilename", /* image file resource */
                                  TRUE, /* flush button */
                                  TRUE); /* save as button */
    cart_image_widget_append_check(image,
                                   "MegaCartNvRAMWriteBack",
                                   "Write image on detach/emulator exit");
    return image;
}

/** \brief  Create Mega Cart settings widget
 *
 * Allows selecting an nvram image and enabling/disabling write-back.
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_megacart_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *chooser;

    grid = gtk_grid_new();
    chooser = create_secondary_image_widget();
    gtk_grid_attach(GTK_GRID(grid), chooser, 0, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
