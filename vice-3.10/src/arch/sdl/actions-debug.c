/** \file   actions-debug.c
 * \brief   UI action implementations for debugging settings (SDL)
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
#include "debug.h"
#ifdef DEBUG

#include "uiactions.h"
#include "uimenu.h"

#include "actions-debug.h"


/** \brief  List of mappings for debugging actions for all emulators
 */
static const ui_action_map_t debug_actions[] = {
    {   .action  = ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_DEBUG_TRACE_CPU_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"MainCPU_TRACE"
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of mappings for additional debugging actions for C64 DTV
 */
static const ui_action_map_t debug_actions_c64dtv[] = {
    {   .action  = ACTION_DEBUG_BLITTER_LOG_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"DtvBlitterLog"
    },
    {   .action  = ACTION_DEBUG_DMA_LOG_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"DtvDmaLog"
    },
    {   .action  = ACTION_DEBUG_FLASH_LOG_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"DtvFlashLog"
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of mappings for additional debugging actions for drives
 *
 * All emulators except VSID.
 */
static const ui_action_map_t debug_actions_drives[] = {
    {   .action  = ACTION_DEBUG_TRACE_IEC_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"IEC_TRACE"
    },
    {   .action  = ACTION_DEBUG_TRACE_IEEE488_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"IEEE_TRACE"
    },
    {   .action  = ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"Drive0CPU_TRACE"
    },
    {   .action  = ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"Drive1CPU_TRACE"
    },
    {   .action  = ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"Drive2CPU_TRACE"
    },
    {   .action  = ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"Drive3CPU_TRACE"
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register debugging actions */
void actions_debug_register(void)
{
    ui_actions_register(debug_actions);
    if (machine_class != VICE_MACHINE_VSID) {
        ui_actions_register(debug_actions_drives);
    }
    if (machine_class == VICE_MACHINE_C64DTV) {
        ui_actions_register(debug_actions_c64dtv);
    }
}

#endif
