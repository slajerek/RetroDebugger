/** \file   actions-debug.c
 * \brief   UI action implementations for debug-related dialogs and settings
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
 * $VICERES MainCPU_TRACE   all
 * $VICERES IEC_TRACE       -vsid
 * $VICERES IEEE_TRACE      -vsid
 * $VICERES Drive0CPU_TRACE -vsid
 * $VICERES Drive1CPU_TRACE -vsid
 * $VICERES Drive2CPU_TRACE -vsid
 * $VICERES Drive3CPU_TRACE -vsid
 * $VICERES DoCoreDump      -vsid
 * $VICERES DtvBlitterLog   x64dtv
 * $VICERES DtvDMALog       x64dtv
 * $VICERES DtvFlashLog     x64dtv
 */

#include "vice.h"
#include "debug.h"

#ifdef DEBUG

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "hotkeys.h"
#include "machine.h"
#include "resources.h"
#include "uiactions.h"
#include "uidebug.h"
#include "uimenu.h"

#include "actions-debug.h"


/** \brief  Object mapping action IDs to resource names
 */
typedef struct action_resource_s {
    int         action;     /**< action ID */
    const char *resource;   /**< resource name */
} action_resource_t;



/** \brief  Pop up dialog to change trace mode
 *
 * \param[in]   self    action map
 */
static void debug_trace_mode_action(ui_action_map_t *self)
{
    ui_debug_trace_mode_dialog_show();
}

/** \brief  Toggle debug resource action
 *
 * Toggle a debug (trace) resource and update the relevant check button menu
 * item. The resource name is avaible in the \c data member of \a self.
 *
 * \param[in]   self    action map
 *
 * \todo    Make available for general use, there are probably more check items
 *          in the menu directly linked to resources.
 */
static void debug_resource_toggle_action(ui_action_map_t *self)
{
    const char *resource = self->data;
    int         enabled  = 0;

    resources_get_int(resource, &enabled);
    enabled = !enabled;
    resources_set_int(resource, enabled);
    vhk_gtk_set_check_item_blocked_by_action(self->action, (gboolean)enabled);
}

/** \brief  Pop up dialog to change number of auto-playback frames
 *
 * \param[in]   self    action map
 */
static void debug_autoplayback_frames_action(ui_action_map_t *self)
{
    ui_debug_playback_frames_dialog_show();
}


/** \brief  List of debugging-related actions */
static const ui_action_map_t debug_actions[] = {
    {   .action  = ACTION_DEBUG_TRACE_MODE,
        .handler = debug_trace_mode_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action   = ACTION_DEBUG_TRACE_CPU_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"MainCPU_TRACE",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_TRACE_IEC_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"IEC_TRACE",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_TRACE_IEEE488_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"IEEE_TRACE",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"Drive0CPU_TRACE",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"Drive1CPU_TRACE",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"Drive2CPU_TRACE",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"Drive3CPU_TRACE",
        .uithread = true
    },
    {   .action  = ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
        .handler = debug_autoplayback_frames_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action   = ACTION_DEBUG_CORE_DUMP_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"DoCoreDump",
        .uithread = true
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  List of debugging-related actions (C64DTV only) */
static const ui_action_map_t debug_actions_dtv[] = {
    {   .action   = ACTION_DEBUG_BLITTER_LOG_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"DtvBlitterLog",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_DMA_LOG_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"DtvDMALog",
        .uithread = true
    },
    {   .action   = ACTION_DEBUG_FLASH_LOG_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"DtvFlashLog",
        .uithread = true
    },
    UI_ACTION_MAP_TERMINATOR
};

static const ui_action_map_t debug_actions_vsid[] = {
    {
        .action  = ACTION_DEBUG_TRACE_MODE,
        .handler = debug_trace_mode_action,
        .blocks  = true,
        .dialog  = true
    },
    {
        .action   = ACTION_DEBUG_TRACE_CPU_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"MainCPU_TRACE",
        .uithread = true
    },
    {
        .action  = ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
        .handler = debug_autoplayback_frames_action,
        .blocks  = true,
        .dialog  = true
    },
    {
        .action   = ACTION_DEBUG_CORE_DUMP_TOGGLE,
        .handler  = debug_resource_toggle_action,
        .data     = (void*)"DoCoreDump",
        .uithread = true
    },
    UI_ACTION_MAP_TERMINATOR
};



/** \brief  Register debugging actions */
void actions_debug_register(void)
{
    if (machine_class == VICE_MACHINE_VSID) {
        /* VSID */
        ui_actions_register(debug_actions_vsid);
    } else {
        /* non-VSID */
        ui_actions_register(debug_actions);
        /* C64DTV-specific */
        if (machine_class == VICE_MACHINE_C64DTV) {
            ui_actions_register(debug_actions_dtv);
        }
    }
}


/** \brief  List of actions and resources to set up check buttons
 */
static const action_resource_t actions_list[] = {
    { ACTION_DEBUG_TRACE_CPU_TOGGLE,        "MainCPU_TRACE" },
    { ACTION_DEBUG_TRACE_IEC_TOGGLE,        "IEC_TRACE" },
    { ACTION_DEBUG_TRACE_IEEE488_TOGGLE,    "IEEE_TRACE" },
    { ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE,    "Drive0CPU_TRACE" },
    { ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE,    "Drive1CPU_TRACE" },
    { ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE,   "Drive2CPU_TRACE" },
    { ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE,   "Drive3CPU_TRACE" },
    { ACTION_DEBUG_CORE_DUMP_TOGGLE,        "DoCoreDump" },
    { ACTION_DEBUG_BLITTER_LOG_TOGGLE,      "DtvBlitterLog" },
    { ACTION_DEBUG_DMA_LOG_TOGGLE,          "DtvDMALog" },
    { ACTION_DEBUG_FLASH_LOG_TOGGLE,        "DtvFlashLog" }
};


/** \brief  Set debug-related check menu items
 */
void actions_debug_setup_ui(void)
{
    size_t i;

    for (i = 0; i < sizeof actions_list / sizeof actions_list[0]; i++) {
        if (ui_action_is_valid(actions_list[i].action)) {
            int enabled;

            resources_get_int(actions_list[i].resource, &enabled);
            vhk_gtk_set_check_item_blocked_by_action(actions_list[i].action,
                                                     (gboolean)enabled);
        }
    }
}

#endif
