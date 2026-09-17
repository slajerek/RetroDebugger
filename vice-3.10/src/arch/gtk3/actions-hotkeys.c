/** \file   actions-hotkeys.c
 * \brief   UI action implementations for hotkey management
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

/* Resources manipulated in this file:
 *
 * $VICERES HotkeyFile all
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "basedialogs.h"
#include "hotkeys.h"
#include "resources.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uihotkeys.h"
#include "uihotkeysload.h"
#include "uihotkeyssave.h"

#include "actions-hotkeys.h"


/** \brief  Clear all hotkeys action
 *
 * \param[in]   self    action map
 */
static void hotkeys_clear_action(ui_action_map_t *self)
{
    ui_display_statustext("Clearing all hotkeys.", true);
    ui_hotkeys_remove_all();
}

/** \brief  Load default hotkeys action
 *
 * \param[in]   self    action map
 */
static void hotkeys_default_action(ui_action_map_t *self)
{
    ui_display_statustext("Loading default hotkeys.", true);
    ui_hotkeys_load_vice_default();
}

/** \brief  Reload current hotkeys file
 *
 * Either load the file in "HotkeyFile" or load the default hotkeys.
 *
 * \param[in]   self    action map
 */
static void hotkeys_load_action(ui_action_map_t *self)
{
    ui_display_statustext("Reloading current hotkeys.", true);
    ui_hotkeys_remove_all();
    ui_hotkeys_reload();
}

/** \brief  Pop up dialog to load hotkeys from a specific file
 *
 * \param[in]   self    action map
 */
static void hotkeys_load_from_action(ui_action_map_t *self)
{
    ui_hotkeys_load_dialog_show(NULL);
}

/** \brief  Save hotkeys to current hotkeys file
 *
 * If the default hotkeys are loaded, don't save anything.
 *
 * \param[in]   self    action map
 */
static void hotkeys_save_action(ui_action_map_t *self)
{
    if (ui_hotkeys_save()) {
        ui_display_statustext("Hotkeys saved succesfully.", true);
    } else {
        /* TODO: a little more info on why it failed perhaps? */
        ui_display_statustext("Failed to save hotkeys.", true);
    }
}

/** \brief  Pop up a dialog to save hotkeys to file
 *
 * \param[in]   self    action map
 */
static void hotkeys_save_to_action(ui_action_map_t *self)
{
    ui_hotkeys_save_dialog_show();
}


/** \brief  List of actions for hotkeys management */
static const ui_action_map_t hotkeys_actions[] = {
    {
        .action   = ACTION_HOTKEYS_CLEAR,
        .handler  = hotkeys_clear_action,
        .uithread = true
    },
    {
        .action   = ACTION_HOTKEYS_DEFAULT,
        .handler  = hotkeys_default_action,
        .uithread = true
    },
    {
        .action   = ACTION_HOTKEYS_LOAD,
        .handler  = hotkeys_load_action,
        .uithread = true
    },
    {
        .action   = ACTION_HOTKEYS_LOAD_FROM,
        .handler  = hotkeys_load_from_action,
        .blocks   = true,
        .dialog   = true,
        .uithread = true
    },
    {
        .action   = ACTION_HOTKEYS_SAVE,
        .handler  = hotkeys_save_action,
        .uithread = true
    },
    {
        .action   = ACTION_HOTKEYS_SAVE_TO,
        .handler  = hotkeys_save_to_action,
        .blocks   = true,
        .dialog   = true,
        .uithread = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register hotkeys-related actions */
void actions_hotkeys_register(void)
{
    ui_actions_register(hotkeys_actions);
}
