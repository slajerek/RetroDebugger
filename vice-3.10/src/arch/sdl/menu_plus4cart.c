/*
 * menu_plus4cart.c - Implementation of the plus4 cartridge settings menu for the SDL UI.
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
#include "menu_common.h"
#include "menu_plus4cart.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"

#include "plus4cart.h"


static UI_MENU_CALLBACK(attach_cart_callback)
{
    char *name = NULL;
    int type = vice_ptr_to_int(param);

    if (activated) {
        int action = ACTION_NONE;
        name = sdl_ui_file_selection_dialog("Select cartridge image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (cartridge_attach_image(type, name) < 0) {
                ui_error("Cannot load cartridge image.");
            }
            lib_free(name);
        }
        /* mark the appropriate UI action finished
         * TODO: there has to be a more elegant way than this ;) */
        switch (type) {
            case CARTRIDGE_CRT:
                action = ACTION_CART_ATTACH;
                break;
            case CARTRIDGE_PLUS4_MAGIC:
                action = ACTION_CART_ATTACH_RAW_MAGIC;
                break;
            case CARTRIDGE_PLUS4_MULTI:
                action = ACTION_CART_ATTACH_RAW_MULTI;
                break;
            case CARTRIDGE_PLUS4_JACINT1MB:
                action = ACTION_CART_ATTACH_RAW_JACINT1MB;
                break;
            case CARTRIDGE_PLUS4_SPEEDY:
                action = ACTION_CART_ATTACH_RAW_SPEEDY;
                break;
            case CARTRIDGE_PLUS4_GENERIC_C1:
                action = ACTION_CART_ATTACH_RAW_C1_FULL;
                break;
            case CARTRIDGE_PLUS4_GENERIC_C1LO:
                action = ACTION_CART_ATTACH_RAW_C1_LOW;
                break;
            case CARTRIDGE_PLUS4_GENERIC_C1HI:
                action = ACTION_CART_ATTACH_RAW_C1_HIGH;
                break;
            case CARTRIDGE_PLUS4_GENERIC_C2:
                action = ACTION_CART_ATTACH_RAW_C2_FULL;
                break;
            case CARTRIDGE_PLUS4_GENERIC_C2LO:
                action = ACTION_CART_ATTACH_RAW_C2_LOW;
                break;
            case CARTRIDGE_PLUS4_GENERIC_C2HI:
                action = ACTION_CART_ATTACH_RAW_C2_HIGH;
                break;
            default:
                break;
        }
        if (action > ACTION_NONE) {
            ui_action_finish(action);
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

UI_MENU_DEFINE_TOGGLE(CartridgeReset)

const ui_menu_entry_t plus4cart_menu[] = {
    {   .action   = ACTION_CART_ATTACH,
        .string   = "Attach CRT image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CRT
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_CART_ATTACH_RAW_MAGIC,
        .string   = "Attach raw " CARTRIDGE_PLUS4_NAME_MAGIC " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_MAGIC
    },
    {   .action   = ACTION_CART_ATTACH_RAW_MULTI,
        .string   = "Attach raw " CARTRIDGE_PLUS4_NAME_MULTI " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_MULTI
    },
    {   .action   = ACTION_CART_ATTACH_RAW_JACINT1MB,
        .string   = "Attach raw " CARTRIDGE_PLUS4_NAME_JACINT1MB " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_JACINT1MB
    },
    {   .action   = ACTION_CART_ATTACH_RAW_SPEEDY,
        .string   = "Attach raw " CARTRIDGE_PLUS4_NAME_SPEEDY " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_SPEEDY
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_CART_ATTACH_RAW_C1_FULL,
        .string   = "Attach full C1 image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_GENERIC_C1
    },
    {   .action   = ACTION_CART_ATTACH_RAW_C1_LOW,
        .string   = "Attach C1 low image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_GENERIC_C1LO
    },
    {   .action   = ACTION_CART_ATTACH_RAW_C1_HIGH,
        .string   = "Attach C1 high image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_GENERIC_C1HI
    },
    {   .action   = ACTION_CART_ATTACH_RAW_C2_FULL,
        .string   = "Attach full C2 image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_GENERIC_C2
    },
    {   .action   = ACTION_CART_ATTACH_RAW_C2_LOW,
        .string   = "Attach C2 low image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_GENERIC_C2LO
    },
    {   .action   = ACTION_CART_ATTACH_RAW_C2_HIGH,
        .string   = "Attach C2 high image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_PLUS4_GENERIC_C2HI
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_CART_DETACH,
        .string   = "Detach cartridge image",
        .type     = MENU_ENTRY_OTHER,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "I/O collision handling ($FD00-$FEFF)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    {   .string   = "Reset on cartridge change",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CartridgeReset_callback
    },
    SDL_MENU_LIST_END
};
