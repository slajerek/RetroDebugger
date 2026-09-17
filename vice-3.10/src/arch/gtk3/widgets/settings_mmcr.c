/** \file   settings_mmcr.c
 * \brief   Settings widget to control MMC Replay resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES MMCRCardImage   x64 x64sc xscpu64 x128
 * $VICERES MMCREEPROMImage x64 x64sc xscpu64 x128
 * $VICERES MMCREEPROMRW    x64 x64sc xscpu64 x128
 * $VICERES MMCRRescueMode  x64 x64sc xscpu64 x128
 * $VICERES MMCRImageWrite  x64 x64sc xscpu64 x128
 * $VICERES MMCRCardRW      x64 x64sc xscpu64 x128
 * $VICERES MMCRSDType      x64 x64sc xscpu64 x128
 * $VICERES MMCRClockPort   x64 x64sc xscpu64 x128
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
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_mmcr.h"

/** \brief  Temporary define to make editing easier */
#define CARTNAME    CARTRIDGE_NAME_MMC_REPLAY


/** \brief  List of memory card types
 */
static const vice_gtk3_radiogroup_entry_t card_types[] = {
    { "Auto", MMCR_TYPE_AUTO },
    { "MMC",  MMCR_TYPE_MMC },
    { "SD",   MMCR_TYPE_SD },
    { "SDHC", MMCR_TYPE_SDHC },
    { NULL,   -1 }
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
        if (cartridge_save_image(CARTRIDGE_MMC_REPLAY, filename) < 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    CARTNAME " Error",
                                    "Failed to save image as '%s'",
                                    filename);
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the "clicked" event of the Save Image button
 *
 * \param[in]   widget      button (unused)
 * \param[in]   user_data   extra event data (unused)
 */
static void on_save_clicked(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog;

    dialog = vice_gtk3_save_file_dialog("Save " CARTNAME " image",
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
    if (cartridge_flush_image(CARTRIDGE_MMC_REPLAY) < 0) {
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                CARTNAME "Error",
                                "Failed to flush image.");
    }
}

/** \brief  Create grid with switch and label for the MMCRRescueMode resource
 *
 * \return  GtkGrid
 */
static GtkWidget *create_rescue_mode_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *rescue;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    label = gtk_label_new("Rescue mode");
    gtk_widget_set_halign(label, GTK_ALIGN_END);

    rescue = vice_gtk3_resource_switch_new("MMCRRescueMode");
    gtk_widget_set_hexpand(rescue, FALSE);
    gtk_widget_set_vexpand(rescue, FALSE);
    gtk_widget_set_halign(rescue, GTK_ALIGN_END);
    gtk_widget_set_valign(rescue, GTK_ALIGN_CENTER);

    gtk_grid_attach(GTK_GRID(grid), label,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rescue, 1, 0, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Add EEPROM widgets to main grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 * \param[in]   columns number of columns in \a grid, for proper column span
 *
 * \return  row in \a grid to add more widgets
 */
static int create_eeprom_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget  *label;
    GtkWidget  *eeprom;
    GtkWidget  *readwrite;
    const char *title;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<b>" CARTNAME " EEPROM</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    label  = gtk_label_new("Image file");
    eeprom = vice_gtk3_resource_filechooser_new("MMCREEPROMImage",
                                                GTK_FILE_CHOOSER_ACTION_OPEN);
    title  = "Select " CARTNAME " EEPROM image file";
    vice_gtk3_resource_filechooser_set_custom_title(eeprom, title);
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), label,  0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), eeprom, 1, row, columns - 1, 1);
    row++;

    /* add RW widget */
    readwrite = vice_gtk3_resource_check_button_new("MMCREEPROMRW",
                                                    "Enable writes to EEPROM image");
    gtk_grid_attach(GTK_GRID(grid), readwrite, 1, row, columns - 1, 1);

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
static int create_card_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget  *label;
    GtkWidget  *image;
    GtkWidget  *card_writes;
    GtkWidget  *card_type;
    const char *title;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<b>" CARTNAME "SD/MMC Card</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    label = gtk_label_new("Image file");
    image = vice_gtk3_resource_filechooser_new("MMCRCardImage",
                                               GTK_FILE_CHOOSER_ACTION_OPEN);
    title = "Select " CARTNAME " SD/MMC card image file";
    vice_gtk3_resource_filechooser_set_custom_title(image, title);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), image, 1, row, columns - 1, 1);
    row++;

    card_writes = vice_gtk3_resource_check_button_new("MMCRCardRW",
                                                      "Enable SD/MMC card writes");

    label = gtk_label_new("Card type");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    card_type = vice_gtk3_resource_radiogroup_new("MMCRSDType",
                                                  card_types,
                                                  GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_set_column_spacing(GTK_GRID(card_type), 16);
    gtk_grid_attach(GTK_GRID(grid), label,     0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), card_type, 1, row, columns - 1, 1);
    row++;

    gtk_grid_attach(GTK_GRID(grid), card_writes, 1, row, columns - 1, 1);

    return row + 1;
}

/** \brief  Add cartridge image widgets to main grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 * \param[in]   columns number of columns in \a grid, for proper column span
 *
 * \return  row in \a grid to add more widgets
 */
static int create_cart_image_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget *label;
    GtkWidget *write_back;
    GtkWidget *box;
    GtkWidget *save;
    GtkWidget *flush;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<b>" CARTNAME " Cartridge Image</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    write_back = vice_gtk3_resource_check_button_new("MMCRImageWrite",
                                                     "Save image when changed");
    gtk_grid_attach(GTK_GRID(grid), write_back, 0, row, 2, 1);


    box =   gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    flush = gtk_button_new_with_label("Flush image");
    save =  gtk_button_new_with_label("Save image as ..");

    g_signal_connect(G_OBJECT(flush),
                     "clicked",
                     G_CALLBACK(on_flush_clicked),
                     NULL);
    g_signal_connect(G_OBJECT(save),
                     "clicked",
                     G_CALLBACK(on_save_clicked),
                     NULL);

    gtk_box_pack_start(GTK_BOX(box), save,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), flush, FALSE, FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(box), 8);
    gtk_widget_set_halign(box, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), box, 2, row, columns - 2, 1);

    return row + 1;
}


/** \brief  Create widget to control MMC Replay resources
 *
 * \param[in]   parent  parent widget (used for dialogs)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_mmcr_widget_create(GtkWidget *parent)
{
#define NUM_COLS 4
    GtkWidget *grid;
    GtkWidget *rescue_widget;
    GtkWidget *clockport_label;
    GtkWidget *clockport_widget;
    int        row = 0;

    grid = vice_gtk3_grid_new_spaced(8, 8);

    rescue_widget = create_rescue_mode_widget();
    gtk_widget_set_halign(rescue_widget, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), rescue_widget, 0, row, NUM_COLS, 1);
    row++;

    row = create_eeprom_layout    (grid, row, NUM_COLS);
    row = create_card_layout      (grid, row, NUM_COLS);
    row = create_cart_image_layout(grid, row, NUM_COLS);
#if 0
    gtk_grid_attach(GTK_GRID(grid), create_card_type_widget(), 0, row, NUM_COLS, 1);
#endif
    clockport_label  = gtk_label_new("ClockPort device");
    clockport_widget = clockport_device_widget_create("MMCRClockPort");
    gtk_widget_set_margin_top(clockport_label, 16);
    gtk_widget_set_margin_top(clockport_widget, 16);
    gtk_widget_set_halign(clockport_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), clockport_label,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), clockport_widget, 1, row, 1, 1);

#undef NUM_COLS
    gtk_widget_show_all(grid);
    return grid;
}

#undef CARTNAME
