/** \file   mididevicewidget.c
 * \brief   Widget to select a midi device
 *
 * \author  groepaz <groepaz@gmx.net>
 */

/*
 * $VICERES MIDIInDev       x64 x64sc xscpu64 x128 xvic
 * $VICERES MIDIOutDev      x64 x64sc xscpu64 x128 xvic
 * (Windows only)
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

#if defined(WINDOWS_COMPILE)

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "vice_gtk3.h"

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "filechooserhelpers.h"

#include "mididrv.h"
#include "mididevicewidget.h"


/** \brief  Struct containing device name and id
 */
typedef struct device_info_s {
    const char *name;       /**< device name */
    int         id;         /**< device ID */
} device_info_t;


/** \brief  Maximum devices
 *
 * Hopefully this is enough :)
 */
#define MAX_EXTRA_DEVICES 32


/** \brief  List of detected input devices on the host
 */
static device_info_t device_list[MAX_EXTRA_DEVICES] = {
    { NULL, -1 }
};


/** \brief  Handler for the "changed" event of the combo box
 *
 * \param[in]   combo       combo box
 * \param[in]   user_data   device (0: in, 1: out)
 */
static void on_device_changed(GtkComboBoxText *combo, gpointer user_data)
{
    const char *id_str;
    int device;

    id_str = gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo));
    device = GPOINTER_TO_INT(user_data);

    resources_set_string_sprintf("Midi%sDev", id_str, (device == 0) ? "In" : "Out");
}


/** \brief  Create MIDI device selection widget
 *
 * \param[in]   device  device (0: in, 1: out)
 *
 * \return  GtkGrid
 */
GtkWidget *midi_device_widget_create(int device)
{
    GtkWidget *combo;
    int id;
    int i;
    const char *current_str;

    resources_get_string_sprintf("Midi%sDev", &current_str, (device == 0) ? "In" : "Out");

    combo = gtk_combo_box_text_new();
    gtk_widget_set_hexpand(combo, TRUE);

    /* add devices */
    mididrv_ui_reset_device_list(device);
    for (i = 0; (device_list[i].name = mididrv_ui_get_next_device_name(device, &id)) != NULL; i++) {
        char id_str[32];
        gchar *utf8;

        if (i >= MAX_EXTRA_DEVICES) {
            log_error(LOG_DEFAULT, "too many MIDI %s devices", (device == 0) ? "input" : "output");
            break;
        }

        /* convert name from locale to UTF-8 to be used in the list */
        utf8 = file_chooser_convert_from_locale(device_list[i].name);

        device_list[i].id = id;
        g_snprintf(id_str, 32, "%d", device_list[i].id);

        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
                                  id_str,
                                  utf8);
        g_free(utf8);

        if (strcmp(id_str, current_str) == 0) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
        }
    }
    g_signal_connect(combo, "changed", G_CALLBACK(on_device_changed),
                     GINT_TO_POINTER(device));

    gtk_widget_show_all(combo);
    return combo;
}

#endif /* WINDOWS_COMPILE */
