/** \file   videopalettewidget.c
 * \brief   Widget to select palette
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * FIXME:   Note sure x64dtv here:
 *
 * $VICERES CrtcPaletteFile         xpet xcbm2
 * $VICERES CrtcExternalPalette     xpet xcbm2
 * $VICEREs TEDPaletteFile          xplus4
 * $VICERES TEDExternalPalette      xplu4
 * $VICERES VDCPaletteFile          x128
 * $VICERES VDCExternalPalette      x128
 * $VICERES VICPaletteFile          xvic
 * $VICERES VICExternalPalette      xvic
 * $VICERES VICIIPaletteFile        x64 x64sc xscpu64 xd64tv xcbm5x0
 * $VICERES VICIIExternalPalette    x64 x64sc xscpu64 xd64tv xcbm5x0
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
#include <string.h>

#include "vice_gtk3.h"
#include "debug_gtk3.h"
#include "palette.h"
#include "resources.h"
#include "lib.h"
#include "ui.h"
#include "util.h"

#include "videopalettewidget.h"


/** \brief  video chip prefix, used to construct proper resource names
 */
static char chip_name[32];

/** \brief  Internal palette radio button */
static GtkWidget *radio_internal = NULL;

/** \brief  External palette radio button */
static GtkWidget *radio_external = NULL;

/** \brief  External palette combo box */
static GtkWidget *combo_external = NULL;

/** \brief  Browse button for custom palette files
 *
 * We can't use the resource browser widget since we use a combobox, not a
 * text entry.
 */
static GtkWidget *button_custom = NULL;


/** \brief  Handler for the "toggled" event of the "External" radio button
 *
 * \param[in]   radio   External radio button
 * \param[in]   data    extra event data (unused)
 */
static void on_external_toggled(GtkWidget *radio, gpointer data)
{
    int external = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio)) ? 1 : 0;

    resources_set_int_sprintf("%sExternalPalette", external, chip_name);
}

/** \brief  Handler for the "changed" event of the palettes combo box
 *
 * \param[in]   combo       combo box
 * \param[in]   user_data   extra event data (unused)
 */
static void on_combo_changed(GtkComboBox *combo, gpointer user_data)
{
    const char *id = gtk_combo_box_get_active_id(combo);

    /* the event handler of the external radio will set ${CHIP}ExternalPalette */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_external), TRUE);
    resources_set_string_sprintf("%sPaletteFile", id, chip_name);
}

/** \brief  Callback for the custom palette file chooser
 *
 * \param[in,out]   dialog      file chooser dialog
 * \param[in,out]   filename    palette filename (NULL to cancel)
 * \param[in]       data        extra data (unused)
 */
static void browse_filename_callback(GtkDialog *dialog,
                                     gchar *filename,
                                     gpointer data)
{
    if (filename != NULL) {
        resources_set_string_sprintf("%sPaletteFile", filename, chip_name);

        /* add to combo box */
        gtk_combo_box_text_insert(GTK_COMBO_BOX_TEXT(combo_external), 0,
                filename, filename);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_external), 0);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the "clicked" event of the "browse ..." button
 *
 * Opens a custom palette file and adds the file name to the combo box.
 *
 * \param[in]   button      browse button (unused)
 * \param[in]   user_data   extra event data (unused)
 */
static void on_browse_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget  *dialog;
    const char *patterns[] = { "*.vpl", NULL };

    dialog = vice_gtk3_open_file_dialog("Open palette file",
                                        "Palette files",
                                        patterns,
                                        NULL,
                                        browse_filename_callback,
                                        NULL);
    gtk_widget_show(dialog);
}

/** \brief  Create combo box with available palettes for the current chip
 *
 * If the file in the resource "${chip}PaletteFile" doesn't match any of the
 * palette files provided by VICE, it is assumed to be a user-defined custom
 * palette and inserted at the top of the combo box.
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_combo_box(void)
{
    GtkWidget            *combo;
    const palette_info_t *list;
    int                   index;
    int                   row = 0;
    const char           *ext = NULL;
    const char           *current = NULL;
    char                 *current2 = NULL;
    gboolean              found = FALSE;

    resources_get_string_sprintf("%sPaletteFile", &current, chip_name);

    if (current && *current != '\0') {
        /*printf("create_combo_box (%sPaletteFile) current:'%s'\n", chip_name, current);*/
        /* if the <CHIP>PaletteFile resource points to a file,
           - create an alternative name without extension, if the file has a
             .vpl extension
           - create an alternative name with .vpl extension, if the file has no
             extension
        */
        ext = util_get_extension(current);
        if ((ext != NULL) && (strcmp(ext, "vpl"))) {
            /* current already has .vpl extension, so remove it for the extra string */
            size_t n = strlen(current);
            current2 = lib_strdup(current);
            current2[n - 4] = 0;
        } else if (ext == NULL) {
            /* current has no extension, so add it for the extra string */
            current2 = lib_strdup(current);
            util_add_extension(&current2, "vpl");
        }
        /*printf("create_combo_box (%sPaletteFile) current2:'%s'\n", chip_name, current2);*/
    }

    /* loop over all items in the palette info list, filter out the ones that
       match the chip we want to get the list for, populate the combo box with
       names and respective file names */
    row = 0;
    list = palette_get_info_list();
    combo = gtk_combo_box_text_new();
    for (index = 0; list[index].chip != NULL; index++) {
        if (g_strcmp0(list[index].chip, chip_name) == 0) {
            /* got a valid entry */
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
                                     list[index].file,
                                     list[index].name);
            /*printf("list[%d].file:%s\n", index, list[index].file);*/
            /* if file of the current item matches one of the names generated
               above (from <CHIP>PaletteFile resource), then make it the
               active item */
            if ((current && (g_strcmp0(list[index].file, current) == 0)) ||
                (current2 && (g_strcmp0(list[index].file, current2) == 0))) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(combo), row);
                /*printf("added in row %d: %s:%s\n", row, current, current2);*/
                found = TRUE;
            }
            row++;
        }
    }

    lib_free(current2);

    /* if we didn't find `current` in the list of VICE palette files, add it as
     * a custom user-defined file */
    if ((!found) && current != NULL && *current != '\0') {
        gtk_combo_box_text_insert(GTK_COMBO_BOX_TEXT(combo), 0, current, current);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    }

    g_signal_connect(combo,
                     "changed",
                     G_CALLBACK(on_combo_changed),
                     NULL);
    return combo;
}

/** \brief  Create "browse ..." button to select a palette file
 *
 * \return  GtkButton
 */
static GtkWidget *create_browse_button(void)
{
    GtkWidget *button;

    button = gtk_button_new_with_label("Browse ...");
    g_signal_connect(button, "clicked", G_CALLBACK(on_browse_clicked), NULL);
    return button;
}


/** \brief  Create video palette widget
 *
 * \param[in]   chip    chip name (used as prefix for resources)
 *
 * \return  GtkGrid
 */
GtkWidget *video_palette_widget_create(const char *chip)
{
    GtkWidget *grid;
    GSList    *group = NULL;
    char       title[64];
    int        external = 0;

    /* make copy of chip name */
    strncpy(chip_name, chip, sizeof chip_name);
    chip_name[sizeof chip_name - 1u] = '\0';

    g_snprintf(title, sizeof title, "%s Palette", chip);
    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, title, 4);
    vice_gtk3_grid_set_title_margin(grid, 8);

    radio_internal = gtk_radio_button_new_with_label(group, "Internal");
    radio_external = gtk_radio_button_new_with_label(group, "External");
    gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio_external),
                                GTK_RADIO_BUTTON(radio_internal));

    combo_external = create_combo_box();
    gtk_widget_set_hexpand(combo_external, TRUE);
    button_custom = create_browse_button();

    gtk_grid_attach(GTK_GRID(grid), radio_internal, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), radio_external, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo_external, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_custom,  2, 2, 1, 1);

    resources_get_int_sprintf("%sExternalPalette", &external, chip);
    if (external) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_external), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_internal), TRUE);
    }

    /* gets called on toggling off AND on */
    g_signal_connect(radio_external,
                     "toggled",
                     G_CALLBACK(on_external_toggled),
                     NULL);

    gtk_widget_show_all(grid);
    return grid;
}
