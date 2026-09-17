/*
 * menu_ethernetcart.c - Ethernet Cart menu for SDL UI.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#ifdef HAVE_RAWNET

#include <stdio.h>

#include "types.h"

#include "lib.h"
#include "menu_common.h"
#include "menu_ethernetcart.h"
#include "rawnet.h"
#include "resources.h"
#include "uimenu.h"

#include "c64cart.h"

/* Common menus */

UI_MENU_DEFINE_TOGGLE(ETHERNETCART_ACTIVE)
UI_MENU_DEFINE_RADIO(ETHERNETCARTMode)
UI_MENU_DEFINE_RADIO(ETHERNETCARTBase)

static const ui_menu_entry_t ethernetcart_mode_menu[] = {
    {   .string   = CARTRIDGE_NAME_RRNET " compatible",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTMode_callback,
        .data     = (ui_callback_data_t)ETHERNETCART_MODE_RRNET
    },
    {   .string   = CARTRIDGE_NAME_TFE " compatible",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTMode_callback,
        .data     = (ui_callback_data_t)ETHERNETCART_MODE_TFE
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ethernetcart_base64_menu[] = {
    {   .string   = "$DE00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$DE10",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde10
    },
    {   .string   = "$DE20",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde20
    },
    {   .string   = "$DE30",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde30
    },
    {   .string   = "$DE40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde40
    },
    {   .string   = "$DE50",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde50
    },
    {   .string   = "$DE60",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde60
    },
    {   .string   = "$DE70",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde70
    },
    {   .string   = "$DE80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde80
    },
    {   .string   = "$DE90",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xde90
    },
    {   .string   = "$DEA0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdea0
    },
    {   .string   = "$DEB0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdeb0
    },
    {   .string   = "$DEC0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdec0
    },
    {   .string   = "$DED0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xded0
    },
    {   .string   = "$DEE0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdee0
    },
    {   .string   = "$DEF0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdef0
    },
    {   .string   = "$DF00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    {   .string   = "$DF10",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf10
    },
    {   .string   = "$DF20",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf20
    },
    {   .string   = "$DF30",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf30
    },
    {   .string   = "$DF40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf40
    },
    {   .string   = "$DF50",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf50
    },
    {   .string   = "$DF60",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf60
    },
    {   .string   = "$DF70",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf70
    },
    {   .string   = "$DF80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf80
    },
    {   .string   = "$DF90",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdf90
    },
    {   .string   = "$DFA0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdfa0
    },
    {   .string   = "$DFB0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdfb0
    },
    {   .string   = "$DFC0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdfc0
    },
    {   .string   = "$DFD0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdfd0
    },
    {   .string   = "$DFE0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdfe0
    },
    {   .string   = "$DFF0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0xdff0
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ethernetcart_base20_menu[] = {
    {   .string   = "$9800",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9800
    },
    {   .string   = "$9810",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9810
    },
    {   .string   = "$9820",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9820
    },
    {   .string   = "$9830",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9830
    },
    {   .string   = "$9840",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9840
    },
    {   .string   = "$9850",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9850
    },
    {   .string   = "$9860",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9860
    },
    {   .string   = "$9870",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9870
    },
    {   .string   = "$9880",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9880
    },
    {   .string   = "$9890",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9890
    },
    {   .string   = "$98A0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x98a0
    },
    {   .string   = "$98B0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x98b0
    },
    {   .string   = "$98C0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x98c0
    },
    {   .string   = "$98D0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x98d0
    },
    {   .string   = "$98E0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x98e0
    },
    {   .string   = "$98F0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x98f0
    },
    {   .string   = "$9C00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c00
    },
    {   .string   = "$9C10",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c10
    },
    {   .string   = "$9C20",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c20
    },
    {   .string   = "$9C30",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c30
    },
    {   .string   = "$9C40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c40
    },
    {   .string   = "$9C50",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c50
    },
    {   .string   = "$9C60",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c60
    },
    {   .string   = "$9C70",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c70
    },
    {   .string   = "$9C80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c80
    },
    {   .string   = "$9C90",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9c90
    },
    {   .string   = "$9CA0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9ca0
    },
    {   .string   = "$9CB0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9cb0
    },
    {   .string   = "$9CC0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9cc0
    },
    {   .string   = "$9CD0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9cd0
    },
    {   .string   = "$9CE0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9ce0
    },
    {   .string   = "$9CF0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ETHERNETCARTBase_callback,
        .data     = (ui_callback_data_t)0x9cf0
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t ethernetcart_menu[] = {
    {   .string   = "Ethernet Cart emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ETHERNETCART_ACTIVE_callback
    },
    {   .string   = "Ethernet Cart mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)ethernetcart_mode_menu
    },
    {   .string   = "Ethernet Cart base address",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)ethernetcart_base64_menu
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t ethernetcart20_menu[] = {
    {   .string   = CARTRIDGE_NAME_ETHERNETCART " emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ETHERNETCART_ACTIVE_callback
    },
    {   .string   = CARTRIDGE_NAME_ETHERNETCART " mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)ethernetcart_mode_menu
    },
    {   .string   = CARTRIDGE_NAME_ETHERNETCART " base address",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)ethernetcart_base20_menu
    },
    SDL_MENU_LIST_END
};

#endif /* HAVE_RAWNET */
