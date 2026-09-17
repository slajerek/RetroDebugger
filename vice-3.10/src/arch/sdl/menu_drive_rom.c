/*
 * menu_drive_rom.c - Drive ROM menu for SDL UI.
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

#include "menu_common.h"
#include "menu_drive_rom.h"
#include "uimenu.h"

UI_MENU_DEFINE_FILE_STRING(DosName1540)
UI_MENU_DEFINE_FILE_STRING(DosName1541)
UI_MENU_DEFINE_FILE_STRING(DosName1541ii)
UI_MENU_DEFINE_FILE_STRING(DosName1570)
UI_MENU_DEFINE_FILE_STRING(DosName1571)
UI_MENU_DEFINE_FILE_STRING(DosName1581)
UI_MENU_DEFINE_FILE_STRING(DosName2000)
UI_MENU_DEFINE_FILE_STRING(DosName4000)
UI_MENU_DEFINE_FILE_STRING(DosNameCMDHD)
UI_MENU_DEFINE_FILE_STRING(DosName2031)
UI_MENU_DEFINE_FILE_STRING(DosName2040)
UI_MENU_DEFINE_FILE_STRING(DosName3040)
UI_MENU_DEFINE_FILE_STRING(DosName4040)
UI_MENU_DEFINE_FILE_STRING(DosName1001)
UI_MENU_DEFINE_FILE_STRING(DosName9000)
UI_MENU_DEFINE_FILE_STRING(DosName1571cr)
UI_MENU_DEFINE_FILE_STRING(DosName1551)

#define DRIVE_ROM_1540_ITEM                                         \
    {   .string   = "1540 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1540_callback,               \
        .data     = (ui_callback_data_t)"Select 1540 ROM image"     \
    }

#define DRIVE_ROM_1541_ITEM                                         \
    {   .string   = "1541 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1541_callback,               \
        .data     = (ui_callback_data_t)"Select 1541 ROM image"     \
    }

#define DRIVE_ROM_1541II_ITEM                                       \
    {   .string   =  "1541-II ROM file",                            \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1541ii_callback,             \
        .data     = (ui_callback_data_t)"Select 1541-II ROM image"  \
    }

#define DRIVE_ROM_1551_ITEM                                         \
    {   .string   = "1551 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1551_callback,               \
        .data     = (ui_callback_data_t)"Select 1551 ROM image"     \
    }

#define DRIVE_ROM_1570_ITEM                                         \
    {   .string   = "1570 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1570_callback,               \
        .data     = (ui_callback_data_t)"Select 1570 ROM image"     \
    }

#define DRIVE_ROM_1571_ITEM                                         \
    {   .string   = "1571 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1571_callback,               \
        .data     = (ui_callback_data_t)"Select 1571 ROM image"     \
    }

#define DRIVE_ROM_1571CR_ITEM                                       \
    {   .string   = "1571CR ROM file",                              \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1571cr_callback,             \
        .data     = (ui_callback_data_t)"Select 1571CR ROM image"   \
    }

#define DRIVE_ROM_1581_ITEM                                         \
    {   .string   = "1581 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1581_callback,               \
        .data     = (ui_callback_data_t)"Select 1581 ROM image"     \
    }

#define DRIVE_ROM_2000_ITEM                                         \
    {   .string   = "2000 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName2000_callback,               \
        .data     = (ui_callback_data_t)"Select 2000 ROM image"     \
    }

#define DRIVE_ROM_4000_ITEM                                         \
    {   .string   = "4000 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName4000_callback,               \
        .data     = (ui_callback_data_t)"Select 4000 ROM image"     \
    }

#define DRIVE_ROM_CMDHD_ITEM                                        \
    {   .string   = "CMDHD ROM file",                               \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosNameCMDHD_callback,              \
        .data     = (ui_callback_data_t)"Select CMDHD ROM image"    \
    }

#define DRIVE_ROM_2031_ITEM                                         \
    {   .string   = "2031 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName2031_callback,               \
        .data     = (ui_callback_data_t)"Select 2031 ROM image"     \
    }

#define DRIVE_ROM_2040_ITEM                                         \
    {   .string   = "2040 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName2040_callback,               \
        .data     = (ui_callback_data_t)"Select 2040 ROM image"     \
    }

#define DRIVE_ROM_3040_ITEM                                         \
    {   .string   = "3040 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName3040_callback,               \
        .data     = (ui_callback_data_t)"Select 3040 ROM image"     \
    }

#define DRIVE_ROM_4040_ITEM                                         \
    {   .string   = "4040 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName4040_callback,               \
        .data     = (ui_callback_data_t)"Select 4040 ROM image"     \
    }

#define DRIVE_ROM_1001_ITEM                                         \
    {   .string   = "1001 ROM file",                                \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName1001_callback,               \
        .data     = (ui_callback_data_t)"Select 1001 ROM image"     \
    }

#define DRIVE_ROM_9000_ITEM                                         \
    {   .string   = "D9090/60 ROM file",                            \
        .type     = MENU_ENTRY_DIALOG,                              \
        .callback = file_string_DosName9000_callback,               \
        .data     = (ui_callback_data_t)"Select D9090/60 ROM image" \
    }

const ui_menu_entry_t c128_drive_rom_menu[] = {
    DRIVE_ROM_1540_ITEM,
    DRIVE_ROM_1541_ITEM,
    DRIVE_ROM_1541II_ITEM,
    DRIVE_ROM_1570_ITEM,
    DRIVE_ROM_1571_ITEM,
    DRIVE_ROM_1571CR_ITEM,
    DRIVE_ROM_1581_ITEM,
    DRIVE_ROM_2000_ITEM,
    DRIVE_ROM_4000_ITEM,
    DRIVE_ROM_CMDHD_ITEM,
    DRIVE_ROM_2031_ITEM,
    DRIVE_ROM_2040_ITEM,
    DRIVE_ROM_3040_ITEM,
    DRIVE_ROM_4040_ITEM,
    DRIVE_ROM_1001_ITEM,
    DRIVE_ROM_9000_ITEM,
    SDL_MENU_LIST_END
};

const ui_menu_entry_t plus4_drive_rom_menu[] = {
    DRIVE_ROM_1540_ITEM,
    DRIVE_ROM_1541_ITEM,
    DRIVE_ROM_1541II_ITEM,
    DRIVE_ROM_1551_ITEM,
    DRIVE_ROM_1570_ITEM,
    DRIVE_ROM_1571_ITEM,
    DRIVE_ROM_1581_ITEM,
    DRIVE_ROM_2000_ITEM,
    DRIVE_ROM_4000_ITEM,
    DRIVE_ROM_CMDHD_ITEM,
    SDL_MENU_LIST_END
};

const ui_menu_entry_t iec_ieee_drive_rom_menu[] = {
    DRIVE_ROM_1540_ITEM,
    DRIVE_ROM_1541_ITEM,
    DRIVE_ROM_1541II_ITEM,
    DRIVE_ROM_1570_ITEM,
    DRIVE_ROM_1571_ITEM,
    DRIVE_ROM_1581_ITEM,
    DRIVE_ROM_2000_ITEM,
    DRIVE_ROM_4000_ITEM,
    DRIVE_ROM_CMDHD_ITEM,
    DRIVE_ROM_2031_ITEM,
    DRIVE_ROM_2040_ITEM,
    DRIVE_ROM_3040_ITEM,
    DRIVE_ROM_4040_ITEM,
    DRIVE_ROM_1001_ITEM,
    DRIVE_ROM_9000_ITEM,
    SDL_MENU_LIST_END
};

const ui_menu_entry_t ieee_drive_rom_menu[] = {
    DRIVE_ROM_2031_ITEM,
    DRIVE_ROM_2040_ITEM,
    DRIVE_ROM_3040_ITEM,
    DRIVE_ROM_4040_ITEM,
    DRIVE_ROM_1001_ITEM,
    DRIVE_ROM_9000_ITEM,
    SDL_MENU_LIST_END
};

const ui_menu_entry_t iec_drive_rom_menu[] = {
    DRIVE_ROM_1540_ITEM,
    DRIVE_ROM_1541_ITEM,
    DRIVE_ROM_1541II_ITEM,
    DRIVE_ROM_1570_ITEM,
    DRIVE_ROM_1571_ITEM,
    DRIVE_ROM_1581_ITEM,
    DRIVE_ROM_2000_ITEM,
    DRIVE_ROM_4000_ITEM,
    DRIVE_ROM_CMDHD_ITEM,
    SDL_MENU_LIST_END
};
