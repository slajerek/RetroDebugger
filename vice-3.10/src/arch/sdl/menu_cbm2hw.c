/*
 * menu_cbm2hw.c - CBM2 HW menu for SDL UI.
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

#include "cbm2mem.h"
#include "cbm2model.h"
#include "cia.h"
#include "menu_cbm2hw.h"
#include "menu_common.h"
#include "menu_joyport.h"
#include "menu_joystick.h"
#include "menu_ram.h"
#include "menu_rom.h"
#include "menu_userport.h"

#ifdef HAVE_MOUSE
#include "menu_mouse.h"
#endif

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "menu_rs232.h"
#endif

#include "menu_sid.h"
#include "menu_tape.h"
#include "uimenu.h"

UI_MENU_DEFINE_RADIO(RamSize)
UI_MENU_DEFINE_TOGGLE(Ram08)
UI_MENU_DEFINE_TOGGLE(Ram1)
UI_MENU_DEFINE_TOGGLE(Ram2)
UI_MENU_DEFINE_TOGGLE(Ram4)
UI_MENU_DEFINE_TOGGLE(Ram6)
UI_MENU_DEFINE_TOGGLE(RamC)

#define CIA_MODEL_MENU(xyz)                                     \
    UI_MENU_DEFINE_RADIO(CIA##xyz##Model)                       \
    static const ui_menu_entry_t cia##xyz##_model_submenu[] = { \
        {   .string   = "6526  (old)",                          \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
            .callback = radio_CIA##xyz##Model_callback,         \
            .data     = (ui_callback_data_t)CIA_MODEL_6526      \
        },                                                      \
        {   .string   = "8521 (new)",                           \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
            .callback = radio_CIA##xyz##Model_callback,         \
            .data     = (ui_callback_data_t)CIA_MODEL_6526A     \
        },                                                      \
        SDL_MENU_LIST_END                                       \
    };

CIA_MODEL_MENU(1)

/* CBM2 MODEL SELECTION */

static UI_MENU_CALLBACK(select_cbm2_model_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        cbm2model_set(selected);
    } else {
        model = cbm2model_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

UI_MENU_DEFINE_RADIO(MachinePowerFrequency)

static const ui_menu_entry_t power_freq_submenu[] = {
    {   .string   = "50Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachinePowerFrequency_callback,
        .data     = (ui_callback_data_t)50
    },
    {   .string   = "60Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachinePowerFrequency_callback,
        .data     = (ui_callback_data_t)60
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t cbm2_model_menu[] = {
    {   .string   = "CBM 610 (PAL)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_610_PAL
    },
    {   .string   = "CBM 610 (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_610_NTSC
    },
    {   .string   = "CBM 620 (PAL)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_620_PAL
    },
    {   .string   = "CBM 620 (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_620_NTSC
    },
    {   .string   = "CBM 620+ (PAL)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_620PLUS_PAL
    },
    {   .string   = "CBM 620+ (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_620PLUS_NTSC
    },
    {   .string   = "CBM 710 (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_710_NTSC
    },
    {   .string   = "CBM 720 (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_720_NTSC
    },
    {   .string   = "CBM 720+ (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_cbm2_model_callback,
        .data     = (ui_callback_data_t)CBM2MODEL_720PLUS_NTSC
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t cbm2_memory_menu[] = {
    SDL_MENU_ITEM_TITLE("CBM2 memory size"),
    {   .string   = "128KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)256
    },
    {   .string   = "512KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)512
    },
    {   .string   = "1MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)1024
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("CBM2 memory blocks"),
    {   .string   = "RAM at $0800-$0FFF",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Ram08_callback
    },
    {   .string   = "RAM at $1000-$1FFF",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Ram1_callback
    },
    {   .string   = "RAM at $2000-$3FFF",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Ram2_callback
    },
    {   .string   = "RAM at $4000-$5FFF",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Ram4_callback
    },
    {   .string   = "RAM at $6000-$7FFF",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Ram6_callback
    },
    {   .string   = "RAM at $C000-$CFFF",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RamC_callback
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t cbm6x0_7x0_hardware_menu[] = {
    {   .string   = "Select CBM2 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cbm2_model_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Joyport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joyport_menu
    },
    {   .string   = "Joystick settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joystick_userport_only_menu
    },
#ifdef HAVE_MOUSE
    {   .string   = "Mouse emulation",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mouse_grab_menu
    },
#endif
    {   .string   = "SID settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sid_cbm2_menu
    },
    {   .string   = "CIA model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia1_model_submenu
    },
    {   .string   = "Power grid frequency",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)power_freq_submenu
    },
    {   .string   = "RAM pattern settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ram_menu
    },
    {   .string   = "ROM settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)cbm2_rom_menu
    },
    {   .string   = "CBM2 memory setting",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)cbm2_memory_menu
    },
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    {   .string   = "RS232 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rs232_nouser_menu
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
    SDL_MENU_LIST_END
};
