/** \file   popupmenu.h
 * \brief   Gtk3 popup menu helpers - header
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

#ifndef VICE_POPUPMENU_H
#define VICE_POPUPMENU_H

#include <gtk/gtk.h>
#include <stdbool.h>

enum {
    POPUP_ITEM_SENTINEL = -1,   /**< end of items list */
    POPUP_ITEM_NORMAL,          /**< normal menu item */
    POPUP_ITEM_CHECK,           /**< check button item */
    POPUP_ITEM_RADIO,           /**< radio menu item */
    POPUP_ITEM_SUBMENU,         /**< submenu */
    POPUP_ITEM_SEPARATOR        /**< separator */
};

/** \brief  Popup menu item object
 */
typedef struct popup_item_s {
    const char *text;       /**< item text */
    int type;               /**< item type */
    int action;             /**< UI action ID */
    void (*handler)(int);   /**< custom handler (optional), gets passed the
                                 UI action ID but the action isn't triggered */
    const struct popup_item_s *submenu; /**< submenu items in case of type
                                             #POPUP_ITEM_SUBMENU */
} popup_item_t;

#define POPUP_MENU_TERMINATOR { .type = POPUP_MENU_SENTINEL }


GtkWidget * popup_menu_item_action_new(const char *text, int action);
void        popup_menu_add_separator(GtkWidget *menu);

GtkWidget * popup_menu_new(const popup_item_t *items);
GtkWidget * popup_menu_nth_item(GtkWidget *menu, guint n);

#endif
