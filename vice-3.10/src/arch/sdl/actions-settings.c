/** \file   actions-settings.c
 * \brief   UI action implementations for settings management (SDL)
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

#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"

#include "actions-settings.h"


/** \brief  Restore settings to default action
 *
 * \param[in]   self    action map
 */
static void settings_default_action(ui_action_map_t *self)
{
    resources_set_defaults();
    ui_message("Default settings restored.");
    ui_action_finish(self->action);
}

/** \brief  Load settings from default file action
 *
 * \param[in]   self    action map
 */
static void settings_load_action(ui_action_map_t *self)
{
    if (resources_reset_and_load(NULL) < 0) {
        ui_error("Cannot load settings.");
    } else {
        ui_message("Settings loaded.");
    }
    ui_action_finish(self->action);
}

/** \brief  Save settings to default file action
 *
 * \param[in]   self    action map
 */
static void settings_save_action(ui_action_map_t *self)
{
    if (resources_save(NULL) != 0) {
        ui_error("Cannot save current settings.");
    } else {
        ui_message("Settings saved.");
    }
    ui_action_finish(self->action);
}


/** \brief  List of mappings for settings actions */
static const ui_action_map_t settings_actions[] = {
    {   .action  = ACTION_SETTINGS_DEFAULT,
        .handler = settings_default_action,
        .dialog  = true     /* shows message box */
    },
    {   .action  = ACTION_SETTINGS_LOAD,
        .handler = settings_load_action,
        .dialog  = true     /* shows message box */
    },
    {   .action  = ACTION_SETTINGS_LOAD_FROM,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_SETTINGS_LOAD_EXTRA,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_SETTINGS_SAVE,
        .handler = settings_save_action,
        .dialog  = true     /* shows message box */
    },
    {   .action  = ACTION_SETTINGS_SAVE_TO,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register settings actions */
void actions_settings_register(void)
{
    ui_actions_register(settings_actions);
}
