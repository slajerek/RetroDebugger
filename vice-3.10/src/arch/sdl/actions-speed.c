/** \file   actions-speed.c
 * \brief   UI action implementations for speed settings (SDL)
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

#include "machine.h"
#include "resources.h"
#include "types.h"
#include "ui.h"
#include "uiactions.h"
#include "menu_common.h"
#include "uimenu.h"
#include "vsync.h"
#include "vsyncapi.h"

#include "actions-speed.h"


/** \brief  Toggle warp mode action
 *
 * \param[in]   self    action map
 */
static void warp_mode_toggle_action(ui_action_map_t *self)
{
    vsync_set_warp_mode(!vsync_get_warp_mode());
}

/** \brief  Set Speed resource value
 *
 * Set Speed resource to set emulated CPU speed (positive values) or FPS target
 * (negative values).
 *
 * \param[in]   self    action map
 */
static void set_speed_resource(ui_action_map_t *self)
{
    resources_set_int("Speed", vice_ptr_to_int(self->data));
}

/** \brief  Update status of "Advance frame" based on pause state
 *
 * If pause isn't active the SDL UI disables the "Advance frame" item.
 * This differs from the Gtk3 UI where "Advance frame" can be activated and
 * simply pauses the emulation when not already paused.
 */
static void update_advance_frame_status(void)
{
    /* update advance frame menu item */
    ui_action_map_t *map = ui_action_map_get(ACTION_ADVANCE_FRAME);
    if (map != NULL) {
        ui_menu_entry_t *item = map->menu_item[0];
        if (item != NULL) {
            item->status = sdl_pause_state || (!sdl_menu_state)
                           ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
        }
    }
}

/** \brief  Update status of "Virtual keyboard" based on pause state */
static void update_virtual_keyboard_status(void)
{
    /* update virtual keyboard menu item */
    ui_action_map_t *map = ui_action_map_get(ACTION_VIRTUAL_KEYBOARD);
    if (map != NULL) {
        ui_menu_entry_t *item = map->menu_item[0];
        if (item != NULL) {
            item->status = sdl_pause_state ? MENU_STATUS_INACTIVE : MENU_STATUS_ACTIVE;
        }
    }
}

/** \brief  Pause toggle action
 *
 * \param[in]   self    action map
 */
static void pause_toggle_action(ui_action_map_t *self)
{
    /* madness: two differents pause states */
    if (sdl_menu_state) {
        /* in menu */
        sdl_pause_state ^= 1;
    } else {
        /* in emulation */
        ui_pause_toggle();
    }

    /* update other menu items (VSID doesn't have them) */
    if (machine_class != VICE_MACHINE_VSID) {
        update_advance_frame_status();
        update_virtual_keyboard_status();
    }
}

/** \brief  Advance frame action
 *
 * \param[in]   self    action map
 */
static void advance_frame_action(ui_action_map_t *self)
{
    if (sdl_menu_state) {
        /* in menu */
        if (sdl_pause_state) {
            sdl_pause_state = 0;
            vsyncarch_advance_frame();
        }
    } else {
        /* in emulation */
        if (ui_pause_active()) {
            vsyncarch_advance_frame();
        }
    }
}


/** \brief  List of mappings for speed/fps actions */
static const ui_action_map_t speed_actions[] = {
    /* Warp mode */
    {   .action  = ACTION_WARP_MODE_TOGGLE,
        .handler = warp_mode_toggle_action
    },
    /* Pause */
    {   .action  = ACTION_PAUSE_TOGGLE,
        .handler = pause_toggle_action
    },
    /* Advance frame */
    {   .action  = ACTION_ADVANCE_FRAME,
        .handler = advance_frame_action
    },

    /* CPU speed radio buttons */
    {   .action  = ACTION_SPEED_CPU_10,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(10)
    },
    {   .action  = ACTION_SPEED_CPU_25,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(25)
    },
    {   .action  = ACTION_SPEED_CPU_50,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(50)
    },
    {   .action  = ACTION_SPEED_CPU_100,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(100)
    },
    {   .action  = ACTION_SPEED_CPU_200,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(200)
    },
    /* Custom CPU speed dialog */
    {   .action  = ACTION_SPEED_CPU_CUSTOM,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },

    /* FPS targets */
    {   .action  = ACTION_SPEED_FPS_50,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(-50)
    },
    {   .action  = ACTION_SPEED_FPS_60,
        .handler = set_speed_resource,
        .data    = vice_int_to_ptr(-60)
    },
    {   .action  = ACTION_SPEED_FPS_CUSTOM,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register speed/fps actions */
void actions_speed_register(void)
{
    ui_actions_register(speed_actions);
}


/** \brief  Display helper for the UI for Warp Mode
 *
 * \param[in]   item    menu item (unused)
 *
 * \return  special checkmark string when warp enabled, NULL otherwise
 */
const char *warp_mode_toggle_display(ui_menu_entry_t *item)
{
    return vsync_get_warp_mode() ? MENU_CHECKMARK_CHECKED_STRING : NULL;
}


/** \brief  Display helper for the UI for pause state
 *
 * \param[in]   item    menu item (unused)
 *
 * \return  `sdl_menu_text_tick` if pause active, `NULL` otherwise
 */
const char *pause_toggle_display(ui_menu_entry_t *item)
{
    /* The old SDL UI code updated the `status` of other menu items in
     * the pause callback wrapper, but the UI actions do not work like that,
     * so we update them here, which has the same effect: this gets called
     * when `sdl_ui_display_item() is called on a "Pause" menu item.
     */
    if (machine_class != VICE_MACHINE_VSID) {
        update_advance_frame_status();
        update_virtual_keyboard_status();
    }

    /* called from menu, so `ui_pause_active()` doesn't return the correct state */
    return sdl_pause_state ? sdl_menu_text_tick : NULL;
}
