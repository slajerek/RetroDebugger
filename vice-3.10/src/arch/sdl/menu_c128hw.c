/*
 * menu_c128hw.c - C128 HW menu for SDL UI.
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

#include "c128.h"
#include "c128model.h"
#include "cartridge.h"
#include "cia.h"
#include "menu_c64_common_expansions.h"
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
#include "menu_ethernetcart.h"
#endif

#include "uimenu.h"
#include "vdc.h"

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
CIA_MODEL_MENU(2)

/* C128 MODEL SELECTION */

static UI_MENU_CALLBACK(select_c128_model_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        c128model_set(selected);
    } else {
        model = c128model_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

static const ui_menu_entry_t c128_model_menu[] = {
    {   .string   = "C128 (PAL)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_c128_model_callback,
        .data     = (ui_callback_data_t)C128MODEL_C128_PAL
    },
    {   .string   = "C128 D (PAL)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_c128_model_callback,
        .data     = (ui_callback_data_t)C128MODEL_C128D_PAL
    },
    {   .string   = "C128 DCR (PAL)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_c128_model_callback,
        .data     = (ui_callback_data_t)C128MODEL_C128DCR_PAL
    },
    {   .string   = "C128 (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_c128_model_callback,
        .data     = (ui_callback_data_t)C128MODEL_C128_NTSC
    },
    {   .string   = "C128 D (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_c128_model_callback,
        .data     = (ui_callback_data_t)C128MODEL_C128D_NTSC
    },
    {   .string   = "C128 DCR (NTSC)",
        .type     = MENU_ENTRY_OTHER,
        .callback = select_c128_model_callback,
        .data     = (ui_callback_data_t)C128MODEL_C128DCR_NTSC
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(VDC64KB)
UI_MENU_DEFINE_RADIO(VDCRevision)

static const ui_menu_entry_t vdc_menu[] = {
    SDL_MENU_ITEM_TITLE("VDC revision"),
    {   .string   = "Rev 0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VDCRevision_callback,
        .data     = (ui_callback_data_t)VDC_REVISION_0
    },
    {   .string   = "Rev 1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VDCRevision_callback,
        .data     = (ui_callback_data_t)VDC_REVISION_1
    },
    {   .string   = "Rev 2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VDCRevision_callback,
        .data     = (ui_callback_data_t)VDC_REVISION_2
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("VDC memory size"),
    {   .string   = "16KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VDC64KB_callback,
        .data     = (ui_callback_data_t)VDC_16KB
    },
    {   .string   = "64KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VDC64KB_callback,
        .data     = (ui_callback_data_t)VDC_64KB
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(MachineType)

static const ui_menu_entry_t machine_type_menu[] = {
    {   .string   = "International",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_INT
    },
    {   .string   = "Finnish",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_FINNISH
    },
    {   .string   = "French",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_FRENCH
    },
    {   .string   = "German",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_GERMAN
    },
    {   .string   = "Italian",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_ITALIAN
    },
    {   .string   =  "Norwegian",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_NORWEGIAN
    },
    {   .string   = "Swedish",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_SWEDISH
    },
    {   .string   = "Swiss",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineType_callback,
        .data     = (ui_callback_data_t)C128_MACHINE_SWISS
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(IEEE488)
UI_MENU_DEFINE_TOGGLE(C128FullBanks)
UI_MENU_DEFINE_TOGGLE(Go64Mode)

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

const ui_menu_entry_t c128_hardware_menu[] = {
    {   .string   = "Select C128 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)c128_model_menu
    },
    {   .string   = "Select machine type",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)machine_type_menu
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
        .data     = (ui_callback_data_t)joystick_c64_menu
    },
    {   .string   = "SID settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sid_c128_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("CIA models"),
    {   .string   = "CIA 1 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia1_model_submenu
    },
    {   .string   = "CIA 2 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia2_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Power grid frequency",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)power_freq_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VDC settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vdc_menu
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
    {   .string   = "RAM banks 2 and 3",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_C128FullBanks_callback
    },
    {   .string   = "ROM settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c128_rom_menu
    },
    {   .string   = "Switch to C64 mode on reset",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Go64Mode_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Hardware expansions"),
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    {   .string   = "RS232 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rs232_c128_menu
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
        .data     = (ui_callback_data_t)ds12c887rtc_c128_menu
    },
    {   .string   = CARTRIDGE_NAME_IEEE488,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IEEE488_callback
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
    SDL_MENU_LIST_END
};
