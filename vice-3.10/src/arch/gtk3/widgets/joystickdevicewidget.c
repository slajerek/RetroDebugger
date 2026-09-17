/** \file   joystickdevicewidget.c
 * \brief   Widget to select a joystick device
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES JoyDevice1      -xcbm2 -xpet -vsid
 * $VICERES JoyDevice2      -xcbm2 -xpet -vsid
 * $VICERES JoyDevice3      -xcbm5x0 -vsid
 * $VICERES JoyDevice4      -xcbm5x0 -xplus4 -vsid
 * $VICERES JoyDevice5      xplus4
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
#include <stdlib.h>

#include "vice_gtk3.h"
#include "joystick.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "filechooserhelpers.h"

#include "joystickdevicewidget.h"


/** \brief  Struct containing device name and id
 */
typedef struct device_info_s {
    const char *name;   /**< device name */
    int         id;     /**< device ID (\see joy.h) */
} device_info_t;


/** \brief  List of available input devices on the host
 */
static const device_info_t predefined_device_list[] = {
    { "None",       JOYDEV_NONE },
    { "Numpad",     JOYDEV_NUMPAD },
    { "Keyset A",   JOYDEV_KEYSET1 },
    { "Keyset B",   JOYDEV_KEYSET2 },
    { NULL,         -1 }
};


/** \brief  Create joystick device selection widget
 *
 * \param[in]   device  device number (0-4)
 * \param[in]   title   widget title
 *
 * \return  GtkGrid
 */
GtkWidget *joystick_device_widget_create(int device, const char *title)
{
    GtkWidget  *grid;
    GtkWidget  *label;
    GtkWidget  *combo;
    int         id;
    const char *name;
    char        buffer[256]; /* large since we use it for resource name as well */
    int         dev;
    int         current = 0;

    resources_get_int_sprintf("JoyDevice%d", &current, device + 1);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    g_snprintf(buffer, sizeof buffer, "<b>%s</b>", title);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), buffer);
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    /* no new_sprintf() method, construct resource name: */
    g_snprintf(buffer, sizeof buffer, "JoyDevice%d", device + 1);
    combo = vice_gtk3_resource_combo_int_new(buffer, NULL /* empty model */);
    gtk_widget_set_hexpand(combo, TRUE);

    /* add predefined standard devices */
    for (dev = 0; (name = predefined_device_list[dev].name) != NULL; dev++) {
        id = predefined_device_list[dev].id;

        vice_gtk3_resource_combo_int_append(combo, id, name);
        if (id == current) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), dev);
        }
    }
    /* add more devices (joysticks) */
    joystick_ui_reset_device_list();
    while ((name = joystick_ui_get_next_device_name(&id)) != NULL) {
        /* convert name from locale to UTF-8 to be used in the list */
        char *utf8 = file_chooser_convert_from_locale(name);

        vice_gtk3_resource_combo_int_append(combo, id, utf8);
        g_free(utf8);
        if (id == current) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), dev);
        }
        dev++;
    }

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set joystick device \a widget to \a id
 *
 * \param[in,out]   widget  joystick device widget
 * \param[in]       id      new value for the \a widget
 */
void joystick_device_widget_update(GtkWidget *widget, int id)
{
    GtkWidget *combo;

    /* get combo box widget */
    combo = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    if (combo != NULL && GTK_IS_COMBO_BOX_TEXT(combo)) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), id);
    }
}
