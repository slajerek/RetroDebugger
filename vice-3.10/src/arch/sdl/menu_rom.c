/*
 * menu_rom.c - ROM menu for SDL UI.
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

#include "functionrom.h"
#include "menu_common.h"
#include "menu_drive_rom.h"
#include "petrom.h"
#include "resources.h"
#include "types.h"
#include "uimenu.h"

#include "menu_rom.h"


UI_MENU_DEFINE_FILE_STRING(InternalFunctionName)

UI_MENU_DEFINE_RADIO(InternalFunctionROM)

UI_MENU_DEFINE_TOGGLE(InternalFunctionROMRTCSave)

const ui_menu_entry_t int_func_rom_menu[] = {
    {   .string   = "None",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_InternalFunctionROM_callback,
        .data     = (ui_callback_data_t)INT_FUNCTION_NONE
    },
    {   .string   = "ROM",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_InternalFunctionROM_callback,
        .data     = (ui_callback_data_t)INT_FUNCTION_ROM
    },
    {   .string   = "RAM",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_InternalFunctionROM_callback,
        .data     = (ui_callback_data_t)INT_FUNCTION_RAM
    },
    {   .string   = "RTC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_InternalFunctionROM_callback,
        .data     = (ui_callback_data_t)INT_FUNCTION_RTC
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t c128_function_rom_menu[] = {
    {   .string   = "Internal function ROM type",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)int_func_rom_menu
    },
    {   .string   = "Internal function ROM file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_InternalFunctionName_callback,
        .data     = (ui_callback_data_t)"Select internal function ROM image"
    },
    {   .string   = "Save Internal Function RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_InternalFunctionROMRTCSave_callback
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_FILE_STRING(KernalIntName)
UI_MENU_DEFINE_FILE_STRING(KernalDEName)
UI_MENU_DEFINE_FILE_STRING(KernalFIName)
UI_MENU_DEFINE_FILE_STRING(KernalFRName)
UI_MENU_DEFINE_FILE_STRING(KernalITName)
UI_MENU_DEFINE_FILE_STRING(KernalNOName)
UI_MENU_DEFINE_FILE_STRING(KernalSEName)
UI_MENU_DEFINE_FILE_STRING(KernalCHName)
UI_MENU_DEFINE_FILE_STRING(BasicLoName)
UI_MENU_DEFINE_FILE_STRING(BasicHiName)
UI_MENU_DEFINE_FILE_STRING(ChargenIntName)
UI_MENU_DEFINE_FILE_STRING(ChargenDEName)
UI_MENU_DEFINE_FILE_STRING(ChargenFIName)
UI_MENU_DEFINE_FILE_STRING(ChargenFRName)
UI_MENU_DEFINE_FILE_STRING(ChargenITName)
UI_MENU_DEFINE_FILE_STRING(ChargenNOName)
UI_MENU_DEFINE_FILE_STRING(ChargenSEName)
UI_MENU_DEFINE_FILE_STRING(ChargenCHName)
UI_MENU_DEFINE_FILE_STRING(Kernal64Name)
UI_MENU_DEFINE_FILE_STRING(Basic64Name)

UI_MENU_DEFINE_FILE_STRING(KernalName)
UI_MENU_DEFINE_FILE_STRING(BasicName)
UI_MENU_DEFINE_FILE_STRING(ChargenName)
UI_MENU_DEFINE_FILE_STRING(SCPU64Name)

const ui_menu_entry_t c128_rom_menu[] = {
    {   .string   = "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c128_drive_rom_menu
    },
    {   .string   = "Function ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c128_function_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "International kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalIntName_callback,
        .data     = (ui_callback_data_t)"Select International kernal ROM image"
    },
    {   .string   = "German kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalDEName_callback,
        .data     = (ui_callback_data_t)"Select German kernal ROM image"
    },
    {   .string   = "Finnish kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalFIName_callback,
        .data     = (ui_callback_data_t)"Select Finnish kernal ROM image"
    },
    {   .string   = "French kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalFRName_callback,
        .data     = (ui_callback_data_t)"Select French kernal ROM image"
    },
    {   .string   = "Italian kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalITName_callback,
        .data     = (ui_callback_data_t)"Select Italian kernal ROM image"
    },
    {   .string   = "Norwegian kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalNOName_callback,
        .data     = (ui_callback_data_t)"Select Norwegian kernal ROM image"
    },
    {   .string   = "Swedish kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalSEName_callback,
        .data     = (ui_callback_data_t)"Select Swedish kernal ROM image"
    },
    {   .string   = "Swiss kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalCHName_callback,
        .data     = (ui_callback_data_t)"Select Swiss kernal ROM image"
    },
    {   .string   = "Basic low",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicLoName_callback,
        .data     = (ui_callback_data_t)"Select basic low ROM image"
    },
    {   .string   = "Basic high",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicHiName_callback,
        .data     = (ui_callback_data_t)"Select basic high ROM image"
    },
    {   .string   = "International chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenIntName_callback,
        .data     = (ui_callback_data_t)"Select International chargen ROM image"
    },
    {   .string   = "German chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenDEName_callback,
        .data     = (ui_callback_data_t)"Select German chargen ROM image"
    },
    {   .string   = "Finnish chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenFIName_callback,
        .data     = (ui_callback_data_t)"Select Finnish chargen ROM image"
    },
    {   .string   = "French chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenFRName_callback,
        .data     = (ui_callback_data_t)"Select French chargen ROM image"
    },
    {   .string   = "Italian chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenITName_callback,
        .data     = (ui_callback_data_t)"Select Italian chargen ROM image"
    },
    {   .string   = "Norwegian chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenNOName_callback,
        .data     = (ui_callback_data_t)"Select Norwegian chargen ROM image"
    },
    {   .string   = "Swedish chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenSEName_callback,
        .data     = (ui_callback_data_t)"Select Swedish chargen ROM image"
    },
    {   .string   = "Swiss chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenCHName_callback,
        .data     = (ui_callback_data_t)"Select Swiss chargen ROM image"
    },
    {   .string   = "C64 mode kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_Kernal64Name_callback,
        .data     = (ui_callback_data_t)"Select C64 mode kernal ROM image"
    },
    {   .string   = "C64 mode basic",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_Basic64Name_callback,
        .data     = (ui_callback_data_t)"Select C64 mode basic ROM image"
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t c64_vic20_rom_menu[] = {
    {   .string   = "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)iec_ieee_drive_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "Kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalName_callback,
        .data     = (ui_callback_data_t)"Select kernal ROM image"
    },
    {   .string   = "Basic",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicName_callback,
        .data     = (ui_callback_data_t)"Select basic ROM image"
    },
    {   .string   = "Chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenName_callback,
        .data     = (ui_callback_data_t)"Select chargen ROM image"
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t c64dtv_rom_menu[] = {
    {   .string   =  "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)iec_drive_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "Kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalName_callback,
        .data     = (ui_callback_data_t)"Select kernal ROM image"
    },
    {   .string   = "Basic",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicName_callback,
        .data     = (ui_callback_data_t)"Select basic ROM image"
    },
    {   .string   = "Chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenName_callback,
        .data     = (ui_callback_data_t)"Select chargen ROM image"
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t cbm2_rom_menu[] = {
    {   .string   = "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ieee_drive_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "Kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalName_callback,
        .data     = (ui_callback_data_t)"Select kernal ROM image"
    },
    {   .string   = "Basic",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicName_callback,
        .data     = (ui_callback_data_t)"Select basic ROM image"
    },
    {   .string   = "Chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenName_callback,
        .data     = (ui_callback_data_t)"Select chargen ROM image"
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t scpu64_rom_menu[] = {
    {   .string   = "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)iec_ieee_drive_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "SCPU64",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_SCPU64Name_callback,
        .data     = (ui_callback_data_t)"Select SCPU64 ROM image"
    },
    {   .string   = "Chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenName_callback,
        .data     = (ui_callback_data_t)"Select chargen ROM image"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_FILE_STRING(EditorName)

UI_MENU_DEFINE_FILE_STRING(RomModule9Name)
UI_MENU_DEFINE_FILE_STRING(RomModuleAName)
UI_MENU_DEFINE_FILE_STRING(RomModuleBName)

UI_MENU_DEFINE_FILE_STRING(H6809RomAName)
UI_MENU_DEFINE_FILE_STRING(H6809RomBName)
UI_MENU_DEFINE_FILE_STRING(H6809RomCName)
UI_MENU_DEFINE_FILE_STRING(H6809RomDName)
UI_MENU_DEFINE_FILE_STRING(H6809RomEName)
UI_MENU_DEFINE_FILE_STRING(H6809RomFName)

UI_MENU_DEFINE_TOGGLE(Basic1)
static UI_MENU_CALLBACK(custom_ChargenName_callback)
{
    if (activated) {
        resources_set_string("ChargenName", (const char *)param);
    }
    return NULL;
}


const ui_menu_entry_t pet_rom_menu[] = {
    {   .string   = "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ieee_drive_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "Kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalName_callback,
        .data     = (ui_callback_data_t)"Select kernal ROM image"
    },
    {   .string   = "Basic",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicName_callback,
        .data     = (ui_callback_data_t)"Select basic ROM image"
    },
    {   .string   = "Chargen",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_ChargenName_callback,
        .data     = (ui_callback_data_t)"Select chargen ROM image"
    },
    {   .string   = "Editor",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_EditorName_callback,
        .data     = (ui_callback_data_t)"Select editor ROM image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Patch Kernal v1 to make the IEEE-488 interface work",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Basic1_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Load normal character set ROM",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_ChargenName_callback,
        .data     = (void*)PET_CHARGEN1_NAME
    },
    {   .string   = "Load German character set ROM",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = custom_ChargenName_callback,
        .data     = (void*)PET_CHARGEN_DE_NAME
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Module ROMs"),
    {   .string   = "$9000 ROM Module",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RomModule9Name_callback,
        .data     = (ui_callback_data_t)"Select $9000 ROM Module image"
    },
    {   .string   = "$A000 ROM Module",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RomModuleAName_callback,
        .data     = (ui_callback_data_t)"Select $A000 ROM Module image"
    },
    {   .string   = "$B000 ROM Module",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RomModuleBName_callback,
        .data     = (ui_callback_data_t)"Select $B000 ROM Module image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("SuperPET 6809 Mode ROMs"),
    {   .string   = "SuperPET 6809 $A000 ROM",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_H6809RomAName_callback,
        .data     = (ui_callback_data_t)"Select SuperPET 6809 $A000 ROM image"
    },
    {   .string   = "SuperPET 6809 $B000 ROM",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_H6809RomBName_callback,
        .data     = (ui_callback_data_t)"Select SuperPET 6809 $B000 ROM image"
    },
    {   .string   = "SuperPET 6809 $C000 ROM",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_H6809RomCName_callback,
        .data     = (ui_callback_data_t)"Select SuperPET 6809 $C000 ROM image"
    },
    {   .string   = "SuperPET 6809 $D000 ROM",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_H6809RomDName_callback,
        .data     = (ui_callback_data_t)"Select SuperPET 6809 $D000 ROM image"
    },
    {   .string   = "SuperPET 6809 $E000 ROM",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_H6809RomEName_callback,
        .data     = (ui_callback_data_t)"Select SuperPET 6809 $E000 ROM image"
    },
    {   .string   = "SuperPET 6809 $F000 ROM",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_H6809RomFName_callback,
        .data     = (ui_callback_data_t)"Select SuperPET 6809 $F000 ROM image"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_FILE_STRING(FunctionLowName)
UI_MENU_DEFINE_FILE_STRING(FunctionHighName)
UI_MENU_DEFINE_FILE_STRING(c1loName)
UI_MENU_DEFINE_FILE_STRING(c1hiName)
UI_MENU_DEFINE_FILE_STRING(c2loName)
UI_MENU_DEFINE_FILE_STRING(c2hiName)

const ui_menu_entry_t plus4_rom_menu[] = {
    {   .string   = "Drive ROMs",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)plus4_drive_rom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Computer ROMs"),
    {   .string   = "Kernal",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_KernalName_callback,
        .data     = (ui_callback_data_t)"Select kernal ROM image"
    },
    {   .string   = "Basic",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_BasicName_callback,
        .data     = (ui_callback_data_t)"Select basic ROM image"
    },
    {   .string   = "Function low",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_FunctionLowName_callback,
        .data     = (ui_callback_data_t)"Select Function low ROM image"
    },
    {   .string   = "Function high",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_FunctionHighName_callback,
        .data     = (ui_callback_data_t)"Select Function high ROM image"
    },
    {   .string   = "C1 low",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_c1loName_callback,
        .data     = (ui_callback_data_t)"Select C1 low ROM image"
    },
    {   .string   = "C1 high",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_c1hiName_callback,
        .data     = (ui_callback_data_t)"Select C1 high ROM image"
    },
    {   .string   = "C2 low",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_c2loName_callback,
        .data     = (ui_callback_data_t)"Select C2 low ROM image"
    },
    {   .string   = "C2 high",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_c2hiName_callback,
        .data     = (ui_callback_data_t)"Select C2 high ROM image"
    },
    SDL_MENU_LIST_END
};
