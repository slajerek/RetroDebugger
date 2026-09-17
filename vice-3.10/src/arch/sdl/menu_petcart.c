/*
 * menu_petcart.c - Implementation of the pet cartridge settings menu for the SDL UI.
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

#include "cartio.h"
#include "menu_common.h"
#include "resources.h"
#include "ui.h"
#include "uimenu.h"

#include "menu_petcart.h"


UI_MENU_DEFINE_FILE_STRING(RomModule9Name)
UI_MENU_DEFINE_FILE_STRING(RomModuleAName)
UI_MENU_DEFINE_FILE_STRING(RomModuleBName)

static UI_MENU_CALLBACK(detach_cart_callback)
{
    if (activated) {
        resources_set_string((char *)param, "");
    }
    return NULL;
}

UI_MENU_DEFINE_RADIO(IOCollisionHandling)

static const ui_menu_entry_t iocollision_menu[] = {
    {   .string   = "Detach all",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOCollisionHandling_callback,
        .data     = (ui_callback_data_t)IO_COLLISION_METHOD_DETACH_ALL
    },
    {   .string   = "Detach last",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOCollisionHandling_callback,
        .data     = (ui_callback_data_t)IO_COLLISION_METHOD_DETACH_LAST
    },
    {   .string   = "AND values",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOCollisionHandling_callback,
        .data     = (ui_callback_data_t)IO_COLLISION_METHOD_AND_WIRES
    },
    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(iocollision_show_type_callback)
{
    int type;

    resources_get_int("IOCollisionHandling", &type);
    switch (type) {
        case IO_COLLISION_METHOD_DETACH_ALL:
            return MENU_SUBMENU_STRING " detach all";
            break;
        case IO_COLLISION_METHOD_DETACH_LAST:
            return MENU_SUBMENU_STRING " detach last";
            break;
        case IO_COLLISION_METHOD_AND_WIRES:
            return MENU_SUBMENU_STRING " AND values";
            break;
    }
    return "n/a";
}

const ui_menu_entry_t petcart_menu[] = {
    {   .string   = "Attach ROM 9",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RomModule9Name_callback,
        .data     = (ui_callback_data_t)"Select ROM 9 image"
    },
    {   .string   = "Detach ROM 9",
        .type     = MENU_ENTRY_OTHER,
        .callback = detach_cart_callback,
        .data     = (ui_callback_data_t)"RomModule9Name"
    },
    {   .string   = "Attach ROM A",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RomModuleAName_callback,
        .data     = (ui_callback_data_t)"Select ROM A image"
    },
    {   .string   = "Detach ROM A",
        .type     = MENU_ENTRY_OTHER,
        .callback = detach_cart_callback,
        .data     = (ui_callback_data_t)"RomModuleAName"
    },
    {   .string   = "Attach ROM B",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RomModuleBName_callback,
        .data     = (ui_callback_data_t)"Select ROM B image"
    },
    {   .string   = "Detach ROM B",
        .type     = MENU_ENTRY_OTHER,
        .callback = detach_cart_callback,
        .data     = (ui_callback_data_t)"RomModuleBName"
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "I/O collision handling ($8800-$8FFF/$E900-$EEFF)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    SDL_MENU_LIST_END
};
