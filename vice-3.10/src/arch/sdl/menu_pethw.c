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

#include "machine.h"
#include "menu_common.h"
#include "menu_joyport.h"
#include "menu_joystick.h"
#include "menu_mouse.h"
#include "menu_ram.h"
#include "menu_rom.h"
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "menu_rs232.h"
#endif
#include "menu_settings.h"
#include "menu_sid.h"
#include "menu_userport.h"
#include "menu_tape.h"
#include "pet.h"
#include "pet-resources.h"
#include "petmodel.h"
#include "pets.h"
#include "types.h"
#include "uimenu.h"

/* PET VIDEO SETTINGS */

/* PET MEMORY SETTINGS */

UI_MENU_DEFINE_RADIO(RamSize)
UI_MENU_DEFINE_RADIO(IOSize)
UI_MENU_DEFINE_TOGGLE(SuperPET)
UI_MENU_DEFINE_TOGGLE(Ram9)
UI_MENU_DEFINE_TOGGLE(RamA)

static const ui_menu_entry_t pet_memory_menu[] = {
    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "4KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "8KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)8
    },
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
    {   .string   = "96KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)96
    },
    {   .string   = "128KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RamSize_callback,
        .data     = (ui_callback_data_t)128
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("I/O size"),
    {   .string   = "256 bytes",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOSize_callback,
        .data     = (ui_callback_data_t)256
    },
    {   .string   = "2KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOSize_callback,
        .data     = (ui_callback_data_t)2048
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("8296 memory blocks"),
    {   .string   = "$9xxx as RAM",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Ram9_callback,
    },
    {   .string   = "$Axxx as RAM",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RamA_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("SUPERPET I/O"),
    {   .string   = "Enable SUPERPET I/O (disables 8x96)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SuperPET_callback,
    },
    SDL_MENU_LIST_END
};

/* PETREU */

UI_MENU_DEFINE_TOGGLE(PETREU)
UI_MENU_DEFINE_RADIO(PETREUsize)
UI_MENU_DEFINE_FILE_STRING(PETREUfilename)

static const ui_menu_entry_t petreu_menu[] = {
    {   .string   = "Enable PET REU",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_PETREU_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "128KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETREUsize_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "512KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETREUsize_callback,
        .data     = (ui_callback_data_t)512
    },
    {   .string   = "1MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETREUsize_callback,
        .data     = (ui_callback_data_t)1024
    },
    {   .string   = "2MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETREUsize_callback,
        .data     = (ui_callback_data_t)2048
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "PET REU image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_PETREUfilename_callback,
        .data     = (ui_callback_data_t)"Select PET REU image"
    },
    SDL_MENU_LIST_END
};

/* PETDWW */

UI_MENU_DEFINE_TOGGLE(PETDWW)
UI_MENU_DEFINE_FILE_STRING(PETDWWfilename)

static const ui_menu_entry_t petdww_menu[] = {
    {   .string   = "Enable PET DWW",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_PETDWW_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "PET DWW image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_PETDWWfilename_callback,
        .data     = (ui_callback_data_t)"Select PET DWW image"
    },
    SDL_MENU_LIST_END
};

/* PETCOLOUR */

UI_MENU_DEFINE_RADIO(PETColour)
UI_MENU_DEFINE_SLIDER(PETColourBG, 0, 255)

static const ui_menu_entry_t petcolour_menu[] = {
    SDL_MENU_ITEM_TITLE("PET Colour type"),
    {   .string   = "Off",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETColour_callback,
        .data     = (ui_callback_data_t)PET_COLOUR_TYPE_OFF
    },
    {   .string   = "RGBI",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETColour_callback,
        .data     = (ui_callback_data_t)PET_COLOUR_TYPE_RGBI
    },
    {   .string   = "Analog",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PETColour_callback,
        .data     = (ui_callback_data_t)PET_COLOUR_TYPE_ANALOG
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "PET Colour background",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = slider_PETColourBG_callback,
        .data     = (ui_callback_data_t)"Set PET Colour background (0-255)"
    },
    SDL_MENU_LIST_END
};

/* SUPERPET CPU */

UI_MENU_DEFINE_RADIO(CPUswitch)

static const ui_menu_entry_t superpet_cpu_menu[] = {
    SDL_MENU_ITEM_TITLE("SuperPET CPU switch"),
    {   .string   = "MOS 6502",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_CPUswitch_callback,
        .data     = (ui_callback_data_t)SUPERPET_CPU_6502
    },
    {   .string   = "Motorola 6809",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_CPUswitch_callback,
        .data     = (ui_callback_data_t)SUPERPET_CPU_6809
    },
    {   .string   = "Programmable",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_CPUswitch_callback,
        .data     = (ui_callback_data_t)SUPERPET_CPU_PROG
    },
    SDL_MENU_LIST_END
};

/* PET MODEL SELECTION */

static UI_MENU_CALLBACK(custom_PETModel_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        petmodel_set(selected);
    } else {
        model = petmodel_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

static const ui_menu_entry_t pet_model_menu[] = {
    {   .string   = "2001",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_2001
    },
    {   .string   = "3008",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_3008
    },
    {   .string   = "3016",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_3016
    },
    {   .string   = "3032",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_3032
    },
    {   .string   = "3032B",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_3032B
    },
    {   .string   = "4016",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_4016
    },
    {   .string   = "4032",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_4032
    },
    {   .string   = "4032B",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_4032B
    },
    {   .string   = "8032",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_8032
    },
    {   .string   = "8096",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_8096
    },
    {   .string   = "8296",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_8296
    },
    {   .string   = "Super PET",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_PETModel_callback,
        .data     = (ui_callback_data_t)PETMODEL_SUPERPET
    },
    SDL_MENU_LIST_END
};

/* FIXME */
#if 0
void uikeyboard_update_pet_type_menu(void)
{
    int idx, type, mapping;

    resources_get_int("KeymapIndex", &idx);
    resources_get_int("KeyboardMapping", &mapping);
}
#endif

static UI_MENU_CALLBACK(radio_KeyboardType_callback)
{
    const char *res = sdl_ui_menu_radio_helper(activated, param, "KeyboardType");
    if (activated) {
        uikeyboard_update_index_menu();
        uikeyboard_update_mapping_menu();
    }
    return res;
}

/* FIXME: this should be dynamic/generated */
static const ui_menu_entry_t pet_keyboard_menu[] = {
    {   .string   = "Business (UK)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeyboardType_callback,
        .data     = (ui_callback_data_t)KBD_TYPE_BUSINESS_UK
    },
    {   .string   = "Business (US)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeyboardType_callback,
        .data     = (ui_callback_data_t)KBD_TYPE_BUSINESS_US
    },
    {   .string   = "Business (DE)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeyboardType_callback,
        .data     = (ui_callback_data_t)KBD_TYPE_BUSINESS_DE
    },
    {   .string   = "Business (JP)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeyboardType_callback,
        .data     = (ui_callback_data_t)KBD_TYPE_BUSINESS_JP
    },
    {   .string   = "Graphics (US)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeyboardType_callback,
        .data     = (ui_callback_data_t)KBD_TYPE_GRAPHICS_US
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(Crtc)
UI_MENU_DEFINE_TOGGLE(PETHRE)

const ui_menu_entry_t pet_hardware_menu[] = {
    {   .string   = "Select PET model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)pet_model_menu
    },
    {   .string   = "Keyboard",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)pet_keyboard_menu
    },
    {   .string   = "SuperPET CPU switch",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)superpet_cpu_menu
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
    {   .string   = "SID cart settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sid_pet_menu
    },
    {   .string   = "RAM pattern settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ram_menu
    },
    {   .string   = "ROM settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)pet_rom_menu
    },
    {   .string   = "PET REU settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)petreu_menu
    },
    {   .string   = "PET DWW settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)petdww_menu
    },
    {   .string   = "PET Colour settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)petcolour_menu
    },
    {   .string   = "Enable PET High Res Emulation board",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_PETHRE_callback
    },
    {   .string   = "Userport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)userport_menu
    },
    {   .string   = "Tape port devices",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)tapeport_pet_devices_menu
    },
    {   .string   = "Memory and I/O settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)pet_memory_menu
    },
    {   .string   = "CRTC chip enable",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Crtc_callback
    },
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    {   .string   = "RS232 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rs232_nouser_menu
    },
#endif
    SDL_MENU_LIST_END
};
