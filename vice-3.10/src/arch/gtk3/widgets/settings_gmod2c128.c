/** \file   settings_gmod2c128.c
 * \brief   Settings widget to control GMod2-C128 resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES GMod128EEPROMImage    x128
 * $VICERES GMod128EEPROMRW       x128
 * $VICERES GMod128FlashWrite     x128
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

#include "vice_gtk3.h"
#include "cartridge.h"
#include "machine.h"
#include "resources.h"
#include "ui.h"

#include "settings_gmod2c128.h"


/** \brief  Text entry used for the EEPROM filename
 */
static GtkWidget *eeprom_entry;


/** \brief  Callback for the open-file dialog
 *
 * \param[in,out]   dialog      open-file dialog
 * \param[in,out]   filename    filename or NULL on cancel
 * \param[in]       data        extra data (unused)
 */
static void save_filename_callback(GtkDialog *dialog,
                                   gchar *filename,
                                   gpointer data)
{
    if (filename != NULL) {
        if (cartridge_save_image(CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128), filename) < 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "Saving failed",
                                    "Failed to save cartridge image '%s'",
                                    filename);
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Handler for the "clicked" event of the Save Image button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   unused
 */
static void on_save_clicked(GtkWidget *widget, gpointer user_data)
{
    /* TODO: retrieve filename of cart image */

    vice_gtk3_save_file_dialog("Save cartridge image",
                               NULL, TRUE, NULL,
                               save_filename_callback,
                               NULL);
}


/** \brief  Handler for the "clicked" event of the Flush Image button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   unused
 */
static void on_flush_clicked(GtkWidget *widget, gpointer user_data)
{
    if (cartridge_flush_image(CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128)) < 0) {
        GtkWidget *parent;

        /* get settings dialog */
        parent = gtk_widget_get_toplevel(widget);
        if (!GTK_IS_WINDOW(parent)) {
            /* revert to current emulator window */
            parent = NULL;
        }
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                "Flushing failed",
                                "Failed to fush cartridge image");
    }
}


/** \brief  Callback for the EEPROM file selection dialog
 *
 * \param[in,out]   dialog      file chooser dialog
 * \param[in,out]   filename    name of the EEPROM file
 * \param[in]       data        extra data (unused)
 */
static void eeprom_filename_callback(GtkDialog *dialog,
                                     gchar     *filename,
                                     gpointer   data)
{
    if (filename != NULL) {
        if (resources_set_string("GMod128EEPROMImage", filename) < 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "Failed to load EEPROM file",
                                    "Failed to load EEPROM image file '%s'",
                                    filename);
        } else {
            gtk_entry_set_text(GTK_ENTRY(eeprom_entry), filename);
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}



/** \brief  Handler for the "clicked" event of the EEPROM browse button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   text entry
 */
static void on_eeprom_browse_clicked(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog;

    dialog = vice_gtk3_open_file_dialog(
            "Open EEMPROM image",
            NULL, NULL, NULL,
            eeprom_filename_callback,
            NULL);
    gtk_widget_show(dialog);
}


/** \brief  Create widget to handle Cartridge image resources and save/flush
 *
 * \return  GtkGrid
 */
static GtkWidget *create_cart_image_widget(void)
{
    GtkWidget *grid;
    GtkWidget *write_back;
    GtkWidget *save_button;
    GtkWidget *flush_button;

    grid = vice_gtk3_grid_new_spaced_with_label(
            -1, -1, "GMod2-C128 Cartridge image", 3);

    write_back = vice_gtk3_resource_check_button_new("GMod128FlashWrite",
                "Save image when changed");
    gtk_widget_set_margin_start(write_back, 16);
    gtk_grid_attach(GTK_GRID(grid), write_back, 0, 1, 1, 1);

    save_button = gtk_button_new_with_label("Save image as ...");
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked),
            NULL);
    gtk_grid_attach(GTK_GRID(grid), save_button, 1, 1, 1, 1);
    gtk_widget_set_sensitive(save_button,
            (gboolean)(cartridge_can_save_image(CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128))));

    flush_button = gtk_button_new_with_label("Save image");
    g_signal_connect(flush_button, "clicked", G_CALLBACK(on_flush_clicked),
            NULL);
    gtk_widget_set_sensitive(flush_button,
            (gboolean)(cartridge_can_flush_image(CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128))));
    gtk_grid_attach(GTK_GRID(grid), flush_button, 2, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to control GMod2 EEPROM
 *
 * \return  GtkGrid
 */
static GtkWidget *create_eeprom_image_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *browse;
    GtkWidget *write_enable;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "GMod2-C128 EEPROM image", 1);

    label = gtk_label_new("EEPROM image file");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 16);

    eeprom_entry = vice_gtk3_resource_entry_new("GMod128EEPROMImage");
    gtk_widget_set_hexpand(eeprom_entry, TRUE);

    browse = gtk_button_new_with_label("Browse ...");
    g_signal_connect(browse, "clicked", G_CALLBACK(on_eeprom_browse_clicked),
            NULL);

    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), eeprom_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), browse, 2, 1, 1, 1);

    write_enable = vice_gtk3_resource_check_button_new("GMod128EEPROMRW",
            "Enable writes to GMod2-C128 EEPROM image");
    gtk_widget_set_margin_start(write_enable, 16);
    gtk_grid_attach(GTK_GRID(grid), write_enable, 0, 2, 3, 1);


    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to control GMOD2-C128 resources
 *
 * \param[in]   parent  parent widget, used for dialogs
 *
 * \return  GtkGrid
 */
GtkWidget *settings_gmod2c128_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;

    grid = vice_gtk3_grid_new_spaced(8, 8);

    gtk_grid_attach(GTK_GRID(grid), create_cart_image_widget(), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), create_eeprom_image_widget(), 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
