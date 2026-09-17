/** \file   settings_gmod3.c
 * \brief   Settings widget to control GMod3 resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES GMod3FlashWrite     x64 x64sc xscpu64 x128
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

#include "settings_gmod3.h"


/** \brief  Callback for the save-dialog response handler
 *
 * \param[in,out]   dialog      save-file dialog
 * \param[in,out]   filename    filename
 * \param[in]       data        extra data (unused)
 */
static void save_filename_callback(GtkDialog *dialog,
                                  gchar *filename,
                                  gpointer data)
{
    if (filename != NULL) {
        if (cartridge_save_image(CARTRIDGE_GMOD3, filename) < 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    CARTRIDGE_NAME_GMOD3 " Error",
                                    "Failed to save cartridge image '%s'.",
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
    vice_gtk3_save_file_dialog("Save " CARTRIDGE_NAME_GMOD3 " cartridge image",
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
    if (cartridge_flush_image(CARTRIDGE_GMOD3) < 0) {
        /* get settings dialog */
        GtkWidget *parent = gtk_widget_get_toplevel(widget);
        if (!GTK_IS_WINDOW(parent)) {
            /* revert to current emulator window */
            parent = NULL;
        }
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                CARTRIDGE_NAME_GMOD3 " Error",
                                "Failed to flush cartridge image.");
    }
}

/** \brief  Create widget to handle Cartridge image resources and save/flush
 *
 * \return  GtkGrid
 */
static GtkWidget *create_cart_image_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *write_back;
    GtkWidget *save_button;
    GtkWidget *flush_button;
    GtkWidget *box;
    gboolean   can_save;
    gboolean   can_flush;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>" CARTRIDGE_NAME_GMOD3 " cartridge image</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);


    write_back = vice_gtk3_resource_check_button_new("GMod3FlashWrite",
                                                     "Write image when contents are changed");
    gtk_widget_set_valign(write_back, GTK_ALIGN_START);
    save_button = gtk_button_new_with_label("Save image as ..");
    g_signal_connect(save_button,
                     "clicked",
                     G_CALLBACK(on_save_clicked),
                     NULL);
    flush_button = gtk_button_new_with_label("Flush image");
    g_signal_connect(flush_button,
                     "clicked",
                     G_CALLBACK(on_flush_clicked),
                     NULL);

    can_save  = (gboolean)cartridge_can_save_image(CARTRIDGE_GMOD3);
    can_flush = (gboolean)cartridge_can_flush_image(CARTRIDGE_GMOD3);
    gtk_widget_set_sensitive(save_button,  can_save);
    gtk_widget_set_sensitive(flush_button, can_flush);

    box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_halign(box, GTK_ALIGN_END);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_box_set_spacing(GTK_BOX(box), 8);
    gtk_box_pack_start(GTK_BOX(box), save_button,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), flush_button, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(grid), write_back, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), box,        1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to control GMOD3 resources
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_gmod3_widget_create(GtkWidget *parent)
{
    return create_cart_image_widget();
}
