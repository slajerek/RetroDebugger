/** \file   settings_sampler.c
 * \brief   Widget to control sampler settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SamplerDevice
 * $VICERES SamplerGain
 * $VICERES SampleName
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

#include "resources.h"
#include "sampler.h"
#include "vice_gtk3.h"

#include "settings_sampler.h"


/** \brief  Stepping for the gain slider (when using cursor left/right) */
#define GAIN_STEP   25


/** \brief  Handler for the "changed" event of the devices combo box
 *
 * \param[in]   combo       combo box with devices
 * \param[in]   user_data   extra data (unused)
 */
static void on_device_changed(GtkComboBoxText *combo, gpointer user_data)
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

    resources_set_int("SamplerDevice", index);
}

/** \brief  Create left-aligned label using Pango markup
 *
 * \param[in]   text    label text
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create combo box for the devices list
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_device_combo(void)
{
    GtkWidget        *combo;
    sampler_device_t *devices;
    int               current = 0;
    int               index;

    resources_get_int("SamplerDevice", &current);

    combo   = gtk_combo_box_text_new();
    devices = sampler_get_devices();
    for (index = 0; devices[index].name != NULL; index++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
                                  devices[index].name,
                                  devices[index].name);
        if (index == current) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), index);
        }
    }

    g_signal_connect(G_OBJECT(combo),
                     "changed",
                     G_CALLBACK(on_device_changed),
                     NULL);
    return combo;
}


/** \brief  Create widget to control sampler settings
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 *
 * \todo    Use resourcebrowser to control the "SampleFile" resource
 */
GtkWidget *settings_sampler_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *device;
    GtkWidget *gain;
    GtkWidget *media;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    /* sampler device list */
    label  = create_label("Sampler device");
    device = create_device_combo();
    gtk_grid_attach(GTK_GRID(grid), label,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), device, 1, 1, 1, 1);

    /* sampler gain */
    label = create_label("Sampler gain");

    gain = vice_gtk3_resource_scale_custom_new_printf("%s",
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       0,
                                                       SAMPLER_GAIN_MAX,
                                                       0,
                                                       (SAMPLER_GAIN_MAX * 100) / SAMPLER_GAIN_ONE,
                                                       GAIN_STEP,
                                                       "%3.0f%%",
                                                       "SamplerGain");
    gtk_widget_set_hexpand(gain, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(gain), GTK_POS_RIGHT);

    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gain,  1, 2, 1, 1);

    /* sampler input file */
    label = create_label("Sampler media file");
    media = vice_gtk3_resource_filechooser_new("SampleName",
                                               GTK_FILE_CHOOSER_ACTION_OPEN);
    vice_gtk3_resource_filechooser_set_custom_title(media,
                                                    "Select a media file");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), media, 1, 3, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
