/** \file   joystickmenupopup.c
 *  \brief  Create a menu to swap joysticks
 *
 * This module contains a popup menu that can be triggered via the statusbar's
 * joysticks widget. With it, the user can swap joystick ports.
 *
 *  \author Bas Wassink <b.wassink@ziggo.nl>
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

#include "actions-joystick.h"
#include "debug_gtk3.h"
#include "hotkeys.h"
#include "machine.h"
#include "resources.h"
#include "uiactions.h"
#include "uimenu.h"
#include "uisettings.h"
#include "widgethelpers.h"

#include "joystickmenupopup.h"


/** \brief  Determine if joystick swapping is possible
 *
 * \return  bool
 */
static gboolean joystick_swap_possible(void)
{
    switch (machine_class) {

        /* always TRUE */
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        /* x64dtv emulates the second joystick hack by default */
        case VICE_MACHINE_C64DTV:   /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:
            return TRUE;

        /* always FALSE */
        case VICE_MACHINE_VIC20:    /* fall through */
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_CBM6x0:
            return FALSE;

        /* should never get here */
        default:
            return FALSE;
    }
}


/** \brief  Handler for the 'activate' event of the "configure ..." menu item
 *
 * Opens the joystick configuration settings page.
 *
 * \param[in]   widget      menu item (unused)
 * \param[in]   user_data   extra event data (unused)
 */
static void on_configure_activate(GtkWidget *widget, gpointer user_data)
{
    ui_settings_dialog_show("input/joystick");
}


/** \brief  Trigger a UI action
 *
 * \param[in]   item    menu item (unused)
 * \param[in]   action  UI action ID
 */
static void trigger_ui_action(GtkWidget *item, gpointer action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
}


/** \brief  Create joystick menu popup for the statusbar
 *
 * \return  GtkMenu
 */
GtkWidget *joystick_menu_popup_create(void)
{
    GtkWidget *menu;
    GtkWidget *item;
    int keyset = 0;
    int mouse = 0;

    menu = gtk_menu_new();

    if (joystick_swap_possible()) {
        /* Swap joysticks */
        item = gtk_check_menu_item_new_with_label("Swap joysticks");
        vhk_gtk_set_menu_item_accel_label(item, ACTION_SWAP_CONTROLPORT_TOGGLE);
        gtk_container_add(GTK_CONTAINER(menu), item);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
                                       (gboolean)ui_get_controlport_swapped());
        g_signal_connect(item,
                         "activate",
                         G_CALLBACK(trigger_ui_action),
                         GINT_TO_POINTER(ACTION_SWAP_CONTROLPORT_TOGGLE));
    }

    /* Enable keyset joysticks */
    item = gtk_check_menu_item_new_with_label("Allow keyset joysticks");
    vhk_gtk_set_menu_item_accel_label(item, ACTION_KEYSET_JOYSTICK_TOGGLE);
    resources_get_int("KeySetEnable", &keyset);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (gboolean)keyset);
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_KEYSET_JOYSTICK_TOGGLE));

    /* Enable mouse grab */
    item = gtk_check_menu_item_new_with_label("Enable mouse grab");
    vhk_gtk_set_menu_item_accel_label(item, ACTION_MOUSE_GRAB_TOGGLE);
    resources_get_int("Mouse", &mouse);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (gboolean)mouse);
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_MOUSE_GRAB_TOGGLE));

    item = gtk_separator_menu_item_new();
    gtk_container_add(GTK_CONTAINER(menu), item);

    item = gtk_menu_item_new_with_label("Configure joysticks ...");
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "activate",
                     G_CALLBACK(on_configure_activate),
                    NULL);

    gtk_widget_show_all(menu);
    return menu;
}
