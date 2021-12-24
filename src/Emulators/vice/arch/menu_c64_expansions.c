/*
 * menu_c64_expansions.c - C64 expansions menu for SDL UI.
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

#include "types.h"

#include "menu_c64_expansions.h"
#include "menu_common.h"
#include "uimenu.h"

/* C64 256K MEMORY EXPANSION HACK MENU */

UI_MENU_DEFINE_TOGGLE(C64_256K)
UI_MENU_DEFINE_RADIO(C64_256Kbase)
UI_MENU_DEFINE_FILE_STRING(C64_256Kfilename)

const ui_menu_entry_t c64_256k_menu[] = {
    { "Enable C64 256K",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_C64_256K_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Base address"),
    { "$DE00",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_C64_256Kbase_callback,
      (ui_callback_data_t)0xde00 },
    { "$DE80",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_C64_256Kbase_callback,
      (ui_callback_data_t)0xde80 },
    { "$DF00",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_C64_256Kbase_callback,
      (ui_callback_data_t)0xdf00 },
    { "$DF80",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_C64_256Kbase_callback,
      (ui_callback_data_t)0xdf80 },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "C64 256K image file",
      MENU_ENTRY_DIALOG,
      file_string_C64_256Kfilename_callback,
      (ui_callback_data_t)"Select C64 256K image" },
    SDL_MENU_LIST_END
};


/* PLUS60K MEMORY EXPANSION HACK MENU */

UI_MENU_DEFINE_TOGGLE(PLUS60K)
UI_MENU_DEFINE_RADIO(PLUS60Kbase)
UI_MENU_DEFINE_FILE_STRING(PLUS60Kfilename)

const ui_menu_entry_t plus60k_menu[] = {
    { "Enable PLUS60K",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_PLUS60K_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Base address"),
    { "$D040",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_PLUS60Kbase_callback,
      (ui_callback_data_t)0xd040 },
    { "$D100",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_PLUS60Kbase_callback,
      (ui_callback_data_t)0xd100 },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "PLUS60K image file",
      MENU_ENTRY_DIALOG,
      file_string_PLUS60Kfilename_callback,
      (ui_callback_data_t)"Select PLUS60K image" },
    SDL_MENU_LIST_END
};


/* PLUS256K MEMORY EXPANSION HACK MENU */

UI_MENU_DEFINE_TOGGLE(PLUS256K)
UI_MENU_DEFINE_FILE_STRING(PLUS256Kfilename)

const ui_menu_entry_t plus256k_menu[] = {
    { "Enable PLUS256K",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_PLUS256K_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "PLUS256K image file",
      MENU_ENTRY_DIALOG,
      file_string_PLUS256Kfilename_callback,
      (ui_callback_data_t)"Select PLUS256K image" },
    SDL_MENU_LIST_END
};
