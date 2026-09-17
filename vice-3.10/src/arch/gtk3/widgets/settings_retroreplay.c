/** \file   settings_retroreplay.c
 * \brief   Settings widget to control Retro Replay resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * \todo    Check if the cartimagehelper can be used here to simplify the code
 *          for image selection and save/flush.
 */

/*
 * $VICERES RRFlashJumper   x64 x64sc xscpu64 x128
 * $VICERES RRBankJumper    x64 x64sc xscpu64/x128
 * $VICERES RRBiosWrite     x64 x64sc xscpu64/x128
 * $VICERES RRrevision      x64 x64sc xscpu64/x128
 * $VICERES RRClockPort     x64 x64sc xscpu64/x128
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

#include "c64cart.h"
#include "cartridge.h"
#include "clockportdevicewidget.h"
#include "vice_gtk3.h"

#include "settings_retroreplay.h"


/** \brief  Temporary define to ease typing and copy/paste */
#define CARTNAME    CARTRIDGE_NAME_RETRO_REPLAY


/** \brief  List of Retro Replay revisions
 */
static const vice_gtk3_combo_entry_int_t rr_revisions[] = {
    { CARTRIDGE_NAME_RETRO_REPLAY,  RR_REV_RETRO_REPLAY },
    { CARTRIDGE_NAME_NORDIC_REPLAY, RR_REV_NORDIC_REPLAY },
    { NULL,                         -1 }
};


/** \brief  Callback for the save-dialog response handler
 *
 * \param[in,out]   dialog      save-file dialog
 * \param[in,out]   filename    filename
 * \param[in]       data        extra data (unused)
 */
static void save_filename_callback(GtkDialog *dialog,
                                   gchar     *filename,
                                   gpointer   data)
{
    if (filename != NULL) {
        if (cartridge_save_image(CARTRIDGE_RETRO_REPLAY, filename) < 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    CARTNAME " Error",
                                    "Failed to save image as '%s'.",
                                    filename);
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'clicked' event of the "Save As" button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra event data (unused)
 */
static void on_save_clicked(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog;

    dialog = vice_gtk3_save_file_dialog("Save " CARTNAME
                                        " image as",
                                        NULL,
                                        TRUE,
                                        NULL,
                                        save_filename_callback,
                                        NULL);
    gtk_widget_show(dialog);
}


/** \brief  Handler for the 'clicked' event of the "Flush now" button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra event data (unused)
 */
static void on_flush_clicked(GtkWidget *widget, gpointer user_data)
{
    if (cartridge_flush_image(CARTRIDGE_RETRO_REPLAY) < 0) {
        GtkWidget *parent;

        parent = gtk_widget_get_toplevel(widget);
        if (!GTK_IS_WINDOW(widget)) {
            parent = NULL;  /* default to current emulator window */
        }
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                CARTNAME " Error",
                                "Failed to flush current image.");
    }
}

/** \brief  Create left-align label
 *
 * \param[in]   text    text for the label (uses Pango markup)
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    return label;
}

/** \brief  Create GtkSwitch for a boolean resource
 *
 * Create new GtkSwitch, left-aligned, centered vertically and non-expanding.
 *
 * \param[in]   resource    resource name
 *
 * \return  GtkSwitch
 */
static GtkWidget *create_switch(const char *resource)
{
    GtkWidget *sw = vice_gtk3_resource_switch_new(resource);

    gtk_widget_set_halign(sw, GTK_ALIGN_START);
    gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(sw, FALSE);
    gtk_widget_set_vexpand(sw, FALSE);
    return sw;
}

/** \brief  Create widget to control Retro Replay resources
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_retroreplay_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *box;
    GtkWidget *flash;
    GtkWidget *bank;
    GtkWidget *label;
    GtkWidget *revision;
    GtkWidget *clockport;
    GtkWidget *save_button;
    GtkWidget *flush_button;
    GtkWidget *bios_write;

    grid = vice_gtk3_grid_new_spaced(8, 8);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    /* RRFlashJumper */
    label = create_label("Flash jumper");
    flash = create_switch("RRFlashJumper");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), flash, 1, 0, 1, 1);

    /* RRBankJumper */
    label = create_label("Bank jumper");
    bank  = create_switch("RRBankJumper");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bank,  1, 1, 1, 1);

    /* RRrevision */
    label = create_label("Revision");
    revision = vice_gtk3_resource_combo_int_new("RRrevision",
                                                    rr_revisions);
    gtk_widget_set_margin_top(label, 16);
    gtk_widget_set_margin_top(revision, 16);
    gtk_grid_attach(GTK_GRID(grid), label,    0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), revision, 1, 2, 1, 1);

    /* RRClockPort */
    label = create_label("Clockport device");
    clockport = clockport_device_widget_create("RRClockPort");
    gtk_widget_set_margin_bottom(label, 16);
    gtk_widget_set_margin_bottom(clockport, 16);
    gtk_grid_attach(GTK_GRID(grid), label,     0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), clockport, 1, 3, 1, 1);

    /* RRBiosWrite */
    bios_write = vice_gtk3_resource_check_button_new("RRBiosWrite",
            "Write back " CARTNAME " Flash ROM image automatically");
    gtk_widget_set_valign(bios_write, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), bios_write, 0, 4, 3, 1);

    /* wrap buttons in button box */
    box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_set_spacing(GTK_BOX(box), 8);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_halign(box, GTK_ALIGN_END);

    /* Save image as .. */
    save_button = gtk_button_new_with_label("Save image as ..");
    g_signal_connect(save_button,
                     "clicked",
                     G_CALLBACK(on_save_clicked),
                     NULL);

    /* Flush image now */
    flush_button = gtk_button_new_with_label("Flush image");
    g_signal_connect(flush_button,
                     "clicked",
                     G_CALLBACK(on_flush_clicked),
                     NULL);

    gtk_box_pack_start(GTK_BOX(box), save_button,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), flush_button, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid), box, 3, 4, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

#undef CARTNAME
