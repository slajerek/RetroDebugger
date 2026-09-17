/** \file   actions-drive.c
 * \brief   UI action implementations for drive-related dialogs and settings (SDL)
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

#include "attach.h"
#include "drive.h"
#include "fliplist.h"
#include "interrupt.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"
#include "vsync.h"

#include "actions-drive.h"

/** \brief  Unpack void pointer into `int unit` and `int drive` assignments
 *
 * \data [in]   P   pointer value encoded with UNIT_DRIVE_TO_PTR()
 */
#define UNPACK_U_D(P) \
    int unit  = UNIT_FROM_PTR(P); \
    int drive = DRIVE_FROM_PTR(P);


/** \brief  Detach disk from drive action
 *
 * \param[in]   self    action map
 *
 * XXX: Can probably be merged with the Gtk3 version
 */
static void drive_detach_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    file_system_detach_disk(unit, drive);
}

/** \brief Detach all disks in all drives action
 *
 * \param[in]   self    action map
 *
 * XXX: Can probably be merged with the Gtk3 version
 */
static void drive_detach_all_action(ui_action_map_t *self)
{
    int unit;
    for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {
        file_system_detach_disk(unit, 0);
        file_system_detach_disk(unit, 1);
    }
}

/* ATN: drive (cpu) functions are indexed-based rather than unit number-based,
 * so  0-3 instead of 8-11 */

/** \brief  Drive reset action
 *
 * \param[in]   self    action map
 */
static void drive_reset_action(ui_action_map_t *self)
{
    vsync_suspend_speed_eval();
    drive_cpu_trigger_reset(vice_ptr_to_int(self->data) - DRIVE_UNIT_MIN);
}

/** \brief  Drive reset into configuration mode action
 *
 * \pram[in]   self    action map
 */
static void drive_reset_config_action(ui_action_map_t *self)
{
    vsync_suspend_speed_eval();
    drive_cpu_trigger_reset_button(vice_ptr_to_int(self->data) - DRIVE_UNIT_MIN,
                                   DRIVE_BUTTON_WRITE_PROTECT);
}

/** \brief  Drive reset into installation mode action
 *
 * \param[in]   self    action map
 */
static void drive_reset_install_action(ui_action_map_t *self)
{
    vsync_suspend_speed_eval();
    drive_cpu_trigger_reset_button(vice_ptr_to_int(self->data) - DRIVE_UNIT_MIN,
                                   DRIVE_BUTTON_SWAP_8|DRIVE_BUTTON_SWAP_9);
}

/** \brief  Add current image to fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_add_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    (void)drive;
    fliplist_add_image(unit);
}

/** \brief  Remove current image from fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_remove_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    (void)drive;
    fliplist_remove(unit, NULL);
}

/** \brief  Add next image in fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_next_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    (void)drive;
    fliplist_attach_head(unit, 1);
}

/** \brief  Attach previous image in fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_previous_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    (void)drive;
    fliplist_attach_head(unit, 0);
}


/** \brief  List of mappings for machine-related actions */
static const ui_action_map_t drive_actions[] = {
    /* Attach disk actions. The unit and drive number are obtained in the menu
     * item callback from its `data` member. */
    {   .action  = ACTION_DRIVE_ATTACH_8_0,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_8_1,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_9_0,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_9_1,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_10_0,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_10_1,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_11_0,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_11_1,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },

    /* create (and optionally attach) disk image, unit selection is done in
     * the dialog */
    {   .action  = ACTION_DRIVE_CREATE,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },

    /* detach disk from drive */
    {   .action  = ACTION_DRIVE_DETACH_8_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0),
    },
    {   .action  = ACTION_DRIVE_DETACH_8_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 1),
    },
    {   .action  = ACTION_DRIVE_DETACH_9_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0),
    },
    {   .action  = ACTION_DRIVE_DETACH_9_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 1),
    },
    {   .action  = ACTION_DRIVE_DETACH_10_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0),
    },
    {   .action  = ACTION_DRIVE_DETACH_10_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 1),
    },
    {   .action  = ACTION_DRIVE_DETACH_11_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0),
    },
    {   .action  = ACTION_DRIVE_DETACH_11_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 1),
    },
    {   .action  = ACTION_DRIVE_DETACH_ALL,
        .handler = drive_detach_all_action,
    },

    /* normal reset */
    {   .action  = ACTION_RESET_DRIVE_8,
        .handler = drive_reset_action,
        .data    = vice_int_to_ptr(8)
    },
    {   .action  = ACTION_RESET_DRIVE_9,
        .handler = drive_reset_action,
        .data    = vice_int_to_ptr(9)
    },
    {   .action  = ACTION_RESET_DRIVE_10,
        .handler = drive_reset_action,
        .data    = vice_int_to_ptr(10)
    },
    {   .action  = ACTION_RESET_DRIVE_11,
        .handler = drive_reset_action,
        .data    = vice_int_to_ptr(11)
    },

    /* reset in configuration mode */
    {   .action  = ACTION_RESET_DRIVE_8_CONFIG,
        .handler = drive_reset_config_action,
        .data    = vice_int_to_ptr(8)
    },
    {   .action  = ACTION_RESET_DRIVE_9_CONFIG,
        .handler = drive_reset_config_action,
        .data    = vice_int_to_ptr(9)
    },
    {   .action  = ACTION_RESET_DRIVE_10_CONFIG,
        .handler = drive_reset_config_action,
        .data    = vice_int_to_ptr(10)
    },
    {   .action  = ACTION_RESET_DRIVE_11_CONFIG,
        .handler = drive_reset_config_action,
        .data    = vice_int_to_ptr(11)
    },

    /* reset in installation mode */
    {   .action  = ACTION_RESET_DRIVE_8_INSTALL,
        .handler = drive_reset_install_action,
        .data    = vice_int_to_ptr(8)
    },
    {   .action  = ACTION_RESET_DRIVE_9_INSTALL,
        .handler = drive_reset_install_action,
        .data    = vice_int_to_ptr(9)
    },
    {   .action  = ACTION_RESET_DRIVE_10_INSTALL,
        .handler = drive_reset_install_action,
        .data    = vice_int_to_ptr(10)
    },
    {   .action  = ACTION_RESET_DRIVE_11_INSTALL,
        .handler = drive_reset_install_action,
        .data    = vice_int_to_ptr(11)
    },

    /* fliplist (unit #8 only for now) */
    {   .action  = ACTION_FLIPLIST_ADD_8_0,
        .handler = fliplist_add_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_REMOVE_8_0,
        .handler = fliplist_remove_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_NEXT_8_0,
        .handler = fliplist_next_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_PREVIOUS_8_0,
        .handler = fliplist_previous_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_LOAD_8_0,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_FLIPLIST_SAVE_8_0,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    /* SDL UI doesn't provide a "clear" option, so we won't provide one here */

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register drive actions */
void actions_drive_register(void)
{
    ui_actions_register(drive_actions);
}
