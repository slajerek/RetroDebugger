/*
 * menu_snapshot.c - Implementation of the snapshot settings menu for the SDL UI.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actions-snapshot.h"
#include "archdep.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "resources.h"
#include "snapshot.h"
#include "ui.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "util.h"
#include "vice-event.h"

#include "menu_snapshot.h"


static int save_disks = 1;
static int save_roms = 0;

UI_MENU_DEFINE_RADIO(EventStartMode)

static UI_MENU_CALLBACK(toggle_save_disk_images_callback)
{
    if (activated) {
        save_disks = !save_disks;
    } else if (save_disks) {
        return sdl_menu_text_tick;
    }
    return NULL;
}

static UI_MENU_CALLBACK(toggle_save_rom_images_callback)
{
    if (activated) {
        save_roms = !save_roms;
    } else if (save_roms) {
        return sdl_menu_text_tick;
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_snapshot_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Choose snapshot file to save", FILEREQ_MODE_SAVE_FILE);
        if (name != NULL) {
            util_add_extension(&name, "vsf");
            if (machine_write_snapshot(name, save_roms, save_disks, 0) < 0) {
                snapshot_display_error();
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_SNAPSHOT_SAVE);
    }
    return NULL;
}

#if 0
/* FIXME */
static UI_MENU_CALLBACK(save_snapshot_slot_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_slot_selection_dialog("Choose snapshot slot to save", SLOTREQ_MODE_SAVE_SLOT);
        if (name != NULL) {
            util_add_extension(&name, "vsf");
            if (machine_write_snapshot(name, save_roms, save_disks, 0) < 0) {
                snapshot_display_error();
            }
            lib_free(name);
        }
    }
    return NULL;
}
#endif

#if 0
static UI_MENU_CALLBACK(quickload_snapshot_callback)
{
    if (activated) {
        if (machine_read_snapshot("snapshot.vsf", 0) < 0) {
           snapshot_display_error();
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(quicksave_snapshot_callback)
{
    if (activated) {
        if (machine_write_snapshot("snapshot.vsf", save_roms, save_disks, 0) < 0) {
            snapshot_display_error();
        }
    }
    return NULL;
}
#endif

#if 0
static UI_MENU_CALLBACK(start_stop_recording_history_callback)
{
    int recording_new;

    recording_new = (event_record_active() ? 0 : 1);
    if (activated) {
        if (recording_new) {
            event_record_start();
        } else {
            event_record_stop();
        }
        return sdl_menu_text_exit_ui;
    } else {
        if (!recording_new) {
            return "(recording)";
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(start_stop_playback_history_callback)
{
    int playback_new;

    playback_new = (event_playback_active() ? 0 : 1);
    if (activated) {
        if (playback_new) {
            event_playback_start();
        } else {
            event_playback_stop();
        }
        return sdl_menu_text_exit_ui;
    } else {
        if (!playback_new) {
            return "(playing)";
        }
    }
    return NULL;
}
#endif

static UI_MENU_CALLBACK(load_snapshot_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Choose snapshot file to load", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (machine_read_snapshot(name, 0) < 0) {
                snapshot_display_error();
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_SNAPSHOT_LOAD);
    }
    return NULL;
}

#if 0
/* FIXME */
static UI_MENU_CALLBACK(load_snapshot_slot_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_slot_selection_dialog("Choose snapshot slot to load", SLOTREQ_MODE_CHOOSE_SLOT);
        if (name != NULL) {
            if (machine_read_snapshot(name, 0) < 0) {
                snapshot_display_error();
            }
            lib_free(name);
        }
    }
    return NULL;
}
#endif
#if 0
static UI_MENU_CALLBACK(set_milestone_callback)
{
    if (activated) {
        event_record_set_milestone();
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}

static UI_MENU_CALLBACK(return_to_milestone_callback)
{
    if (activated) {
        event_record_reset_milestone();
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}
#endif

static UI_MENU_CALLBACK(select_history_files_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select event history directory", FILEREQ_MODE_CHOOSE_DIR);
        if (name != NULL) {
            resources_set_string("EventSnapshotDir", name);
        }
    }
    return NULL;
}

static const ui_menu_entry_t save_snapshot_menu[] = {
    {   .string   = "Save currently attached disk images",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = toggle_save_disk_images_callback
    },
    {   .string   = "Save currently attached ROM images",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = toggle_save_rom_images_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Select filename and save snapshot",
        .type     = MENU_ENTRY_DIALOG,
        .callback = save_snapshot_callback
    },
    SDL_MENU_LIST_END
};

/* cannot be const, we're changing some items' status fields in UI actions */
ui_menu_entry_t snapshot_menu[] = {
    {   .action   = ACTION_SNAPSHOT_LOAD,
        .string   = "Load snapshot image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = load_snapshot_callback
    },
    {   .action   = ACTION_SNAPSHOT_SAVE,
        .string   = "Save snapshot image",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)save_snapshot_menu
    },
    {   .action    = ACTION_SNAPSHOT_QUICKLOAD,
        .string    = "Quickload snapshot.vsf",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {   .action    = ACTION_SNAPSHOT_QUICKSAVE,
        .string    = "Quicksave snapshot.vsf",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action    = ACTION_HISTORY_RECORD_START,
        .string    = "Start recording history",
        .type      = MENU_ENTRY_OTHER,
        .status    = MENU_STATUS_ACTIVE,
        .activated = MENU_EXIT_UI_STRING,
        .displayed = history_record_display
    },
    {   .action    = ACTION_HISTORY_RECORD_STOP,
        .string    = "Stop recording history",
        .type      = MENU_ENTRY_OTHER,
        .status    = MENU_STATUS_INACTIVE,
        .activated = MENU_EXIT_UI_STRING
    },
    {   .action    = ACTION_HISTORY_PLAYBACK_START,
        .string    = "Start playing back history",
        .type      = MENU_ENTRY_OTHER,
        .status    = MENU_STATUS_ACTIVE,
        .activated = MENU_EXIT_UI_STRING,
        .displayed = history_playback_display
    },
    {   .action    = ACTION_HISTORY_PLAYBACK_STOP,
        .string    = "Stop playing back history",
        .type      = MENU_ENTRY_OTHER,
        .status    = MENU_STATUS_INACTIVE,
        .activated = MENU_EXIT_UI_STRING
    },

    {   .action    = ACTION_HISTORY_MILESTONE_SET,
        .string    = "Set recording milestone",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {   .action    = ACTION_HISTORY_MILESTONE_RESET,
        .string    = "Return to milestone",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Record start mode"),
    {   .string   = "Save new snapshot",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_EventStartMode_callback,
        .data     = (ui_callback_data_t)EVENT_START_MODE_FILE_SAVE
    },
    {   .string   = "Load existing snapshot",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_EventStartMode_callback,
        .data     = (ui_callback_data_t)EVENT_START_MODE_FILE_LOAD
    },
    {   .string   = "Start with reset",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_EventStartMode_callback,
        .data     = (ui_callback_data_t)EVENT_START_MODE_RESET
    },
    {   .string   = "Overwrite playback",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_EventStartMode_callback,
        .data     = (ui_callback_data_t)EVENT_START_MODE_PLAYBACK
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Select history files/directory",
        .type     = MENU_ENTRY_DIALOG,
        .callback = select_history_files_callback
    },
    SDL_MENU_LIST_END
};


/* The following are accessors for the snapshot action handlers */

/** \brief  Determine if disk images need to be saved with snapshots
 *
 * \return  non-0 if true
 */
int menu_snapshot_get_save_disks(void)
{
    return save_disks;
}

/** \brief  Determine if ROM images need to be saved with snapshots
 *
 * \return  non-0 if true
 */
int menu_snapshot_get_save_roms(void)
{
    return save_roms;
}

