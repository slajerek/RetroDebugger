/*
 * menu_scpu64hw.c - SCPU64 HW menu for SDL UI.
 *
 * Written by
 *  Marco van den Heuevel <blackystardust68@yahoo.com>
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
#include "menu_c64model.h"
#include "menu_common.h"
#include "menu_joyport.h"
#include "menu_joystick.h"
#include "menu_scpu64hw.h"

#ifdef HAVE_MIDI
#include "menu_midi.h"
#endif

#ifdef HAVE_MOUSE
#include "menu_mouse.h"
#endif

#include "menu_ram.h"
#include "menu_rom.h"
#include "menu_userport.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "menu_rs232.h"
#endif

#include "menu_sid.h"

#ifdef HAVE_RAWNET
#include "menu_ethernet.h"
#include "menu_ethernetcart.h"
#endif

#include "uiactions.h"
#include "uimenu.h"
#include "userport.h"
#include "util.h"

UI_MENU_DEFINE_RADIO(BurstMod)

const ui_menu_entry_t scpu64_burstmod_menu[] = {
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
        .data     = (ui_callback_data_t)BURST_MOD_CIA2
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(SIMMSize)

const ui_menu_entry_t scpu64_simmsize_menu[] = {
    {   .string   = "0 MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SIMMSize_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1 MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SIMMSize_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "4 MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SIMMSize_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "8 MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SIMMSize_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "16 MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SIMMSize_callback,
        .data     = (ui_callback_data_t)16
    },
    SDL_MENU_LIST_END
};


const ui_menu_entry_t scpu64_hardware_menu_template[] = {
    {   .string   = "Model settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)scpu64_model_menu
    },
    {   .string   = "SIMM size",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)scpu64_simmsize_menu
    },
    {   .action   = ACTION_SCPU_JIFFY_SWITCH_TOGGLE,
        .string   = "Jiffy switch enable",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "JiffySwitch"
    },
    {   .action   = ACTION_SCPU_SPEED_SWITCH_TOGGLE,
        .string   = "Speed switch enable",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "SpeedSwitch"
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
        .data     = (ui_callback_data_t)scpu64_rom_menu
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
#ifdef HAVE_RAWNET
    {   .string   = "Ethernet settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernet_menu
    },
    {   .string   = CARTRIDGE_NAME_ETHERNETCART " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernetcart_menu
    },
#endif
    {   .string   = "Burst Mode Modification",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)scpu64_burstmod_menu
    },
    {   .string   = "Userport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)userport_menu
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t scpu64_hardware_menu[sizeof(scpu64_hardware_menu_template) / sizeof(ui_menu_entry_t)];

void scpu64_create_machine_menu(void)
{
    int has_userport = userport_get_active_state();
    int i;
    int j = 0;

    for (i = 0; scpu64_hardware_menu_template[i].string != NULL; i++) {
        if (!util_strcasecmp(scpu64_hardware_menu_template[i].string, "Userport settings")) {
            if (has_userport) {
                scpu64_hardware_menu[j] = scpu64_hardware_menu_template[i];
                j++;
            }
        } else {
            scpu64_hardware_menu[j] = scpu64_hardware_menu_template[i];
            j++;
        }
    }
    scpu64_hardware_menu[j].string = scpu64_hardware_menu_template[i].string;
}
