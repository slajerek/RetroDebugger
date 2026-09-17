/*
 * menu_cbm2cart.c - Implementation of the cbm2 cartridge settings menu for the SDL UI.
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
#include "cartridge.h"
#include "lib.h"
#include "menu_cbm2cart.h"
#include "menu_common.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"

UI_MENU_DEFINE_TOGGLE(CartridgeReset)

static UI_MENU_CALLBACK(attach_cart_callback)
{
    char *name;
    if (activated) {
        name = sdl_ui_file_selection_dialog("Select Cart image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (cartridge_attach_image(vice_ptr_to_int(param), name) < 0) {
                ui_error("Cannot load cartridge image.");
            }
            lib_free(name);
        }
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

const ui_menu_entry_t cbm2cart_menu[] = {
    {   .action   = ACTION_CART_ATTACH,
        .string   = "Attach CRT image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CRT
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_CART_ATTACH_RAW_1000,
        .string   = "Load new Cart $1***",
        .type     = MENU_ENTRY_OTHER,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CBM2_GENERIC_C1
    },
    {   .action   = ACTION_CART_DETACH_1000,
        .string   = "Unload Cart $1***",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_CART_ATTACH_RAW_2000,
        .string   = "Load new Cart $2-3***",
        .type     = MENU_ENTRY_OTHER,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CBM2_GENERIC_C2
    },
    {   .action   = ACTION_CART_DETACH_2000,
        .string   = "Unload Cart $2-3***",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_CART_ATTACH_RAW_4000,
        .string   = "Load new Cart $4-5***",
        .type     = MENU_ENTRY_OTHER,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CBM2_GENERIC_C4
    },
    {   .action   = ACTION_CART_DETACH_4000,
        .string   = "Unload Cart $4-5***",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_CART_ATTACH_RAW_6000,
        .string   = "Load new Cart $6-7***",
        .type     = MENU_ENTRY_OTHER,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CBM2_GENERIC_C6
    },
    {   .action   = ACTION_CART_DETACH_6000,
        .string   = "Unload Cart $6-7***",
        .type     = MENU_ENTRY_OTHER,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_CART_DETACH,
        .string   = "Detach cartridge image",
        .type     = MENU_ENTRY_OTHER,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "I/O collision handling ($D800-$DFFF)",
        .type     = MENU_ENTRY_SUBMENU, iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    {   .string   = "Reset on cartridge change",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CartridgeReset_callback
    },
    SDL_MENU_LIST_END
};
