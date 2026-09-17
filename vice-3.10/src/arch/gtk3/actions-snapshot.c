/** \file   actions-snapshot.c
 * \brief   UI action implementations for snapshots and event recording
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

#include "uiactions.h"
#include "uiapi.h"
#include "uisnapshot.h"
#include "vice-event.h"

#include "actions-snapshot.h"


/* {{{ Snapshit actions */
/** \brief  Pop up dialog to load a snapshot action
 *
 * \param[in]   self    action map
 */
static void snapshot_load_action(ui_action_map_t *self)
{
    ui_snapshot_load_snapshot();
}

/** \brief  Pop up dialog to save a snapshot action
 *
 * \param[in]   self    action map
 */
static void snapshot_save_action(ui_action_map_t *self)
{
    ui_snapshot_save_snapshot();
}

/** \brief  Quickload snapshot action
 *
 * \param[in]   self    action map
 */
static void snapshot_quickload_action(ui_action_map_t *self)
{
    ui_snapshot_quickload_snapshot();
}

/** \brief  Quicksave snapshot action
 *
 * \param[in]   self    action map
 */
static void snapshot_quicksave_action(ui_action_map_t *self)
{
    ui_snapshot_quicksave_snapshot();
}
/* }}} */

/* {{{ History actions */
/** \brief  Start recording history action
 *
 * \param[in]   self    action map
 */
static void history_record_start_action(ui_action_map_t *self)
{
    event_record_start();
    ui_display_recording(UI_RECORDING_STATUS_EVENTS);

}

/** \brief  Stop recording history action
 *
 * \param[in]   self    action map
 */
static void history_record_stop_action(ui_action_map_t *self)
{
    event_record_stop();
    ui_display_recording(UI_RECORDING_STATUS_NONE);
}


/** \brief  Start history playback action
 *
 * \param[in]   self    action map
 */
static void history_playback_start_action(ui_action_map_t *self)
{
    event_playback_start();
}

/** \brief  Stop history playback action
 *
 * \param[in]   self    action map
 */
static void history_playback_stop_action(ui_action_map_t *self)
{
    event_playback_stop();
}

/** \brief  Set history milestone action
 *
 * \param[in]   self    action map
 */
static void history_milestone_set_action(ui_action_map_t *self)
{
    event_record_set_milestone();
}

/** \brief  Rewind to history milestone action
 *
 * \param[in]   self    action map
 */
static void history_milestone_reset_action(ui_action_map_t *self)
{
    event_record_reset_milestone();
}
/* }}} */


/** \brief  List of snapshot and event-related actions */
static const ui_action_map_t snapshot_actions[] = {
    /* Snapshot actions */
    {   .action  = ACTION_SNAPSHOT_LOAD,
        .handler = snapshot_load_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_SNAPSHOT_SAVE,
        .handler = snapshot_save_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_SNAPSHOT_QUICKLOAD,
        .handler = snapshot_quickload_action
    },
    {   .action  = ACTION_SNAPSHOT_QUICKSAVE,
        .handler = snapshot_quicksave_action
    },

    /* History actions */
    {   .action   = ACTION_HISTORY_RECORD_START,
        .handler  = history_record_start_action,
        .uithread = true
    },
    {   .action   = ACTION_HISTORY_RECORD_STOP,
        .handler  = history_record_stop_action,
        .uithread = true
    },
    {   .action   = ACTION_HISTORY_PLAYBACK_START,
        .handler  = history_playback_start_action,
        .uithread = true
    },
    {   .action   = ACTION_HISTORY_PLAYBACK_STOP,
        .handler  = history_playback_stop_action,
        .uithread = true
    },
    {   .action  = ACTION_HISTORY_MILESTONE_SET,
        .handler = history_milestone_set_action
    },
    {   .action  = ACTION_HISTORY_MILESTONE_RESET,
        .handler = history_milestone_reset_action
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register snapshot and history actions */
void actions_snapshot_register(void)
{
    ui_actions_register(snapshot_actions);
}
