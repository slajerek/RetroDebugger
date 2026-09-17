/** \file   actions-machine.c
 * \brief   UI action implementations for machine-related dialogs and settings (SDL)
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
#include "monitor.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "vsync.h"

#include "actions-machine.h"


/** \brief  Trigger reset of the machine
 *
 * The reset mode is available in the \c data member of \a self.
 *
 * \param[in]   self    action map
 */
static void reset_action(ui_action_map_t *self)
{
    vsync_suspend_speed_eval();
    machine_trigger_reset(vice_ptr_to_int(self->data));
}

/** \brief  Open monitor action
 *
 * \param[in]   self    action map
 */
static void monitor_open_action(ui_action_map_t *self)
{
    /* Are we called from the menu? */
    if (sdl_menu_state) {
        monitor_startup(e_default_space);
    } else {
        /* The monitor was activated with a hotkey. */
        /* we must remember the pause state and unpause, else we can not
           enter the monitor when the emulation is paused */
        sdl_pause_state = ui_pause_active();
        if (sdl_pause_state) {
            sdl_ui_create_draw_buffer_backup();
            ui_pause_disable();
        }
        /* The trap is needed for the machine state to be properly imported. */
        monitor_startup_trap();
    }
}

/** \brief  Quit action
 *
 * Exit the emulator.
 *
 * \param[in]   self    action map
 */
static void quit_action(ui_action_map_t *self)
{
    ui_sdl_quit();
}

/** \brief  Toggle SCPU Speed switch action
 *
 * \param[in]   self    action map
 */
static void scpu_speed_switch_toggle_action(ui_action_map_t *self)
{
    int speed = 0;

    resources_get_int("SpeedSwitch", &speed);
    resources_set_int("SpeedSwitch", !speed);
}

/** \brief  Toggle SCPU JiffyDOS switch action
 *
 * \param[in]   self    action map
 */
static void scpu_jiffy_switch_toggle_action(ui_action_map_t *self)
{
    int jiffy = 0;

    resources_get_int("JiffySwitch", &jiffy);
    resources_set_int("JiffySwitch", !jiffy);
}


/** \brief  List of mappings for machine-related actions */
static const ui_action_map_t machine_actions[] = {
    {   .action  = ACTION_SMART_ATTACH,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_MACHINE_RESET_CPU,
        .handler = reset_action,
        .data    = vice_int_to_ptr(MACHINE_RESET_MODE_RESET_CPU)
    },
    {   .action  = ACTION_MACHINE_POWER_CYCLE,
        .handler = reset_action,
        .data    = vice_int_to_ptr(MACHINE_RESET_MODE_POWER_CYCLE)
    },
    {   .action  = ACTION_MONITOR_OPEN,
        .handler = monitor_open_action
    },
    {   .action  = ACTION_QUIT,
        .handler = quit_action
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of xscpu64-specific machine actions */
static const ui_action_map_t machine_actions_xscpu64[] = {
    {   .action  = ACTION_SCPU_JIFFY_SWITCH_TOGGLE,
        .handler = scpu_jiffy_switch_toggle_action
    },
    {   .action  = ACTION_SCPU_SPEED_SWITCH_TOGGLE,
        .handler = scpu_speed_switch_toggle_action
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register machine actions */
void actions_machine_register(void)
{
    ui_actions_register(machine_actions);
    if (machine_class == VICE_MACHINE_SCPU64) {
        ui_actions_register(machine_actions_xscpu64);
    }
}
