/** \file   settings_rexramfloppy.c
 * \brief   Settings widget to control REX Ram-Floppy resources
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 */

/*
 * $VICERES RRFfilename    x64 x64sc xscpu64 x128
 * $VICERES RRFImageWrite  x64 x64sc xscpu64 x128
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

#include "settings_rexramfloppy.h"


/** \brief  Create widget to load/save REX Ram-Floppy image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_secondary_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_REX_RAMFLOPPY,
                                  CARTRIDGE_NAME_REX_RAMFLOPPY,
                                  CART_IMAGE_SECONDARY,
                                  "RAM",
                                  "RRFfilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "RRFImageWrite",
                                   "Write image on detach/emulator exit");
    return image;
}


/** \brief  Create widget to control REX Ram-Floppy resources
 *
 * \param[in]   parent  parent widget, used for dialogs
 *
 * \return  GtkGrid
 */
GtkWidget *settings_rexramfloppy_widget_create(GtkWidget *parent)
{
    GtkWidget *primary;

    primary = create_secondary_image_widget();
    gtk_widget_show_all(primary);
    return primary;
}
