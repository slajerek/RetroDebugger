/** \file   actions-datasette.c
 * \brief   UI action implementations for datasette-related dialogs and settings
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

#include <stddef.h>
#include <stdbool.h>

#include "datasette.h"
#include "tape.h"
#include "types.h"
#include "uiactions.h"
#include "uitapeattach.h"
#include "uitapecreate.h"

#include "actions-datasette.h"


/** \brief  Encode datasette port number and control code in void pointer
 *
 * Encode \a p | \a c << 8 as a void pointer for use in UI action map data.
 *
 * \param[in]   p   port number (1-2)
 * \param[in]   c   control code
 *
 * \return  void pointer for use as the \c data member of a \c ui_action_map_t
 *
 * \see     \c src/datasette/datasette.h for control codes
 */
#define DS_PC(p, c) (vice_int_to_ptr((p) | (c << 8)))


/** \brief  Send control command to datasette action
 *
 * Trigger a virtual button on a datasette. The \c data member of the \a map
 * contains the port number and control code.
 *
 * \param[in]   self    action map
 */
static void tape_control_action(ui_action_map_t *self)
{
    int port    = vice_ptr_to_int(self->data) & 0xff;
    int control = vice_ptr_to_int(self->data) >> 8;

    /* UI code uses 1 or 2 for port numbers, while *some* datasette code uses
     * port indexes 0 or 1: */
    datasette_control(port - 1, control);
}

/** \brief  Show tape attach dialog action
 *
 * \param[in]   self    action map
 */
static void tape_attach_action(ui_action_map_t *self)
{
    ui_tape_attach_show_dialog(vice_ptr_to_int(self->data));
}

/** \brief  Show tape create dialog action
 *
 * \param[in]   self    action map
 */
static void tape_create_action(ui_action_map_t *self)
{
    ui_tape_create_dialog_show(vice_ptr_to_int(self->data));
}

/** \brief  Detach tape action
 *
 * \param[in]   self    action map
 */
static void tape_detach_action(ui_action_map_t *self)
{
    tape_image_detach(vice_ptr_to_int(self->data));
}


/** \brief  List of actions for datasettes */
static const ui_action_map_t datasette_actions[] = {
    /* Datasette #1 image actions */
    {   .action  = ACTION_TAPE_ATTACH_1,
        .handler = tape_attach_action,
        .data    = vice_int_to_ptr(1),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_TAPE_CREATE_1,
        .handler = tape_create_action,
        .data    = vice_int_to_ptr(1),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_TAPE_DETACH_1,
        .handler = tape_detach_action,
        .data    = vice_int_to_ptr(1)
    },

    /* Datasette #1 command actions */
    {   .action  = ACTION_TAPE_STOP_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_STOP)
    },
    {   .action  = ACTION_TAPE_PLAY_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_START)
    },
    {   .action  = ACTION_TAPE_FFWD_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_FORWARD)
    },
    {   .action  = ACTION_TAPE_REWIND_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_REWIND)
    },
    {   .action  = ACTION_TAPE_RECORD_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_RECORD)
    },
    {   .action  = ACTION_TAPE_RESET_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_RESET)
    },
    {   .action  = ACTION_TAPE_RESET_COUNTER_1,
        .handler = tape_control_action,
        .data    = DS_PC(1, DATASETTE_CONTROL_RESET_COUNTER)
    },

    /* Datasette #2 image actions */
    {   .action  = ACTION_TAPE_ATTACH_2,
        .handler = tape_attach_action,
        .data    = vice_int_to_ptr(2),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_TAPE_CREATE_2,
        .handler = tape_create_action,
        .data    = vice_int_to_ptr(2),
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_TAPE_DETACH_2,
        .handler = tape_detach_action,
        .data    = vice_int_to_ptr(2)
    },

    /* Datasette #2 command actions */
    {   .action  = ACTION_TAPE_STOP_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_STOP)
    },
    {   .action  = ACTION_TAPE_PLAY_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_START)
    },
    {   .action  = ACTION_TAPE_FFWD_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_FORWARD)
    },
    {   .action  = ACTION_TAPE_REWIND_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_REWIND)
    },
    {   .action  = ACTION_TAPE_RECORD_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_RECORD)
    },
    {   .action  = ACTION_TAPE_RESET_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_RESET)
    },
    {   .action  = ACTION_TAPE_RESET_COUNTER_2,
        .handler = tape_control_action,
        .data    = DS_PC(2, DATASETTE_CONTROL_RESET_COUNTER)
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register datasette-related actions */
void actions_datasette_register(void)
{
    ui_actions_register(datasette_actions);
}

#undef DS_PC
