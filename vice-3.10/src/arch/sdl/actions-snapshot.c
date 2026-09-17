/** \file   actions-snapshot.c
 * \brief   UI action implementations for snapshots and history recording (SDL)
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

#include "menu_common.h"
#include "menu_snapshot.h"
#include "snapshot.h"
#include "uiactions.h"
#include "uimenu.h"
#include "vice-event.h"

#include "actions-snapshot.h"


/** \brief  Quickload snapshot action
 *
 * \param[in]   self    action map
 */
static void snapshot_quickload_action(ui_action_map_t *self)
{
    if (machine_read_snapshot("snapshot.vsf", 0) < 0) {
        snapshot_display_error();
    }
    ui_action_finish(self->action);
}

/** \brief  Quicksave snapshot action
 *
 * \param[in]   self    action map
 */
static void snapshot_quicksave_action(ui_action_map_t *self)
{
    if (machine_write_snapshot("snapshot.vsf",
                               menu_snapshot_get_save_roms(),
                               menu_snapshot_get_save_disks(),
                               0) < 0) {
        snapshot_display_error();
    }
    ui_action_finish(self->action);
}

/** \brief  Update status of the playback menu items
 *
 * Due to the SDL UI using traps to start/stop playback/recording of items the
 * \c event_playback_active() and \c event_record_active() functions cannot be
 * used while the UI is still active, so we hardcode the item enable/disable
 * stuff here.
 *
 * \param[in]   start   enable start
 * \param[in]   stop    enable stop
 */
static void update_playback_items(bool start, bool stop)
{
    /* enable/disable "Start playback" item */
    sdl_ui_menu_item_set_status_by_action(ACTION_HISTORY_PLAYBACK_START,
                                          start ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE);
    /* enable/disable "Stop playback" item */
    sdl_ui_menu_item_set_status_by_action(ACTION_HISTORY_PLAYBACK_STOP,
                                          stop ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE);
}

/** \brief  Update status of the recording menu items
 *
 * Due to the SDL UI using traps to start/stop playback/recording of items the
 * \c event_playback_active() and \c event_record_active() functions cannot be
 * used while the UI is still active, so we hardcode the item enable/disable
 * stuff here.
 *
 * \param[in]   start   enable start
 * \param[in]   stop    enable stop
 */
static void update_recording_items(bool start, bool stop)
{
    /* enable/disable "Start recording" item */
    sdl_ui_menu_item_set_status_by_action(ACTION_HISTORY_RECORD_START,
                                          start ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE);
    /* enable/disable "Stop recording" item */
    sdl_ui_menu_item_set_status_by_action(ACTION_HISTORY_RECORD_STOP,
                                          stop ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE);
}

/** \brief  Start history playback action
 *
 * \param[in]   self    action map
 */
static void history_playback_start_action(ui_action_map_t *self)
{
    if (!event_playback_active() && !event_record_active()) {
        event_playback_start();
        update_playback_items(false, true);
        update_recording_items(false, false);
    }
    ui_action_finish(self->action);
}

/** \brief  Stop history playback action
 *
 * \param[in]   self    action map
 */
static void history_playback_stop_action(ui_action_map_t *self)
{
    if (event_playback_active() && !event_record_active()) {
        event_playback_stop();
        update_playback_items(true, false);
        update_recording_items(true, false);
    }
    ui_action_finish(self->action);
}

/** \brief  Start history recording action
 *
 * \param[in]   self    action map
 */
static void history_record_start_action(ui_action_map_t *self)
{
    if (!event_record_active() && !event_playback_active()) {
        event_record_start();
        update_playback_items(false, false);
        update_recording_items(false, true);
    }
    ui_action_finish(self->action);
}

/** \brief  Stop history recording action
 *
 * \param[in]   self    action map
 */
static void history_record_stop_action(ui_action_map_t *self)
{
    if (event_record_active() && !event_playback_active()) {
        event_record_stop();
        update_playback_items(true, false);
        update_recording_items(true, false);
    }
    ui_action_finish(self->action);
}

/** \brief  Set history milestone action
 *
 * \param[in]   self    action map
 */
static void history_milestone_set_action(ui_action_map_t *self)
{
    event_record_set_milestone();
}

/** \brief  Reset history to milestone action
 *
 * \param[in]   self    action map
 */
static void history_milestone_reset_action(ui_action_map_t *self)
{
    event_record_reset_milestone();
}


/** \brief  List of mappings for snapshot and history actions */
static const ui_action_map_t snapshot_actions[] = {
    {   .action  = ACTION_SNAPSHOT_LOAD,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_SNAPSHOT_SAVE,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_SNAPSHOT_QUICKLOAD,
        .handler = snapshot_quickload_action,
        .blocks  = true
    },
    {   .action  = ACTION_SNAPSHOT_QUICKSAVE,
        .handler = snapshot_quicksave_action,
        .blocks  = true
    },
    {   .action  = ACTION_HISTORY_PLAYBACK_START,
        .handler = history_playback_start_action,
        .blocks  = true
    },
    {   .action  = ACTION_HISTORY_PLAYBACK_STOP,
        .handler = history_playback_stop_action,
        .blocks  = true
    },
    {   .action  = ACTION_HISTORY_RECORD_START,
        .handler = history_record_start_action,
        .blocks  = true
    },
    {   .action  = ACTION_HISTORY_RECORD_STOP,
        .handler = history_record_stop_action,
        .blocks  = true
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


/** \brief  Display helper for the "Start event recording" menu item
 *
 * \param[in]   item    menu item (unused)
 */
const char *history_record_display(ui_menu_entry_t *item)
{
    return event_record_active() ? "(recording)" : NULL;
}


/** \brief  Display helper for the "Start event playback" menu item
 *
 * \param[in]   item    menu item (unused)
 */
const char *history_playback_display(ui_menu_entry_t *item)
{
    return event_playback_active() ? "(playing)" : NULL;
}
