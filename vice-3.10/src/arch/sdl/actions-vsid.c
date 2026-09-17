/** \file   actions-vsid.c
 * \brief   UI action implementations for VSID (SDL)
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
#include "menu_common.h"
#include "psid.h"
#include "resources.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"
#include "vsidui_sdl.h"

#include "actions-vsid.h"


/** \brief  Play subtune helper
 *
 * Initialize driver, start playback of \a tune.
 *
 * \param[in]   tune    subtune number
 */
static void play_subtune(int tune)
{
    psid_init_driver();
    machine_play_psid(tune);
    sdl_vsid_current_tune = tune;
    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
}

/** \brief  Play subtune action
 *
 * \param[in]   self    action map
 */
static void psid_subtune_action(ui_action_map_t *self)
{
    play_subtune(vice_ptr_to_int(self->data));
}

/** \brief  Play default subtune action
 *
 * \param[in]   self    action map
 */
static void psid_subtune_default_action(ui_action_map_t *self)
{
    play_subtune(sdl_vsid_default_tune);
}

/** \brief  Play next subtune action
 *
 * \param[in]   self    action map
 */
static void psid_subtune_next_action(ui_action_map_t *self)
{
    sdl_vsid_current_tune++;
    if (sdl_vsid_current_tune > sdl_vsid_tunes) {
        sdl_vsid_current_tune = 1;
    }
    play_subtune(sdl_vsid_current_tune);
}

/** \brief  Play next subtune action
 *
 * \param[in]   self    action map
 */
static void psid_subtune_previous_action(ui_action_map_t *self)
{
    sdl_vsid_current_tune--;
    if (sdl_vsid_current_tune < 1) {
        sdl_vsid_current_tune = sdl_vsid_tunes;
    }
    play_subtune(sdl_vsid_current_tune);
}


/** \brief  List of mappings for VSID-specific actions
 */
static const ui_action_map_t vsid_actions[] = {
    /* {{{ Subtunes 1-30 */
    {   .action  = ACTION_PSID_SUBTUNE_1,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(1)
    },
    {   .action  = ACTION_PSID_SUBTUNE_2,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(2)
    },
    {   .action  = ACTION_PSID_SUBTUNE_3,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(3)
    },
    {   .action  = ACTION_PSID_SUBTUNE_4,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(4)
    },
    {   .action  = ACTION_PSID_SUBTUNE_5,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(5)
    },
    {   .action  = ACTION_PSID_SUBTUNE_6,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(6)
    },
    {   .action  = ACTION_PSID_SUBTUNE_7,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(7)
    },
    {   .action  = ACTION_PSID_SUBTUNE_8,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(8)
    },
    {   .action  = ACTION_PSID_SUBTUNE_9,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(9)
    },
    {   .action  = ACTION_PSID_SUBTUNE_10,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(10)
    },
    {   .action  = ACTION_PSID_SUBTUNE_11,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(11)
    },
    {   .action  = ACTION_PSID_SUBTUNE_12,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(12)
    },
    {   .action  = ACTION_PSID_SUBTUNE_13,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(13)
    },
    {   .action  = ACTION_PSID_SUBTUNE_14,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(14)
    },
    {   .action  = ACTION_PSID_SUBTUNE_15,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(15)
    },
    {   .action  = ACTION_PSID_SUBTUNE_16,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(16)
    },
    {   .action  = ACTION_PSID_SUBTUNE_17,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(17)
    },
    {   .action  = ACTION_PSID_SUBTUNE_18,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(18)
    },
    {   .action  = ACTION_PSID_SUBTUNE_19,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(19)
    },
    {   .action  = ACTION_PSID_SUBTUNE_20,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(20)
    },
    {   .action  = ACTION_PSID_SUBTUNE_21,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(21)
    },
    {   .action  = ACTION_PSID_SUBTUNE_22,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(22)
    },
    {   .action  = ACTION_PSID_SUBTUNE_23,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(23)
    },
    {   .action  = ACTION_PSID_SUBTUNE_24,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(24)
    },
    {   .action  = ACTION_PSID_SUBTUNE_25,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(25)
    },
    {   .action  = ACTION_PSID_SUBTUNE_26,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(26)
    },
    {   .action  = ACTION_PSID_SUBTUNE_27,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(27)
    },
    {   .action  = ACTION_PSID_SUBTUNE_28,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(28)
    },
    {   .action  = ACTION_PSID_SUBTUNE_29,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(29)
    },
    {   .action  = ACTION_PSID_SUBTUNE_30,
        .handler = psid_subtune_action,
        .data    = vice_int_to_ptr(30)
    },
    /* }}} */

    {   .action  = ACTION_PSID_LOAD,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_PSID_SUBTUNE_NEXT,
        .handler = psid_subtune_next_action
    },
    {   .action  = ACTION_PSID_SUBTUNE_PREVIOUS,
        .handler = psid_subtune_previous_action
    },
    {   .action  = ACTION_PSID_SUBTUNE_DEFAULT,
        .handler = psid_subtune_default_action
    },
    {   .action  = ACTION_PSID_OVERRIDE_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"PSIDKeepEnv"
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register VSID-specific actions */
void actions_vsid_register(void)
{
    ui_actions_register(vsid_actions);
}


/** \brief  Display helper for the subtune radio items
 *
 * \param[in]   item    menu item
 *
 * \return  string to display
 */
const char *psid_subtune_display(ui_menu_entry_t *item)
{
    int tune = vice_ptr_to_int(item->data);

    if (tune == sdl_vsid_current_tune) {
        return sdl_menu_text_tick;
    } else {
        if (tune > sdl_vsid_tunes) {
            return MENU_NOT_AVAILABLE_STRING;
        }
    }
    return NULL;
}


/** \brief  Determine if the radion list \a item should be displayed checked
 *
 * \param[in]   item    menu item
 *
 * \return  `true` if the tune number in the item matches the current tune
 */
bool psid_subtune_check(ui_menu_entry_t *item)
{
    int tune = vice_ptr_to_int(item->data);

    if (tune > sdl_vsid_tunes) {
        /* exit */
        return true;
    }

    return (bool)(tune == sdl_vsid_current_tune);
}
