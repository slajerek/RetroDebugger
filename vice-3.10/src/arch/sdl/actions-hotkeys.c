/** \file   actions-hotkeys.c
 * \brief   UI action implementations for hotkeys management (SDL)
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

#include "vice.h"

#include "log.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uihotkeys.h"
#include "uimenu.h"

#include "actions-hotkeys.h"


/** \brief  Clear hotkeys action
 *
 * \param[in]   self    action map
 */
static void hotkeys_clear_action(ui_action_map_t *self)
{
    /* no dialog available to ask for confirmation, so just delete */
    ui_hotkeys_remove_all();
    ui_message("All hotkeys are cleared.");
}

/** \brief  Restore default hotkeys action
 *
 * \param[in]   self    action map
 */
static void hotkeys_default_action(ui_action_map_t *self)
{
    /* load default hotkeys and clear "HotkeyFile" resource */
    ui_hotkeys_remove_all();
    ui_hotkeys_load_vice_default();
}

/** \brief  Load hotkeys action
 *
 * \param[in]   self    action map
 */
static void hotkeys_load_action(ui_action_map_t *self)
{
    ui_hotkeys_remove_all();
    ui_hotkeys_reload();
}

/** \brief  Load hotkeys from custom file action
 *
 * \param[in]   self    action map
 */
static void hotkeys_load_from_action(ui_action_map_t *self)
{
    sdl_ui_menu_item_activate_by_action(self->action);
}

/** \brief  Save hotkeys action
 *
 * \param[in]   self    action map
 */
static void hotkeys_save_action(ui_action_map_t *self)
{
    if (ui_hotkeys_save()) {
        ui_message("Hotkeys saved succesfully.");
    } else{
        ui_error("Failed to save hotkeys.");
    }
}

/** \brief  Save hotkeys to custom file action
 *
 * \param[in]   self    action map
 */
static void hotkeys_save_to_action(ui_action_map_t *self)
{
    sdl_ui_menu_item_activate_by_action(self->action);
}


/** \brief  List of mappings for hotkeys actions */
static const ui_action_map_t hotkeys_actions[] = {
    {   .action  = ACTION_HOTKEYS_CLEAR,
        .handler = hotkeys_clear_action
    },
    {   .action  = ACTION_HOTKEYS_DEFAULT,
        .handler = hotkeys_default_action
    },
    {   .action  = ACTION_HOTKEYS_LOAD,
        .handler = hotkeys_load_action
    },
    {   .action  = ACTION_HOTKEYS_LOAD_FROM,
        .handler = hotkeys_load_from_action,
        .dialog  = true
    },
    {   .action  = ACTION_HOTKEYS_SAVE,
        .handler = hotkeys_save_action
    },
    {   .action  = ACTION_HOTKEYS_SAVE_TO,
        .handler = hotkeys_save_to_action,
        .dialog  = true
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register hotkeys actions */
void actions_hotkeys_register(void)
{
    ui_actions_register(hotkeys_actions);
}
