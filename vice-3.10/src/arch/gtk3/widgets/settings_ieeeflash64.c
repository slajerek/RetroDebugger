/** \file   settings_ieeeflash64.c
 * \brief   Settings widget to control IEEE Flash! 64 resources
 *
 * \author  Christopher Bongaarts <cab@bongalow.net>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES IEEEFlash64         x64 x64sc
 * $VICERES IEEEFlash64Image    x64 x64sc
 * $VICERES IEEEFlash64Dev8     x64 x64sc
 * $VICERES IEEEFlash64Dev910   x64 x64sc
 * $VICERES IEEEFlash64Dev4     x64 x64sc
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
#include "log.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_ieeeflash64.h"

/** \brief  Temporary define to make editing easier */
#define CARTNAME    CARTRIDGE_NAME_IEEEFLASH64


/** \brief  Handler for the "toggled" event of the 'enable' check button
 *
 * Toggles the 'enabled' state of the IEEE Flash! 64 adapter/cart, but
 * only if an EEPROM image has been specified, otherwise when trying to
 * set the check button to 'true', an error message is displayed and the
 * check button is reverted to 'off'.
 *
 * \param[in,out]   widget  check button
 * \param[in]       data    unused
 */
static void on_enable_toggled(GtkWidget *widget, gpointer data)
{
    GtkWidget *parent;
    gboolean   state;

    parent = gtk_widget_get_toplevel(widget);
    if (!GTK_IS_WINDOW(widget)) {
        parent = NULL;
    }
    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (state) {
        const char *image = NULL;

        resources_get_string("IEEEFlash64Image", &image);
        if (image == NULL || *image == '\0') {
            /* no image */
            vice_gtk3_message_error(GTK_WINDOW(parent),
                                    CARTNAME " Error",
                                    "Cannot enable " CARTNAME ", no image specified.");
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
            state = 0;
        }
    }

    if (state) {
        if (cartridge_enable(CARTRIDGE_IEEEFLASH64) < 0) {
            log_error(LOG_DEFAULT, "failed to enable " CARTNAME ".");
            vice_gtk3_message_error(GTK_WINDOW(parent),
                                    CARTNAME " Error",
                                    "Failed to enable " CARTNAME ".");
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
        }
    } else {
        if (cartridge_disable(CARTRIDGE_IEEEFLASH64) < 0) {
            log_error(LOG_DEFAULT, "failed to disable " CARTNAME ".");
            vice_gtk3_message_error(GTK_WINDOW(parent),
                                    CARTNAME" Error",
                                    "Failed to enable " CARTNAME ".");
        }
    }
}


/** \brief  Create widget to control IEEE Flash! 64 resources
 *
 * \param[in]   parent  parent widget, used for dialogs
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ieeeflash64_widget_create(GtkWidget *parent)
{
    GtkWidget  *grid;
    GtkWidget  *enable;
    GtkWidget  *chlabel;
    GtkWidget  *chooser;
    GtkWidget  *dev8_route;
    GtkWidget  *dev910_route;
    GtkWidget  *dev4_route;
    const char *patterns[] = { "*.bin", "*.rom", NULL };
    const char *image = NULL;
    int         cart_enabled;

    resources_get_string("IEEEFlash64Image", &image);
    cart_enabled = cartridge_type_enabled(CARTRIDGE_IEEEFLASH64);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    /* we can't use a `resource_check_button` here, since toggling the resource
     * depends on whether an image file is specified
     */
    enable = gtk_check_button_new_with_label("Enable " CARTNAME " emulation");
    gtk_widget_set_margin_bottom(enable, 16);
    /* only set state to true if both the state is true and an image is given */
    if (cart_enabled && (image != NULL && *image != '\0')) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable), TRUE);
    }
    g_signal_connect(G_OBJECT(enable),
                     "toggled",
                     G_CALLBACK(on_enable_toggled),
                     NULL);

    chlabel = gtk_label_new(CARTNAME " ROM Image");
    gtk_widget_set_halign(chlabel, GTK_ALIGN_START);
    chooser = vice_gtk3_resource_filechooser_new("IEEEFlash64Image",
                                                 GTK_FILE_CHOOSER_ACTION_OPEN);
    vice_gtk3_resource_filechooser_set_filter(chooser,
                                              "ROM files",
                                              patterns,
                                              TRUE);

    dev8_route   = vice_gtk3_resource_check_button_new("IEEEFlash64Dev8",
                                                       "Route device 8 to IEEE bus");
    dev910_route = vice_gtk3_resource_check_button_new("IEEEFlash64Dev910",
                                                       "Route devices 9/10 to IEEE bus");
    dev4_route   = vice_gtk3_resource_check_button_new("IEEEFlash64Dev4",
                                                       "Route device 4 to IEEE bus");
    gtk_widget_set_margin_top(dev8_route, 16);

    gtk_grid_attach(GTK_GRID(grid), enable,       0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), chlabel,      0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chooser,      1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), dev8_route,   0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), dev910_route, 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), dev4_route,   0, 4, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}

#undef CARTNAME
