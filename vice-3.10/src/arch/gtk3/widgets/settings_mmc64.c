/** \file   settings_mmc64.c
 * \brief   Settings widget to control MMC64 resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES MMC64               x64 x64sc xscpu64 x128
 * $VICERES MMC64BIOSfilename   x64 x64sc xscpu64 x128
 * $VICERES MMC64_bios_write    x64 x64sc xscpu64 x128
 * $VICERES MMC64_flashjumper   x64 x64sc xscpu64 x128
 * $VICERES MMC64_revision      x64 x64sc xscpu64 x128
 * $VICERES MMC64imagefilename  x64 x64sc xscpu64 x128
 * $VICERES MMC64_RO            x64 x64sc xscpu64 x128
 * $VICERES MMC64_sd_type       x64 x64sc xscpu64 x128
 * $VICERES MMC64ClockPort      x64 x64sc xscpu64 x128
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
#include "log.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_mmc64.h"


/** \brief  Temporary define to make editing easier */
#define CARTNAME    CARTRIDGE_NAME_MMC64


/** \brief  List of revisions
 */
static const vice_gtk3_radiogroup_entry_t revisions[] = {
    { "Rev. A", MMC64_REV_A },
    { "Rev. B", MMC64_REV_B },
    { NULL,     -1 }
};

/** \brief  List of memory card types
 */
static const vice_gtk3_radiogroup_entry_t card_types[] = {
    { "Auto", MMC64_TYPE_AUTO },
    { "MMC",  MMC64_TYPE_MMC },
    { "SD",   MMC64_TYPE_SD },
    { "SDHC", MMC64_TYPE_SDHC },
    { NULL,   -1 }
};


/** \brief  BIOS filename entry */
static GtkWidget *bios_filename = NULL;

/** \brief  BIOS "Flush image" button */
static GtkWidget *flush_button = NULL;

/** \brief  BIOS "Save As" button" */
static GtkWidget *save_button = NULL;

/** \brief  SD card widget filename entry */
static GtkWidget *card_filename = NULL;


/** \brief  Update sensitivity of BIOS flush and save buttons
 *
 * Set sensitivity of the "Flush" and "Save As" buttons of the BIOS file
 * widget, based on the presence of a BIOS image.
 * (Assumes the resource "MMC64BIOSfilename" is only set if a valid BIOS file
 * is loaded)
 */
static void update_buttons_sensitivity(void)
{
    const char *bios = NULL;
    gboolean    sensitive;

    resources_get_string("MMC64BIOSfilename", &bios);
    sensitive = (bios != NULL && *bios != '\0');
    gtk_widget_set_sensitive(flush_button, sensitive);
    gtk_widget_set_sensitive(save_button,  sensitive);
}

/** \brief  Custom callback for the BIOS file chooser widget
 *
 * \param[in]   entry       file chooser's GtkEntry widget (ignored)
 * \param[in]   filename    filename returned by the file chooser (ignored)
 */
static void bios_callback(GtkEntry *entry, gchar *filename)
{
    update_buttons_sensitivity();
}

/** \brief  Handler for the 'toggled' event of the "MMC64 Enabled" widget
 *
 * \param[in,out]   check       check button
 * \param[in]       user_data   extra event data (unused)
 */
static void on_enable_toggled(GtkWidget *check, gpointer user_data)
{
    GtkWidget  *parent;
    gboolean    enabled;
    const char *bios;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    bios    = gtk_entry_get_text(GTK_ENTRY(bios_filename));
    parent  = gtk_widget_get_toplevel(check);
    if (!GTK_IS_WINDOW(parent)) {
        parent = NULL;
    }

    if (enabled && (bios == NULL || *bios == '\0')) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                CARTNAME " Error",
                                "Cannot enable " CARTNAME " due to missing BIOS file.");
        update_buttons_sensitivity();
        return;
    }

    if (enabled && bios != NULL && *bios != '\0') {
        if (cartridge_enable(CARTRIDGE_MMC64) < 0) {
            /* failed to set resource */
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
            log_error(LOG_DEFAULT,
                      "failed to enable " CARTNAME ", please set BIOS file.");
            vice_gtk3_message_error(GTK_WINDOW(parent),
                                    CARTNAME " Error",
                                    "Failed to enable " CARTNAME ", please set BIOS file.");
        }
    } else if (!enabled) {
        if (cartridge_disable(CARTRIDGE_MMC64) < 0) {
            log_error(LOG_DEFAULT, "failed to disable " CARTNAME ".");
            vice_gtk3_message_error(GTK_WINDOW(parent),
                                    CARTNAME " Error",
                                    "Failed to disable " CARTNAME ", please set BIOS file.");
        }
    }

    update_buttons_sensitivity();
}

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
        if (cartridge_save_image(CARTRIDGE_MMC64, filename) < 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    CARTRIDGE_NAME_MMC64 " Error",
                                    "Failed to save image as '%s'.",
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
    GtkWidget *dialog;

    dialog = vice_gtk3_save_file_dialog("Save " CARTRIDGE_NAME_MMC64 " image",
                                        NULL, TRUE, NULL,
                                        save_filename_callback,
                                        NULL);
    gtk_widget_show(dialog);
}

/** \brief  Handler for the "clicked" event of the Flush Image button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   unused
 */
static void on_flush_clicked(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *parent = gtk_widget_get_toplevel(widget);

    if (!GTK_IS_WINDOW(parent)) {
        parent = NULL;  /* make error dialog default to active window */
    }
    if (cartridge_flush_image(CARTRIDGE_MMC64) < 0) {
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                CARTNAME " Error",
                                "Failed to flush image.");
    }
}


/** \brief  Create widget to toggle MMC64 emulation
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_mmc64_enable_widget(void)
{
    GtkWidget *check;

    check = gtk_check_button_new_with_label("Enable " CARTNAME " emulation");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                 cartridge_type_enabled(CARTRIDGE_MMC64));
    g_signal_connect(G_OBJECT(check),
                     "toggled",
                     G_CALLBACK(on_enable_toggled),
                     NULL);
    return check;
}

/** \brief  Create widget to toggle the MMC64 flash jumper
 *
 * \return  GtkSwitch
 */
static GtkWidget *create_mmc64_jumper_widget(void)
{
    GtkWidget *sw;

    sw = vice_gtk3_resource_switch_new("MMC64_flashjumper");
    gtk_widget_set_hexpand(sw, FALSE);
    gtk_widget_set_vexpand(sw, FALSE);
    return sw;
}

/** \brief  Create widget to set the MMC64 revision
 *
 * \return  GtkGrid
 */
static GtkWidget *create_mmc64_revision_widget(void)
{
    GtkWidget *group;

    group = vice_gtk3_resource_radiogroup_new("MMC64_revision",
                                              revisions,
                                              GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_set_column_spacing(GTK_GRID(group), 8);
    return group;
}

/** \brief  Add BIOS widgets to main grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 * \param[in]   columns number of columns in \a grid, for proper column span
 *
 * \return  row in \a grid to add more widgets
 */
static int create_bios_image_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget  *label;
    GtkWidget  *bios_write;
    GtkWidget  *button_box;
    const char *title;

    /* header */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<b>" CARTRIDGE_NAME_MMC64 " BIOS</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    /* BIOS file chooser */
    label = gtk_label_new("BIOS filename");
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    bios_filename = vice_gtk3_resource_filechooser_new("MMC64BIOSfilename",
                                                       GTK_FILE_CHOOSER_ACTION_OPEN);
    title = "Select " CARTRIDGE_NAME_MMC64 " BIOS file";
    vice_gtk3_resource_filechooser_set_custom_title(bios_filename, title);
    vice_gtk3_resource_filechooser_set_callback(bios_filename, bios_callback);
    gtk_grid_attach(GTK_GRID(grid), label,         0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), bios_filename, 1, row, columns - 1, 1);
    row++;

    /* BIOS writes */
    bios_write = vice_gtk3_resource_check_button_new("MMC64_bios_write",
                                                     "Enable BIOS image writes");
    /* buttons */
    button_box   = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    flush_button = gtk_button_new_with_label("Flush image");
    save_button  = gtk_button_new_with_label("Save image as ..");

    g_signal_connect(G_OBJECT(flush_button),
                     "clicked",
                     G_CALLBACK(on_flush_clicked),
                     NULL);
    g_signal_connect(G_OBJECT(save_button),
                     "clicked",
                     G_CALLBACK(on_save_clicked),
                     NULL);

    gtk_box_pack_start(GTK_BOX(button_box), save_button,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), flush_button, FALSE, FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(button_box), 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), bios_write, 1, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), button_box, 2, row, columns - 2, 1);
    return row + 1;
}

/** \brief  Add widgets for the SD/MCC card
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 * \param[in]   columns number of columns in \a grid, for proper column span
 *
 * \return  row in \a grid to add more widgets
 */
static int create_card_image_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget  *label;
    GtkWidget  *card_writes;
    GtkWidget  *type;
    const char *title;

    /* header */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<b>" CARTNAME " SD/MMC Card</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    /* card image file */
    label = gtk_label_new("Card image file");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    card_filename = vice_gtk3_resource_filechooser_new("MMC64Imagefilename",
                                                       GTK_FILE_CHOOSER_ACTION_OPEN);
    title = "Select " CARTNAME " SD/MMC card image file";
    vice_gtk3_resource_filechooser_set_custom_title(card_filename, title);
    gtk_grid_attach(GTK_GRID(grid), label,         0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), card_filename, 1, row, columns - 1, 1);
    row++;

    /* card type */
    label = gtk_label_new("Card type");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    type = vice_gtk3_resource_radiogroup_new("MMC64_sd_type",
                                             card_types,
                                             GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_set_column_spacing(GTK_GRID(type), 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), type,  1, row, columns - 1, 1);
    row++;

    /* card writes */
    card_writes = vice_gtk3_resource_check_button_new("MMC64_RO",
                                                      "Enable SD/MMC card read-only");
    gtk_grid_attach(GTK_GRID(grid), card_writes, 1, row, columns - 1, 1);
    row++;
    return row + 1;
}


/** \brief  Create widget to control MMC64 resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_mmc64_widget_create(GtkWidget *parent)
{
    /* number of columns in grid */
#define NUM_COLS 4
    GtkWidget *grid;
    GtkWidget *enable_widget;
    GtkWidget *jumper_wrapper;
    GtkWidget *jumper_label;
    GtkWidget *jumper_widget;
    GtkWidget *revision_label;
    GtkWidget *revision_widget;
    GtkWidget *clockport_label;
    GtkWidget *clockport_widget;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    /* all rows need at least 8 pixels space between widgets, so we set the
     * minimum spacing here */
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* enable emulation check button */
    enable_widget = create_mmc64_enable_widget();

    /* jumper switch and label */
    jumper_wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(jumper_wrapper), 16);
    jumper_label   = gtk_label_new(CARTNAME " Flash jumper");
    jumper_widget  = create_mmc64_jumper_widget();
    gtk_widget_set_halign(jumper_label, GTK_ALIGN_END);
    gtk_widget_set_hexpand(jumper_label, TRUE);
    gtk_widget_set_halign(jumper_widget, GTK_ALIGN_END);
    gtk_widget_set_valign(jumper_widget, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(jumper_wrapper), jumper_label,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(jumper_wrapper), jumper_widget, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), enable_widget,  0, row, 2 ,1);
    gtk_grid_attach(GTK_GRID(grid), jumper_wrapper, 2, row, 2, 1);
    row++;

    row = create_bios_image_layout(grid, row, NUM_COLS);
    row = create_card_image_layout(grid, row, NUM_COLS);

    clockport_label  = gtk_label_new("ClockPort device");
    clockport_widget = clockport_device_widget_create("MMC64ClockPort");
    gtk_widget_set_halign(clockport_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(clockport_label, 16);
    gtk_widget_set_margin_top(clockport_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), clockport_label,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), clockport_widget, 1, row, 1, 1);
    row++;

    revision_label   = gtk_label_new(CARTNAME " Revision");
    revision_widget  = create_mmc64_revision_widget();
    gtk_widget_set_halign(revision_label, GTK_ALIGN_START);
    gtk_widget_set_valign(revision_widget, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), revision_label,   0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), revision_widget,  1, row, 1, 1);
#undef NUM_COLS

    update_buttons_sensitivity();

    gtk_widget_show_all(grid);
    return grid;
}

#undef CARTNAME
