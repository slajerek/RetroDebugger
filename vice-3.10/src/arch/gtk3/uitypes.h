/** \file   uitypes.h
 * \brief   Gtk3 UI menu item types
 *
 * These types are shared between uimenu.{c,h} and uimachinemenu.{c,h}.
 *
 * Due to someone splitting uimenu.c into uimenu.c and uimachinemenu.c we now
 * have to deal with a lot of cross-dependencies between those files. So fixing
 * that is definitely a TODO.
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
 *
 */

#ifndef VICE_UITYPES_H
#define VICE_UITYPES_H

#include "vice.h"

#include <gtk/gtk.h>
#include <stdbool.h>

/** \brief  Menu item types
 *
 * The submenu types needs special handling, no more callbacks to create the
 * menu so we won't have to deal with the `ui_update_checkmarks` crap of Gtk2.
 *
 * The 'layer' between VICE and Gtk3 should be as thin as possible, so no
 * UI_CREATE_TOGGLE_BUTTON() stuff.
 */
typedef enum ui_menu_item_type_e {
    UI_MENU_TYPE_GUARD = -1,        /**< list terminator */
    UI_MENU_TYPE_ITEM_ACTION,       /**< standard list item: activate dialog */
    UI_MENU_TYPE_ITEM_CHECK,        /**< menu item with checkmark */
    UI_MENU_TYPE_ITEM_RADIO_INT,    /**< menu item with radio button, int value */
    UI_MENU_TYPE_ITEM_RADIO_STR,    /**< menu item with radio button, string value */
    UI_MENU_TYPE_SUBMENU,           /**< submenu */
    UI_MENU_TYPE_SEPARATOR          /**< items separator */
} ui_menu_item_type_t;



/** \brief  Menu item object
 *
 * Contains information on a menu item
 */
typedef struct ui_menu_item_s {
    /** \brief  Menu item label
     *
     * The label displayed in the menu. Do not add any accelerator description
     * here, that is set and updated dynamically.
     */
    char *label;

    /** \brief  Menu item type
     *
     * \see ui_menu_item_type_t
     */
    ui_menu_item_type_t type;

    /** \brief  UI action ID as defined in uiactions.h
     *
     * The action ID is used for the hotkeys to be able to set and alter the
     * hotkey assigned to the menu item.
     *
     * \note    Must be unique or 0.
     * \note    Do NOT use the same action ID for similar actions in different
     *          emulators, thanks to the run-time checking of the machine this
     *          will lead to the hotkeys code updating the first matching action
     *          it finds and ignoring the other actions with the same name.
     */
    int action;

    /** \brief  Submenu
     *
     * In case the item is of type `UI_MENU_TYPE_SUBMENU`, this member points
     * to the submenu to add.
     */
    const struct ui_menu_item_s *submenu;

    /** \brief  Hold VICE mainlock
     *
     * Determines whether the callback should be called while holding the VICE
     * mainlock.
     */
    bool unlocked;

    /** \brief  Use the 'activate' signal instead of the 'toggled' signal
     *
     * If a menu item is radio button, use the 'activate' signal instead of the
     * 'toggled' signal so radio buttons that are already selected (e.g
     * "Custom FPS" still activate their dialog when selected again.
     * Has no meaning for other types of menu items.
     */
    bool activate;

} ui_menu_item_t;


/** \brief  Terminator of a menu items list
 */
#define UI_MENU_TERMINATOR { .label = NULL, .type = UI_MENU_TYPE_GUARD }


/** \brief  Menu items separator
 */
#define UI_MENU_SEPARATOR { .label = "---", .type = UI_MENU_TYPE_SEPARATOR }


/** \brief  Platform-dependent accelerator key defines
 */
#ifdef MACOS_COMPILE
  /* Mac Command key (Windows key on PC keyboards) */
  #define VICE_MOD_MASK GDK_META_MASK
#else
  /* Alt key (Option key on Mac keyboards) */
  #define VICE_MOD_MASK GDK_MOD1_MASK
#endif


/** \brief  Menu item runtime data
 *
 * Used for easier and faster access to runtime menu items and their associated
 * data and handlers.
 */
typedef struct ui_menu_item_ref_s {
    /** \brief  Menu item initialization data */
    const ui_menu_item_t *decl;

    /** \brief  Runtime menu item */
    GtkWidget *item[2];

    /** \brief  ID of the 'activate' signal handler of a menu item
     *
     * This ID is used to temporarily block the activate event of a check or
     * radio button when updating their state, to avoid triggering cascading
     * events.
     */
    gulong handler_id[2];

    /** \brief  Gdk keysym of the accelerator
     *
     * \see /usr/include/gtk-3.0/gdk/gdkkeysyms.h
     */
    guint keysym;

    /** \brief  Gdk modifier mask */
    GdkModifierType modifier;
} ui_menu_item_ref_t;

#endif
