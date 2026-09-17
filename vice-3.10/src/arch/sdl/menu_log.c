/*
 * menu_log.c - Implementation of the log settings menu for the SDL UI.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include "lib.h"
#include "menu_common.h"
#include "resources.h"
#include "sound.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "util.h"

#include "menu_log.h"

UI_MENU_DEFINE_STRING(LogFileName);
UI_MENU_DEFINE_TOGGLE(LogToFile)
UI_MENU_DEFINE_TOGGLE(LogToStdout)
UI_MENU_DEFINE_TOGGLE(LogToMonitor)
UI_MENU_DEFINE_RADIO(LogLimit)
UI_MENU_DEFINE_RADIO(LogLevelANE)
UI_MENU_DEFINE_RADIO(LogLevelLXA)

static ui_menu_entry_t log_limit_menu[] = {
    {   .string   = "silent",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLimit_callback,
        .data     = (ui_callback_data_t)LOG_LIMIT_SILENT
    },
    {   .string   = "standard",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLimit_callback,
        .data     = (ui_callback_data_t)LOG_LIMIT_STANDARD
    },
    {   .string   = "verbose",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLimit_callback,
        .data     = (ui_callback_data_t)LOG_LIMIT_VERBOSE
    },
    {   .string   = "debug (all)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLimit_callback,
        .data     = (ui_callback_data_t)LOG_LIMIT_DEBUG
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t log_menu[] = {
    {   .string   = "Log to file",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_LogToFile_callback
    },
    {   .string   = "File name",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_LogFileName_callback,
        .data     = (ui_callback_data_t)"Set log file"
    },
    {   .string   = "Log to stdout",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_LogToStdout_callback
    },
    {   .string   = "Log to monitor",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_LogToMonitor_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Log Limit",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)log_limit_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("ANE log limit"),
    {   .string   = "none",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLevelANE_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "unstable only",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLevelANE_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "all",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLevelANE_callback,
        .data     = (ui_callback_data_t)2
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("LXA log limit"),
    {   .string   = "none",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLevelLXA_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "unstable only",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLevelLXA_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "all",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LogLevelLXA_callback,
        .data     = (ui_callback_data_t)2
    },
    SDL_MENU_LIST_END
};

/** \brief  Clean up memory used by the dynamically created sound output menus
 */
void log_menu_shutdown(void)
{

}
