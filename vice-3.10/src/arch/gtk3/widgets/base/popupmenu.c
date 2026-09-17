/** \file   popupmenu.c
 * \brief   Gtk3 popup menu helpers
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#include "hotkeys.h"
#include "uiactions.h"

#include "popupmenu.h"


/** \brief  Handler for the 'activate' event of a UI action menu item
 *
 * Triggers UI action \a action.
 *
 * \param[in]   item    menu item (unused)
 * \param[in]   action  UI action ID
 */
static void on_action_item_activate(GtkWidget *item, gpointer action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
}

/** \brief  Handler for the 'activate' event of a menu item with custom handler
 *
 * Triggers UI action \a action.
 *
 * \param[in]   item    menu item
 * \param[in]   action  UI action ID
 */
static void on_custom_item_activate(GtkWidget *item, gpointer action)
{
    void (*callback)(int) = g_object_get_data(G_OBJECT(item), "CustomHandler");
    if (callback != NULL) {
        callback(GPOINTER_TO_INT(action));
    }
}


/** \brief  Create menu item that triggers a UI action
 *
 * Creates menu item to trigger \a action, setting the appropriate
 * accelerator if a hotkey is defined for \a action.
 *
 * \param[in]   text    text for the item
 * \param[in]   action  UI action ID
 */
GtkWidget *popup_menu_item_action_new(const char *text, int action)
{
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(text);
    vhk_gtk_set_menu_item_accel_label(item, action);
    g_signal_connect(G_OBJECT(item),
                     "activate",
                     G_CALLBACK(on_action_item_activate),
                     GINT_TO_POINTER(action));
    return item;
}


/** \brief  Add separator item to menu
 *
 * \param[in]   menu    menu
 */
void popup_menu_add_separator(GtkWidget *menu)
{
    gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
}


/** \brief  Get item of menu by index
 *
 * \param[in]   menu    popup menu
 * \param[in]   n       index of item to retrieve
 *
 * \return  GtkMenuItem or `NULL` when not found
 */
GtkWidget *popup_menu_nth_item(GtkWidget *menu, guint n)
{
    GList *items;
    gpointer data = NULL;

    items = gtk_container_get_children(GTK_CONTAINER(menu));
    if (items != NULL) {
        data = g_list_nth_data(items, n);
        g_list_free(items);
    }
    return GTK_WIDGET(data);
}


/** \brief  Create popup menu
 *
 * Create popup menu from \a items.
 *
 * \param[in]   items   list of items for the new menu
 *
 * \return  GtkMenu
 */
GtkWidget *popup_menu_new(const popup_item_t *items)
{
    GSList *group = NULL;
    GtkWidget *item;
    GtkWidget *submenu;
    GtkWidget *menu = gtk_menu_new();

    while (items->type >= POPUP_ITEM_NORMAL) {
        switch (items->type) {
            case POPUP_ITEM_SEPARATOR:
                group = NULL;
                item = gtk_separator_menu_item_new();
                break;
            case POPUP_ITEM_NORMAL:
                group = NULL;
                item = gtk_menu_item_new_with_label(items->text);
                break;
            case POPUP_ITEM_CHECK:
                group = NULL;
                item = gtk_check_menu_item_new_with_label(items->text);
                break;
            case POPUP_ITEM_RADIO:
                item = gtk_radio_menu_item_new_with_label(group, items->text);
                break;
            case POPUP_ITEM_SUBMENU:
                group = NULL;
                submenu = popup_menu_new(items->submenu);
                item = gtk_menu_item_new_with_label(items->text);
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
                break;
            default:
                /* potjandikkie! */
                group = NULL;
                item = NULL;
                break;
        }

        /* custom handler or UI action trigger handler? */
        if (items->handler != NULL) {
            g_object_set_data(G_OBJECT(item),
                              "CustomHandler",
                              (gpointer)(items->handler));
            g_signal_connect(G_OBJECT(item),
                             "activate",
                             G_CALLBACK(on_custom_item_activate),
                             GINT_TO_POINTER(items->action));
        } else if (items->action > ACTION_NONE) {
            g_signal_connect(G_OBJECT(item),
                             "activate",
                             G_CALLBACK(on_action_item_activate),
                             GINT_TO_POINTER(items->action));
        }
        /* set accelerator, if any */
        if (items->action > ACTION_NONE) {
            vhk_gtk_set_menu_item_accel_label(item, items->action);
        }

        gtk_container_add(GTK_CONTAINER(menu), item);
        items++;
    }
    return menu;
}
