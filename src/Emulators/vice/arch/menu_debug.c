/*
 * menu_debug.c - Debug menu for SDL UI.
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
#include "debug.h"

#ifdef VICE_DEBUG

#include <stdio.h>

#include "vicetypes.h"

#include "menu_common.h"
#include "menu_debug.h"
#include "uimenu.h"

UI_MENU_DEFINE_TOGGLE(MainCPU_TRACE)
UI_MENU_DEFINE_TOGGLE(Drive0CPU_TRACE)
UI_MENU_DEFINE_TOGGLE(Drive1CPU_TRACE)
UI_MENU_DEFINE_RADIO(TraceMode)

const ui_menu_entry_t debug_menu[] = {
    SDL_MENU_ITEM_TITLE("Trace mode"),
    { "Normal",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TraceMode_callback,
      (ui_callback_data_t)DEBUG_NORMAL },
    { "Small",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TraceMode_callback,
      (ui_callback_data_t)DEBUG_SMALL },
    { "History",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TraceMode_callback,
      (ui_callback_data_t)DEBUG_HISTORY },
    { "History autoplay",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_TraceMode_callback,
      (ui_callback_data_t)DEBUG_AUTOPLAY },
    SDL_MENU_ITEM_SEPARATOR,
    { "Main CPU trace",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MainCPU_TRACE_callback,
      NULL },
    { "Drive0 CPU trace",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_Drive0CPU_TRACE_callback,
      NULL },
    { "Drive1 CPU trace",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_Drive1CPU_TRACE_callback,
      NULL },
    SDL_MENU_LIST_END
};

#endif
