/** \file   sounddriverwidget.c
 * \brief   GTK3 sound driver widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SoundDeviceName     all
 * $VICERES SoundDeviceArg      all
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
 */


#include "vice.h"

#include <gtk/gtk.h>

#include "basewidgets.h"
#include "lib.h"
#include "ui.h"
#include "resources.h"
#include "vsync.h"
#include "sound.h"
#include "widgethelpers.h"
#include "debug_gtk3.h"

#include "sounddriverwidget.h"


/** \brief  Handler for the "changed" event of the device combobox
 *
 * \param[in]   widget      widget triggering the event
 * \param[in]   user_data   data for the event (unused)
 */
static void on_device_changed(GtkWidget *widget, gpointer user_data)
{
    const char *id;

    id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    resources_set_string("SoundDeviceName", id);
}

/** \brief  Create combobox with sound devices
 *
 * \return  combobox
 */
static GtkWidget *create_device_combobox(void)
{
    GtkWidget  *combo;
    int         i;
    int         count = sound_device_num();
    const char *current_device = NULL;

    resources_get_string("SoundDeviceName", &current_device);
    combo = gtk_combo_box_text_new();
    for (i = 0; i < count; i++) {
        const char *device = sound_device_name(i);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
                                  device,
                                  device);
    }

    /* set active device */
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), current_device);

    /* now connect event handler */
    g_signal_connect(G_OBJECT(combo),
                     "changed",
                     G_CALLBACK(on_device_changed),
                     NULL);

    return combo;
}

/** \brief  Create the 'device driver argument' text entry box
 *
 * \return  GtkEntry
 */
static GtkWidget *create_argument_entry(void)
{
    return vice_gtk3_resource_entry_new("SoundDeviceArg");
}

/** \brief  Create left-aligned labe with Pango markup
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


/** \brief  Create the sound driver/device widget
 *
 * \return  driver widget (GtkGrid)
 */
GtkWidget *sound_driver_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *device;
    GtkWidget *label;
    GtkWidget *args;

    /* row spacing of 8 works fine here to separate the text entries */
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = create_label("<b>Host device</b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

    label  = create_label("Device name");
    device = create_device_combobox();
    gtk_widget_set_hexpand(device, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), device, 1, 1, 1, 1);

    label = create_label("Driver arguments");
    args  = create_argument_entry();
    gtk_widget_set_hexpand(args, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), args,  1, 2, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
