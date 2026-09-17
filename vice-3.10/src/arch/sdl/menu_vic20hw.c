/*
 * menu_vic20hw.c - VIC-20 HW menu for SDL UI.
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

#include <stdio.h>

#include "types.h"

#include "cartridge.h"

#include "menu_common.h"
#include "menu_joyport.h"
#include "menu_joystick.h"

#ifdef HAVE_MIDI
#include "menu_midi.h"
#endif

#include "menu_ram.h"
#include "menu_rom.h"

#ifdef HAVE_MOUSE
#include "menu_mouse.h"
#endif

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "menu_rs232.h"
#endif

#include "menu_sid.h"
#include "menu_tape.h"
#include "menu_userport.h"

#ifdef HAVE_RAWNET
#include "menu_ethernet.h"
#include "menu_ethernetcart.h"
#endif

#include "resources.h"
#include "uimenu.h"
#include "vic20model.h"

#include "menu_vic20hw.h"


enum {
    BLOCK_0 = 1,
    BLOCK_1 = 1 << 1,
    BLOCK_2 = 1 << 2,
    BLOCK_3 = 1 << 3,
    BLOCK_5 = 1 << 5
};

static UI_MENU_CALLBACK(custom_memory_callback)
{
    int blocks, value;

    if (activated) {
        blocks = vice_ptr_to_int(param);
        resources_set_int("RAMBlock0", blocks & BLOCK_0 ? 1 : 0);
        resources_set_int("RAMBlock1", blocks & BLOCK_1 ? 1 : 0);
        resources_set_int("RAMBlock2", blocks & BLOCK_2 ? 1 : 0);
        resources_set_int("RAMBlock3", blocks & BLOCK_3 ? 1 : 0);
        resources_set_int("RAMBlock5", blocks & BLOCK_5 ? 1 : 0);
    } else {
        resources_get_int("RAMBlock0", &value);
        blocks = value;
        resources_get_int("RAMBlock1", &value);
        blocks |= (value << 1);
        resources_get_int("RAMBlock2", &value);
        blocks |= (value << 2);
        resources_get_int("RAMBlock3", &value);
        blocks |= (value << 3);
        resources_get_int("RAMBlock5", &value);
        blocks |= (value << 5);

        if (blocks == vice_ptr_to_int(param)) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static const ui_menu_entry_t vic20_memory_common_menu[] = {
    {   .string   = "No expansion",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_memory_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "3KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_memory_callback,
        .data     = (ui_callback_data_t)(BLOCK_0)
    },
    {   .string   = "8KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_memory_callback,
        .data     = (ui_callback_data_t)(BLOCK_1)
    },
    {   .string   = "16KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_memory_callback,
        .data     = (ui_callback_data_t)(BLOCK_1 | BLOCK_2)
    },
    {   .string   = "24KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_memory_callback,
        .data     = (ui_callback_data_t)(BLOCK_1 | BLOCK_2 | BLOCK_3)
    },
    {   .string   = "All",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_memory_callback,
        .data     = (ui_callback_data_t)(BLOCK_0 | BLOCK_1 | BLOCK_2 | BLOCK_3 | BLOCK_5)
    },
    SDL_MENU_LIST_END
};

/* VIC20 MODEL SELECTION */

static UI_MENU_CALLBACK(custom_VIC20Model_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        vic20model_set(selected);
    } else {
        model = vic20model_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

static const ui_menu_entry_t vic20_model_submenu[] = {
    {   .string   = "VIC20 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_VIC20Model_callback,
        .data     = (ui_callback_data_t)VIC20MODEL_VIC20_PAL
    },
    {   .string   = "VIC20 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_VIC20Model_callback,
        .data     = (ui_callback_data_t)VIC20MODEL_VIC20_NTSC
    },
    {   .string   = "VIC21",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_VIC20Model_callback,
        .data     = (ui_callback_data_t)VIC20MODEL_VIC21
    },
    {   .string   = "VIC1001",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_VIC20Model_callback,
        .data     = (ui_callback_data_t)VIC20MODEL_VIC1001
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(RAMBlock0)
UI_MENU_DEFINE_TOGGLE(RAMBlock1)
UI_MENU_DEFINE_TOGGLE(RAMBlock2)
UI_MENU_DEFINE_TOGGLE(RAMBlock3)
UI_MENU_DEFINE_TOGGLE(RAMBlock5)
UI_MENU_DEFINE_TOGGLE(IEEE488)

UI_MENU_DEFINE_TOGGLE(VFLImod)

const ui_menu_entry_t vic20_hardware_menu[] = {
    {   .string   = "Select VIC20 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vic20_model_submenu
    },
    {   .string   = "Joyport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joyport_menu
    },
    {   .string   = "Joystick settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joystick_vic20_menu
    },
#ifdef HAVE_MOUSE
    {   .string   = "Mouse emulation",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mouse_menu
    },
#endif
    {   .string   = "SID cart settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sid_vic_menu
    },
    {   .string   = "RAM pattern settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ram_menu
    },
    {   .string   = "ROM settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c64_vic20_rom_menu
    },
    {   .string   = "IEEE488 interface",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IEEE488_callback
    },
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    {   .string   = "RS232 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rs232_vic20_menu
    },
#endif
#ifdef HAVE_MIDI
    {   .string   = "MIDI settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)midi_vic20_menu
    },
#endif
    {   .string   = "Userport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)userport_menu
    },
    {   .string   = "Tape port devices",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)tapeport_devices_menu
    },
    {   .string   = "VFLI modification",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VFLImod_callback
    },
#ifdef HAVE_RAWNET
    {   .string   = "Ethernet settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernet_menu
    },
    {   .string   = "Ethernet Cart settings (MasC=uerade)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernetcart20_menu
    },
#endif
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory expansions"),
    {   .string   = "Common configurations",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vic20_memory_common_menu
    },
    {   .string   = "Block 0 (3KiB at $0400-$0FFF)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMBlock0_callback
    },
    {   .string   = "Block 1 (8KiB at $2000-$3FFF)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMBlock1_callback
    },
    {   .string   = "Block 2 (8KiB at $4000-$5FFF)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMBlock2_callback
    },
    {   .string   = "Block 3 (8KiB at $6000-$7FFF)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMBlock3_callback
    },
    {   .string   = "Block 5 (8KiB at $A000-$BFFF)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMBlock5_callback
    },
    SDL_MENU_LIST_END
};
