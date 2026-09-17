/** \file   actions-cartridge.c
 * \brief   UI action implementations for cartridge-related dialogs and settings (SDL)
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

#include "cartridge.h"
#include "keyboard.h"
#include "log.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"

#include "actions-cartridge.h"


/** \brief  Detach cartridge action
 *
 * \param[in]   self    action map
 */
static void cart_detach_action(ui_action_map_t *self)
{
    cartridge_detach_image(-1);
}

/** \brief  Detach cartridge from memory address action
 *
 * \param[in]   self    action map
 */
static void cart_detach_address_action(ui_action_map_t *self)
{
    cartridge_detach_image(vice_ptr_to_int(self->data));
}

/** \brief  Trigger freeze action
 *
 * \param[in]   self    action map
 */
static void cart_freeze_action(ui_action_map_t *self)
{
    keyboard_clear_keymatrix();
    cartridge_trigger_freeze();
}


/** \brief  List of mappings for cartridge actions - C64 & C128 */
static const ui_action_map_t cartridge_actions_c64[] = {
    {   .action  = ACTION_CART_ATTACH,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW,
        .handler = sdl_ui_activate_item_action,
    },
    {   .action  = ACTION_CART_DETACH,
        .handler = cart_detach_action
    },
    {   .action  = ACTION_CART_FREEZE,
        .handler = cart_freeze_action
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of mappings for cartridge actions - VIC20 */
static const ui_action_map_t cartridge_actions_vic20[] = {
    {   .action  = ACTION_CART_ATTACH,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW,
        .handler = sdl_ui_activate_item_action,
    },
    {   .action  = ACTION_CART_ATTACH_RAW_2000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true,
    },
    {   .action  = ACTION_CART_ATTACH_RAW_4000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true,
    },
    {   .action  = ACTION_CART_ATTACH_RAW_6000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true,
    },
    {   .action  = ACTION_CART_ATTACH_RAW_A000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true,
    },
    {   .action  = ACTION_CART_ATTACH_RAW_B000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true,
    },
    {   .action  = ACTION_CART_ATTACH_RAW_BEHRBONZ,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_FINAL,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_MEGACART,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_MINIMON,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_ULTIMEM,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_VICFP,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_DETACH,
        .handler = cart_detach_action
    },
    {   .action  = ACTION_CART_FREEZE,
        .handler = cart_freeze_action
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of mappings for cartridge actions - Plus/4 */
static const ui_action_map_t cartridge_actions_plus4[] = {
    {   .action  = ACTION_CART_ATTACH,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_MAGIC,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_MULTI,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_JACINT1MB,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_SPEEDY,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_C1_FULL,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_C1_LOW,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_C1_HIGH,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_C2_FULL,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_C2_LOW,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_C2_HIGH,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_DETACH,
        .handler = cart_detach_action
    },
    {   .action  = ACTION_CART_FREEZE,
        .handler = cart_freeze_action
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  List of mappings for cartridge actions - CBM-II */
static const ui_action_map_t cartridge_actions_cbm2[] = {
    {   .action  = ACTION_CART_ATTACH_RAW_1000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_2000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_4000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_ATTACH_RAW_6000,
        .handler = sdl_ui_activate_item_action,
        .dialog  = true
    },
    {   .action  = ACTION_CART_DETACH_1000,
        .handler = cart_detach_address_action,
        .data    = vice_int_to_ptr(CARTRIDGE_CBM2_GENERIC_C1)
    },
    {   .action  = ACTION_CART_DETACH_2000,
        .handler = cart_detach_address_action,
        .data    = vice_int_to_ptr(CARTRIDGE_CBM2_GENERIC_C2)
    },
    {   .action  = ACTION_CART_DETACH_4000,
        .handler = cart_detach_address_action,
        .data    = vice_int_to_ptr(CARTRIDGE_CBM2_GENERIC_C4)
    },
    {   .action  = ACTION_CART_DETACH_6000,
        .handler = cart_detach_address_action,
        .data    = vice_int_to_ptr(CARTRIDGE_CBM2_GENERIC_C6)
    },
    {   .action  = ACTION_CART_DETACH,
        .handler = cart_detach_action
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register cartridge actions */
void actions_cartridge_register(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:
            ui_actions_register(cartridge_actions_c64);
            break;
        case VICE_MACHINE_VIC20:
            ui_actions_register(cartridge_actions_vic20);
            break;
        case VICE_MACHINE_PLUS4:
            ui_actions_register(cartridge_actions_plus4);
            break;
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
            ui_actions_register(cartridge_actions_cbm2);
            break;
        default:
            /* no cartridge support for the remaining machines */
            break;
    }
}
