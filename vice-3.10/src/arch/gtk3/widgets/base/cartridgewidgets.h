/** \file   cartridgewidgets.h
 * \brief   Widgets to control cartridge resources - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#ifndef VICE_CARTRIDGEWIDGETS_H
#define VICE_CARTRIDGEWIDGETS_H

#include <gtk/gtk.h>

/** \brief  Cartridge image number type
 */
typedef enum {
    _CART_IMAGE_FORCE_SIGNED = -1,  /**< enum constants are signed, but an
                                         enum type can be unsigned if the
                                         range of values fits in an unsigned
                                         type */
    CART_IMAGE_PRIMARY = 1,         /**< primary cartridge image */
    CART_IMAGE_SECONDARY,           /**< secondary cartridge image */
    CART_IMAGE_TERTIARY,            /**< unused for now */
    CART_IMAGE_QUATERNARY,          /**< unused for now */

    CART_IMAGE_COUNT = CART_IMAGE_QUATERNARY,   /**< number of possible images,
                                                     used for range checks */
} cart_img_t;


GtkWidget *cart_image_widget_new(int         cart_id,
                                 const char *cart_name,
                                 cart_img_t  image_num,
                                 const char *image_tag,
                                 const char *resource,
                                 gboolean    flush_button,
                                 gboolean    save_button);

GtkWidget *cart_image_widget_append_check(GtkWidget  *widget,
                                          const char *resource,
                                          const char *text);

void       cart_image_widget_update_sensitivity(GtkWidget *widget);

void       cart_image_widgets_shutdown(void);

#endif

