/*
 * menu_plus4hw.c - PLUS4 HW menu for SDL UI.
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
#include "menu_joyport.h"
#include "menu_joystick.h"
#include "menu_plus4hw.h"
#include "menu_ram.h"
#include "menu_rom.h"
#include "menu_userport.h"
#include "plus4memhacks.h"
#include "plus4model.h"

#ifdef HAVE_MOUSE
#include "menu_mouse.h"
#endif

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "menu_rs232.h"
#endif

#include "menu_sid.h"
#include "menu_tape.h"
#include "uiactions.h"
#include "uimenu.h"
#include "userport.h"
#include "util.h"

/* PLUS4 MODEL SELECTION */

static UI_MENU_CALLBACK(custom_PLUS4Model_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        plus4model_set(selected);
        plus4_create_machine_menu();
    } else {
        model = plus4model_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

static const ui_menu_entry_t plus4_model_submenu[] = {
    {   .string   = "C16 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PLUS4Model_callback,
        .data     = (ui_callback_data_t)PLUS4MODEL_C16_PAL
    },
    {   .string   = "C16 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PLUS4Model_callback,
        .data     = (ui_callback_data_t)PLUS4MODEL_C16_NTSC
    },
    {   .string   = "Plus4 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PLUS4Model_callback,
        .data     = (ui_callback_data_t)PLUS4MODEL_PLUS4_PAL
    },
    {   .string   = "Plus4 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PLUS4Model_callback,
        .data     = (ui_callback_data_t)PLUS4MODEL_PLUS4_NTSC
    },
    {   .string   = "V364 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PLUS4Model_callback,
        .data     = (ui_callback_data_t)PLUS4MODEL_V364_NTSC
    },
    {   .string   = "C232 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PLUS4Model_callback,
        .data     = (ui_callback_data_t)PLUS4MODEL_232_NTSC
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(SpeechEnabled)
UI_MENU_DEFINE_FILE_STRING(SpeechImage)

static const ui_menu_entry_t v364speech_menu[] = {
    {   .string   = "Enable V364 Speech",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SpeechEnabled_callback
    },
    {   .string   = "ROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_SpeechImage_callback,
        .data     = (ui_callback_data_t)"Select Speech ROM image"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(MemoryHack)
UI_MENU_DEFINE_RADIO(RamSize)
UI_MENU_DEFINE_TOGGLE(Acia1Enable)

const ui_menu_entry_t plus4_hardware_menu_template[] = {
    {   .string   = "Select Plus4 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)plus4_model_submenu
    },
    {   .string   = "Joyport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joyport_menu
    },
    {   .string   = "Joystick settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joystick_plus4_menu
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
        .data     = (ui_callback_data_t)sid_plus4_menu
    },
    {   .string   = "V364 Speech settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)v364speech_menu
    },
    {   .string   = "RAM pattern settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ram_menu
    },
    {   .string   = "ROM settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)plus4_rom_menu
    },
    {   .string   = "ACIA installed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Acia1Enable_callback
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
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory"),
    {   .string   = "16KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)16
    },
    {   .string   = "32KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)32
    },
    {   .string   = "64KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)64
    },
    SDL_MENU_ITEM_TITLE("Memory expansion hack"),
    {   .string   = "None",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MemoryHack_callback,
        .data     = (ui_callback_data_t)MEMORY_HACK_NONE
    },
    {   .string   = "256KiB (CSORY)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MemoryHack_callback,
        .data     = (ui_callback_data_t)MEMORY_HACK_C256K
    },
    {   .string   = "256KiB (HANNES)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MemoryHack_callback,
        .data     = (ui_callback_data_t)MEMORY_HACK_H256K
    },
    {   .string   = "1MiB (HANNES)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MemoryHack_callback,
        .data     = (ui_callback_data_t)MEMORY_HACK_H1024K
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t plus4_hardware_menu[sizeof(plus4_hardware_menu_template) / sizeof(ui_menu_entry_t)];

void plus4_create_machine_menu(void)
{
    int has_userport = userport_get_active_state();
    int i;
    int j = 0;

    for (i = 0; plus4_hardware_menu_template[i].string != NULL; i++) {
        if (!util_strcasecmp(plus4_hardware_menu_template[i].string, "Userport settings")) {
            if (has_userport) {
                plus4_hardware_menu[j].action   = ACTION_NONE;
                plus4_hardware_menu[j].string   = plus4_hardware_menu_template[i].string;
                plus4_hardware_menu[j].type     = plus4_hardware_menu_template[i].type;
                plus4_hardware_menu[j].callback = plus4_hardware_menu_template[i].callback;
                plus4_hardware_menu[j].data     = plus4_hardware_menu_template[i].data;
                j++;
            }
        } else {
            plus4_hardware_menu[j].action   = ACTION_NONE;
            plus4_hardware_menu[j].string   = plus4_hardware_menu_template[i].string;
            plus4_hardware_menu[j].type     = plus4_hardware_menu_template[i].type;
            plus4_hardware_menu[j].callback = plus4_hardware_menu_template[i].callback;
            plus4_hardware_menu[j].data     = plus4_hardware_menu_template[i].data;
            j++;
        }
    }
    plus4_hardware_menu[j].string = plus4_hardware_menu_template[i].string;
}
