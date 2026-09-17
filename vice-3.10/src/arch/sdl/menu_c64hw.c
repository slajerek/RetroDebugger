/*
 * menu_c64hw.c - C64 HW menu for SDL UI.
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

#include "c64fastiec.h"
#include "cartridge.h"
#include "menu_c64_common_expansions.h"
#include "menu_c64_expansions.h"
#include "menu_c64hw.h"
#include "menu_c64model.h"
#include "menu_common.h"
#include "menu_joyport.h"
#include "menu_joystick.h"

#ifdef HAVE_MIDI
#include "menu_midi.h"
#endif

#ifdef HAVE_MOUSE
#include "menu_mouse.h"
#endif

#include "menu_ram.h"
#include "menu_rom.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "menu_rs232.h"
#endif

#include "menu_sid.h"
#include "menu_tape.h"
#include "menu_userport.h"

#ifdef HAVE_RAWNET
#include "menu_ethernet.h"
#endif

#include "uimenu.h"

#include "tapeport.h"
#include "userport.h"
#include "util.h"

UI_MENU_DEFINE_RADIO(BurstMod)

const ui_menu_entry_t burstmod_menu[] = {
    {   .string   = "None",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_BurstMod_callback,
        .data     = (ui_callback_data_t)BURST_MOD_NONE
    },
    {   .string   = "CIA1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_BurstMod_callback,
        .data     = (ui_callback_data_t)BURST_MOD_CIA1
    },
    {   .string   = "CIA2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_BurstMod_callback,
        .data    = (ui_callback_data_t)BURST_MOD_CIA2
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t c64_hardware_menu_template[] = {
    {   .string   = "Model settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c64_model_menu
    },
    {   .string   = "Joyport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joyport_menu
    },
    {   .string   = "Joystick settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joystick_c64_menu
    },
#ifdef HAVE_MOUSE
    {   .string   = "Mouse emulation",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mouse_menu
    },
#endif
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
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Hardware expansions"),
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    {   .string   = "RS232 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rs232_c64_menu
    },
#endif
    {   .string   = CARTRIDGE_NAME_DIGIMAX " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)digimax_menu
    },
    {   .string   = CARTRIDGE_NAME_DS12C887RTC " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ds12c887rtc_c64_menu
    },
#ifdef HAVE_MIDI
    {   .string   = "MIDI settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)midi_c64_menu
    },
#endif
    {   .string   = "Memory Expansion Hack settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c64_memory_hacks_menu
    },
#ifdef HAVE_RAWNET
    {   .string   = "Ethernet settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernet_menu
    },
#endif
    {   .string   = "Burst Mode Modification",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)burstmod_menu
    },
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

ui_menu_entry_t c64_hardware_menu[sizeof(c64_hardware_menu_template) / sizeof(ui_menu_entry_t)];

const ui_menu_entry_t c64sc_hardware_menu_template[] = {
    {   .string   = "Model settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c64sc_model_menu
    },
    {   .string   = "Joyport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joyport_menu
    },
    {   .string   = "Joystick settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joystick_c64_menu
    },
#ifdef HAVE_MOUSE
    {   .string   = "Mouse emulation",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mouse_menu
    },
#endif
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
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Hardware expansions"),
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    {   .string   = "RS232 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rs232_c64_menu
    },
#endif
    {   .string   = CARTRIDGE_NAME_DIGIMAX " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)digimax_menu
    },
    {   .string   = CARTRIDGE_NAME_DS12C887RTC " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ds12c887rtc_c64_menu
    },
#ifdef HAVE_MIDI
    {   .string   = "MIDI settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)midi_c64_menu
    },
#endif
    {   .string   = "Memory Expansion Hack settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c64_memory_hacks_menu
    },
#ifdef HAVE_RAWNET
    {   .string   = "Ethernet settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernet_menu
    },
#endif
    {   .string   = "Burst Mode Modification",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)burstmod_menu
    },
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

ui_menu_entry_t c64sc_hardware_menu[sizeof(c64sc_hardware_menu_template) / sizeof(ui_menu_entry_t)];

void c64_create_machine_menu(void)
{
    int has_userport = userport_get_active_state();
    int has_tapeport = tapeport_get_active_state();
    int i;
    int j = 0;

    for (i = 0; c64_hardware_menu_template[i].string != NULL; i++) {
        if (!util_strcasecmp(c64_hardware_menu_template[i].string, "Userport settings")) {
            if (has_userport) {
                c64_hardware_menu[j] = c64_hardware_menu_template[i];
                j++;
            }
        } else if (!util_strcasecmp(c64_hardware_menu_template[i].string, "Tape port devices")) {
            if (has_tapeport) {
                c64_hardware_menu[j] = c64_hardware_menu_template[i];
                j++;
            }
        } else {
            c64_hardware_menu[j] = c64_hardware_menu_template[i];
            j++;
        }
    }
    c64_hardware_menu[j] = c64_hardware_menu_template[i];

    j = 0;

    for (i = 0; c64sc_hardware_menu_template[i].string != NULL; i++) {
        if (!util_strcasecmp(c64sc_hardware_menu_template[i].string, "Userport settings")) {
            if (has_userport) {
                c64sc_hardware_menu[j] = c64sc_hardware_menu_template[i];
                j++;
            }
        } else if (!util_strcasecmp(c64_hardware_menu_template[i].string, "Tape port devices")) {
            if (has_tapeport) {
                c64sc_hardware_menu[j] = c64sc_hardware_menu_template[i];
                j++;
            }
        } else {
            c64sc_hardware_menu[j] = c64sc_hardware_menu_template[i];
            j++;
        }
    }
    c64sc_hardware_menu[j] = c64sc_hardware_menu_template[i];
}
