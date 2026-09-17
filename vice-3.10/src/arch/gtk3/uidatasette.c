/** \file   uidatasette.c
 * \brief   Create independent datasette control widgets
 *
 * \author  Michael C. Martin <mcmartin@gmail.com>
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

#include "datasette.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "tapeport.h"
#include "uiactions.h"
#include "uisettings.h"
#include "uitapeattach.h"
#include "vice_gtk3.h"

#include "uidatasette.h"


/** \brief  Table of UI action codes for datasette control codes and port
 */
static const int action_ids[][2] = {
    /* DATASETTE_CONTROL_STOP */
    { ACTION_TAPE_STOP_1,           ACTION_TAPE_STOP_2 },
    /* DATASETTE_CONTROL_START */
    { ACTION_TAPE_PLAY_1,           ACTION_TAPE_PLAY_2 },
    /* DATASETTE_CONTROL_FORWARD */
    { ACTION_TAPE_FFWD_1,           ACTION_TAPE_FFWD_2 },
    /* DATASETTE_CONTROL_REWIND */
    { ACTION_TAPE_REWIND_1,         ACTION_TAPE_REWIND_2 },
    /* DATASETTE_CONTROL_RECORD */
    { ACTION_TAPE_RECORD_1,         ACTION_TAPE_RECORD_2 },
    /* DATASETTE_CONTROL_RESET */
    { ACTION_TAPE_RESET_1,          ACTION_TAPE_RESET_2 },
    /* DATASETTE_CONTROL_RESET_COUNTER */
    { ACTION_TAPE_RESET_COUNTER_1,  ACTION_TAPE_RESET_COUNTER_2 }
};


/** \brief  Translate datasette command codes to UI action IDs
 *
 * \param[in]   command datasette command code
 * \param[in]   port    port number (1 or 2)
 *
 * \return  action ID or `ACTION_NONE` for invalid \a control or \a port values
 *
 * \see src/datasette/datasette.h for the command codes
 */
int ui_datasette_command_to_action(int port, int command)
{
    if (command < DATASETTE_CONTROL_STOP || command > DATASETTE_CONTROL_RESET_COUNTER) {
        return ACTION_NONE;
    }
    if (port < 1 || port > 2) {
        return ACTION_NONE;
    }
    return action_ids[command][port - 1];
}


/** \brief  Handler for the 'activate' event of the "Configure" menu item
 *
 * Pop up the settings UI and select "I/O extensions" -> "Tapeport devices"
 *
 * \param[in]   widget  menu item triggering the event (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_configure_activate(GtkWidget *widget, gpointer data)
{
    ui_settings_dialog_show("peripheral/tapeport-devices");
}


/** \brief  Handler for the 'activate' event of a menu item
 *
 * Trigger a UI action.
 *
 * \param[in]   action  UI action ID
 */
static void trigger_ui_action(GtkWidget *item, gpointer action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
}


/** \brief  Create datasette control menu
 *
 * \param[in]   port    datasette port number (1 or 2)
 *
 * \return  GtkMenu with datasette controls
 *
 * \todo    Update for second datasette on PET
 */
GtkWidget *ui_create_datasette_control_menu(int port)
{
    GtkWidget *menu, *item, *menu_items[DATASETTE_CONTROL_RESET_COUNTER+1];
    int i;
    gchar buffer[256];
    int action_id;

    menu = gtk_menu_new();

    /* Attach */
    action_id = port == 1 ? ACTION_TAPE_ATTACH_1 : ACTION_TAPE_ATTACH_2;
    if (machine_class == VICE_MACHINE_PET) {
        g_snprintf(buffer, sizeof(buffer), "Attach tape #%d image ...", port);
        item = gtk_menu_item_new_with_label(buffer);
    } else {
        item = gtk_menu_item_new_with_label("Attach tape image ...");
    }
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect_unlocked(item,
                              "activate",
                              G_CALLBACK(trigger_ui_action),
                              GINT_TO_POINTER(action_id));

    /* Detach */
    action_id = port == 1 ? ACTION_TAPE_DETACH_1 : ACTION_TAPE_DETACH_2;
    if (machine_class == VICE_MACHINE_PET) {
        g_snprintf(buffer, sizeof(buffer), "Detach tape #%d image", port);
        item = gtk_menu_item_new_with_label(buffer);
    } else {
        item = gtk_menu_item_new_with_label("Detach tape image");
    }
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "activate",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(action_id));

    gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

    /* Datasette control items */
    menu_items[0] = gtk_menu_item_new_with_label("Stop");
    menu_items[1] = gtk_menu_item_new_with_label("Play");
    menu_items[2] = gtk_menu_item_new_with_label("Forward");
    menu_items[3] = gtk_menu_item_new_with_label("Rewind");
    menu_items[4] = gtk_menu_item_new_with_label("Record");
    menu_items[5] = gtk_menu_item_new_with_label("Reset");
    menu_items[6] = gtk_menu_item_new_with_label("Reset Counter");
    for (i = 0; i <= DATASETTE_CONTROL_RESET_COUNTER; ++i) {
        action_id = ui_datasette_command_to_action(port, i);
        gtk_container_add(GTK_CONTAINER(menu), menu_items[i]);
        g_signal_connect(menu_items[i],
                         "activate",
                         G_CALLBACK(trigger_ui_action),
                         GINT_TO_POINTER(action_id));
    }

    /* add "configure tapeport devices" */
    gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
    item = gtk_menu_item_new_with_label("Configure tapeport devices ...");
    g_signal_connect(item,
                     "activate",
                     G_CALLBACK(on_configure_activate),
                     NULL);
    gtk_container_add(GTK_CONTAINER(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}


/** \brief  Update sensitivity of the datasettet controls
 *
 * Disables/enables the datasette controls (keys), depending on whether the
 * datasette is enabled.
 *
 * \param[in]   menu    tape menu
 * \param[in]   port    tape port number (1 or 2)
 */
void ui_datasette_update_sensitive(GtkWidget *menu, int port)
{
    int datasette;
    int y;
    GList *children;
    GList *controls;

    resources_get_int_sprintf("TapePort%dDevice", &datasette, port);

    /* get all children of the menu */
    children = gtk_container_get_children(GTK_CONTAINER(menu));
    /* skip 'Attach', 'Detach' and separator item */
    controls = g_list_nth(children, 3);

    for (y = 0; y <= DATASETTE_CONTROL_RESET_COUNTER; y++) {
        GtkWidget *item = controls->data;
        gtk_widget_set_sensitive(item, datasette == TAPEPORT_DEVICE_DATASETTE);
        controls = g_list_next(controls);
    }

    /* free list of children */
    g_list_free(children);
}
