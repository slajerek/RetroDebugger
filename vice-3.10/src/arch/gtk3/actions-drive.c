/** \file   actions-drive.c
 * \brief   UI action implementations for drive-related dialogs and settings
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

/* Resources altered by this file:
 *
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "attach.h"
#include "debug_gtk3.h"
#include "drive.h"
#include "fliplist.h"
#include "types.h"
#include "ui.h"
#include "uiapi.h"
#include "uiactions.h"
#include "uidiskattach.h"
#include "uidiskcreate.h"
#include "uifliplist.h"
#include "uismartattach.h"

#include "actions-drive.h"


/** \brief  Size of buffer used for status bar messages */
#define MSGBUF_SIZE 1024

/** \brief  Unpack P into assignments to `int unit` and `int drive`
 *
 * \param[in]   P   pointer value encoded with UNIT_DRIVE_TO_PTR()
 */
#define UNPACK_U_D(P) \
    int unit = UNIT_FROM_PTR(P); \
    int drive = DRIVE_FROM_PTR(P);


/** \brief  Pop up smart attach dialog
 *
 * \param[in]   self    action map
 */
static void smart_attach_action(ui_action_map_t *self)
{
    ui_smart_attach_dialog_show();
}

/** \brief  Attach drive action for (unit, drive)
 *
 * \param[in]   self    action map
 */
static void drive_attach_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    debug_gtk3("unit = %d, drive = %d", unit, drive);
    ui_disk_attach_dialog_show(unit, drive);
}

/** \brief  Pop up dialog to create and attach a disk image
 *
 * \param[in]   self    action map
 */
static void drive_create_action(ui_action_map_t *self)
{
    ui_disk_create_dialog_show(8);
}

/** \brief  Detach disk image from (unit, drive)
 *
 * \param[in]   self    action map
 */
static void drive_detach_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    debug_gtk3("unit = %d, drive = %d", unit, drive);
    file_system_detach_disk(unit, drive);
}

/** \brief  Detach all disk images from all units and drives
 *
 * \param[in]   self    action map
 */
static void drive_detach_all_action(ui_action_map_t *self)
{
    file_system_detach_disk_all();
}

/** \brief  Drive reset action
 *
 * \param[in]   self    action map
 */
static void reset_drive_action(ui_action_map_t *self)
{
    int unit = vice_ptr_to_int(self->data) - DRIVE_UNIT_MIN;
    drive_cpu_trigger_reset(unit);
}

/** \brief  Add current image to fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_add_action(ui_action_map_t *self)
{
    char buffer[MSGBUF_SIZE];
    UNPACK_U_D(self->data);

    if (fliplist_add_image(unit)) {
        g_snprintf(buffer, sizeof buffer,
                   "Fliplist: added image to unit %d, drive %d: %s.",
                   unit, drive, fliplist_get_head(unit));
        ui_display_statustext(buffer, true);
    } else {
        /* Display proper error message once we have a decent
         * get_image_filename(unit) function which returns NULL on non-attached
         * images.
         */
        g_snprintf(buffer, sizeof buffer,
                   "Fliplist: failed to add image to unit %d, drive %d.",
                   unit, drive);
        ui_display_statustext(buffer, true);
    }
}

/** \brief  Remove current image from fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_remove_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    const char *image = fliplist_get_head(unit);

    if (image != NULL) {
        char buffer[MSGBUF_SIZE];

        fliplist_remove(unit, NULL);
        g_snprintf(buffer, sizeof buffer,
                   "Fliplist: removed image from unit %d, drive %d: %s.",
                   unit, drive, image);
        ui_display_statustext(buffer, true);
    } else {
        ui_display_statustext("Fliplist: nothing to remove.", true);
    }
}

/** \brief  Attach next image in fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_next_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    char buffer[MSGBUF_SIZE];

    if (fliplist_attach_head(unit, 1)) {

        g_snprintf(buffer, sizeof buffer,
                   "Fliplist: attached next image to unit %d, drive %d: %s.",
                   unit, drive, fliplist_get_head(unit));
        ui_display_statustext(buffer, true);
    } else {
        g_snprintf(buffer, sizeof buffer,
                   "Fliplist: failed to attach next image to unit %d, drive %d.",
                   unit, drive);
        ui_display_statustext(buffer, true);
    }
}

/** \brief  Attach previous image in fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_previous_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    char buffer[MSGBUF_SIZE];

    if (fliplist_attach_head(unit, 0)) {

        g_snprintf(buffer, sizeof buffer,
                   "Fliplist: attached previous image to unit %d, drive %d: %s.",
                   unit, drive, fliplist_get_head(unit));
        ui_display_statustext(buffer, true);
    } else {
        g_snprintf(buffer, sizeof buffer,
                  "Fliplist: failed to attach previous image to unit %d, drive %d.",
                  unit, drive);
        ui_display_statustext(buffer, true);
    }
}

/** \brief  Clear fliplist action
 *
 * \param[in]   self    action map
 */
static void fliplist_clear_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    char buffer[MSGBUF_SIZE];

    fliplist_clear_list(unit);
    g_snprintf(buffer, sizeof buffer,
              "Fliplist: Cleared for unit %d, drive %d.",
              unit, drive);
    ui_display_statustext(buffer, true);
}

/** \brief  Load fliplist action
 *
 * \param[in]   self    action map
 *
 * \note    The drive number is ignored until the fliplist API supports
 *          dual-drive devices.
 */
static void fliplist_load_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    (void)drive;
    ui_fliplist_load_dialog_show(unit);
}

/** \brief  Save fliplist action
 *
 * \param[in]   self    action map
 *
 * \note    The drive number is ignored until the fliplist API supports
 *          dual-drive devices.
 */
static void fliplist_save_action(ui_action_map_t *self)
{
    UNPACK_U_D(self->data);
    (void)drive;
    ui_fliplist_save_dialog_show(unit);
}


/** \brief  List of drive-related actions */
static const ui_action_map_t drive_actions[] = {
    /* Smart attach, technically not just disk-related, but let's put it here */
    {   .action  = ACTION_SMART_ATTACH,
        .handler = smart_attach_action,
        .blocks  = true,
        .dialog  = true
    },

    /* Attach disk actions */
    {   .action  = ACTION_DRIVE_ATTACH_8_0,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_8_1,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 1),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_9_0,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_9_1,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 1),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_10_0,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_10_1,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 1),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_11_0,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_DRIVE_ATTACH_11_1,
        .handler = drive_attach_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 1),
        .blocks  = true,
        .dialog  = true
    },

    /* Create and attach new image */
    {   .action  = ACTION_DRIVE_CREATE,
        .handler = drive_create_action,
        .blocks  = true,
        .dialog  = true
    },

    /* Detach disk actions */
    {   .action  = ACTION_DRIVE_DETACH_8_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action = ACTION_DRIVE_DETACH_8_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 1)
    },
    {   .action  = ACTION_DRIVE_DETACH_9_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0)
    },
    {   .action  = ACTION_DRIVE_DETACH_9_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 1)
    },
    {   .action  = ACTION_DRIVE_DETACH_10_0,
        .handler    = drive_detach_action,
        .data   = UNIT_DRIVE_TO_PTR(10, 0)
    },
    {   .action  = ACTION_DRIVE_DETACH_10_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 1)
    },
    {   .action  = ACTION_DRIVE_DETACH_11_0,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0)
    },
    {   .action  = ACTION_DRIVE_DETACH_11_1,
        .handler = drive_detach_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 1)
    },
    {   .action  = ACTION_DRIVE_DETACH_ALL,
        .handler = drive_detach_all_action
    },

    /* Drive reset actions */
    {   .action  = ACTION_RESET_DRIVE_8,
        .handler = reset_drive_action,
        .data   = vice_int_to_ptr(8),
    },
    {   .action  = ACTION_RESET_DRIVE_9,
        .handler = reset_drive_action,
        .data    = vice_int_to_ptr(9),
    },
    {   .action  = ACTION_RESET_DRIVE_10,
        .handler = reset_drive_action,
        .data    = vice_int_to_ptr(10),
    },
    {   .action  = ACTION_RESET_DRIVE_11,
        .handler = reset_drive_action,
        .data    = vice_int_to_ptr(11),
    },

    /* Fliplist actions
     *
     * Although the non-dialog actions display a message on the status bar,
     * they do not require to be run on the UI thread: the function
     * `ui_display_statustext()` can be called from any thread since the status
     * bar code has its own locking mechanism.
     */
    {   .action  = ACTION_FLIPLIST_ADD_8_0,
        .handler = fliplist_add_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_ADD_9_0,
        .handler = fliplist_add_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0)
    },
    {   .action  = ACTION_FLIPLIST_ADD_10_0,
        .handler = fliplist_add_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0)
    },
    {   .action  = ACTION_FLIPLIST_ADD_11_0,
        .handler = fliplist_add_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0)
    },
    {   .action  = ACTION_FLIPLIST_REMOVE_8_0,
        .handler = fliplist_remove_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_REMOVE_9_0,
        .handler = fliplist_remove_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0)
    },
    {   .action  = ACTION_FLIPLIST_REMOVE_10_0,
        .handler = fliplist_remove_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0)
    },
    {   .action  = ACTION_FLIPLIST_REMOVE_11_0,
        .handler = fliplist_remove_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0)
    },

    {   .action  = ACTION_FLIPLIST_NEXT_8_0,
        .handler = fliplist_next_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_NEXT_9_0,
        .handler = fliplist_next_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0)
    },
    {   .action  = ACTION_FLIPLIST_NEXT_10_0,
        .handler = fliplist_next_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0)
    },
    {   .action  = ACTION_FLIPLIST_NEXT_11_0,
        .handler = fliplist_next_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0)
    },
    {   .action  = ACTION_FLIPLIST_PREVIOUS_8_0,
        .handler = fliplist_previous_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_PREVIOUS_9_0,
        .handler = fliplist_previous_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0)
    },
    {   .action  = ACTION_FLIPLIST_PREVIOUS_10_0,
        .handler = fliplist_previous_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0)
    },
    {   .action  = ACTION_FLIPLIST_PREVIOUS_11_0,
        .handler = fliplist_previous_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0)
    },

    {   .action  = ACTION_FLIPLIST_CLEAR_8_0,
        .handler = fliplist_clear_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0)
    },
    {   .action  = ACTION_FLIPLIST_CLEAR_9_0,
        .handler = fliplist_clear_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0)
    },
    {   .action  = ACTION_FLIPLIST_CLEAR_10_0,
        .handler = fliplist_clear_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0)
    },
    {   .action  = ACTION_FLIPLIST_CLEAR_11_0,
        .handler = fliplist_clear_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0)
    },

    {   .action  = ACTION_FLIPLIST_LOAD_8_0,
        .handler = fliplist_load_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_FLIPLIST_LOAD_9_0,
        .handler = fliplist_load_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_FLIPLIST_LOAD_10_0,
        .handler = fliplist_load_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_FLIPLIST_LOAD_11_0,
        .handler = fliplist_load_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0),
        .blocks  = true,
        .dialog  = true
    },

    {   .action  = ACTION_FLIPLIST_SAVE_8_0,
        .handler = fliplist_save_action,
        .data    = UNIT_DRIVE_TO_PTR(8, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action = ACTION_FLIPLIST_SAVE_9_0,
        .handler = fliplist_save_action,
        .data    = UNIT_DRIVE_TO_PTR(9, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_FLIPLIST_SAVE_10_0,
        .handler = fliplist_save_action,
        .data    = UNIT_DRIVE_TO_PTR(10, 0),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_FLIPLIST_SAVE_11_0,
        .handler = fliplist_save_action,
        .data    = UNIT_DRIVE_TO_PTR(11, 0),
        .blocks  = true,
        .dialog  = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register drive-related actions */
void actions_drive_register(void)
{
    ui_actions_register(drive_actions);
}

#undef  UNPACK_UD
