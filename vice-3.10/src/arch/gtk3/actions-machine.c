/** \file   actions-machine.c
 * \brief   UI action implementations for machine-related dialogs and settings (Gtk3)
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

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "archdep.h"
#include "basedialogs.h"
#include "machine.h"
#include "mainlock.h"
#include "monitor.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uistatusbar.h"
#include "vsync.h"

#include "actions-machine.h"


/** \brief  Callback for the confirm-on-exit dialog
 *
 * Exit VICE if \a result is TRUE.
 *
 * \param[in]   dialog  dialog reference (unused)
 * \param[in]   result  dialog result
 */
static void confirm_exit_callback(GtkDialog *dialog, gboolean result)
{
    if (result) {
        mainlock_release();
        archdep_vice_exit(0);
        mainlock_obtain();
    }
    /* mark action finished in case the user selected "No" */
    ui_action_finish(ACTION_QUIT);
}


/** \brief  Quit emulator action
 *
 * Quit the emulator, possibly popping up a confirmation dialog before actually
 * closing the emulator.
 *
 * \param[in]   self    action map
 */
static void quit_action(ui_action_map_t *self)
{
    int confirm = FALSE;

    resources_get_int("ConfirmOnExit", &confirm);
    if (!confirm) {
        ui_action_finish(ACTION_QUIT);
        archdep_vice_exit(0);
        return;
    }

    vice_gtk3_message_confirm(NULL, /* current window as parent */
                             confirm_exit_callback,
                             "Exit VICE",
                             "Do you really wish to exit VICE?");
}

/** \brief  Open the monitor action
 *
 * \param[in]   self    action map
 */
static void monitor_open_action(ui_action_map_t *self)
{
    int server = 0;

    /* don't spawn the monitor if the monitor server is running */
    resources_get_int("MonitorServer", &server);
    if (server == 0) {
        vsync_suspend_speed_eval();
#ifdef HAVE_MOUSE
        /* FIXME: restore mouse in case it was grabbed */
        /* ui_restore_mouse(); */
#endif
        if (ui_pause_active()) {
            ui_pause_enter_monitor();
        } else {
            monitor_startup_trap();
        }
    }
}

/** \brief  Trigger reset/power-cycle of the machine
 *
 * The \c data member of \a self contains the reset mode.
 *
 * \param[in]   self    action map
 */
static void machine_reset_action(ui_action_map_t *self)
{
    machine_trigger_reset(vice_ptr_to_int(self->data));
    ui_pause_disable();
}

/** \brief  Toggle PET userport diagnostic pin
 *
 * \param[in]   self    action map
 */
static void diagnostic_pin_toggle_action(ui_action_map_t *self)
{
    int active = 0;

    resources_get_int("DiagPin", &active);
    resources_set_int("DiagPin", !active);
}

/** \brief  Toggle SuperCPU JiffyDOS switch
 *
 * \param[in]   self    action map
 */
static void scpu_jiffy_switch_toggle_action(ui_action_map_t *self)
{
    int jiffy = 0;

    resources_get_int("JiffySwitch", &jiffy);
    jiffy = !jiffy;
    resources_set_int("JiffySwitch", jiffy);
    /* update status bar LED */
    supercpu_jiffy_led_set_active(PRIMARY_WINDOW, jiffy ? TRUE : FALSE);
}

/** \brief  Toggle SuperCPU Speed switch
 *
 * \param[in]   self    action map
 */
static void scpu_speed_switch_toggle_action(ui_action_map_t *self)
{
    int speed = 0;

    resources_get_int("SpeedSwitch", &speed);
    speed = !speed;
    resources_set_int("SpeedSwitch", speed);
    /* update status bar LED */
    supercpu_turbo_led_set_active(PRIMARY_WINDOW, speed ? TRUE : FALSE);
}


/** \brief  List of machine-related actions */
static const ui_action_map_t machine_actions[] = {
    {   .action  = ACTION_QUIT,
        .handler = quit_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action   = ACTION_MONITOR_OPEN,
        .handler  = monitor_open_action,
        .uithread = true
    },
    {   .action  = ACTION_MACHINE_RESET_CPU,
        .handler = machine_reset_action,
        .data    = vice_int_to_ptr(MACHINE_RESET_MODE_RESET_CPU)
    },
    {   .action  = ACTION_MACHINE_POWER_CYCLE,
        .handler = machine_reset_action,
        .data    = vice_int_to_ptr(MACHINE_RESET_MODE_POWER_CYCLE)
    },
    {   .action  = ACTION_DIAGNOSTIC_PIN_TOGGLE,
        .handler = diagnostic_pin_toggle_action,
        /* no need for UI thread, the status bar code will update the LED when
         * it runs */
        .uithread = false
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of additional actions for xscpu64 */
static const ui_action_map_t machine_actions_xscpu64[] = {
    {   .action   = ACTION_SCPU_JIFFY_SWITCH_TOGGLE,
        .handler  = scpu_jiffy_switch_toggle_action,
        /* LED is not updated in the status bar sync code, so we need to guard
         * this: */
        .uithread = true
    },
    {   .action   = ACTION_SCPU_SPEED_SWITCH_TOGGLE,
        .handler  = scpu_speed_switch_toggle_action,
        /* LED is not updated in the status bar sync code, so we need to guard
         * this: */
        .uithread = true
    },
    UI_ACTION_MAP_TERMINATOR
};


void actions_machine_register(void)
{
    ui_actions_register(machine_actions);
    if (machine_class == VICE_MACHINE_SCPU64) {
        ui_actions_register(machine_actions_xscpu64);
    }
}
