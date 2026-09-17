/*
 * menu_c64dtvhw.c - C64DTV HW menu for SDL UI.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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
#include "types.h"
#include <stddef.h>

#include "c64dtv-resources.h"
#include "c64dtvmodel.h"
#include "menu_common.h"
#ifdef HAVE_MOUSE
#include "menu_mouse.h"
#endif
#include "menu_joyport.h"
#include "menu_joystick.h"
#include "menu_ram.h"
#include "menu_rom.h"
#include "menu_sid.h"
#include "menu_userport.h"
#include "uimenu.h"

/* DTV MODEL SELECTION */

static UI_MENU_CALLBACK(custom_DTVModel_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        dtvmodel_set(selected);
    } else {
        model = dtvmodel_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

UI_MENU_DEFINE_TOGGLE(VICIINewLuminances)

static const ui_menu_entry_t dtv_model_submenu[] = {
    {   .string   = "DTV2 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_DTVModel_callback,
        .data     = (ui_callback_data_t)DTVMODEL_V2_PAL
    },
    {   .string   = "DTV2 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_DTVModel_callback,
        .data     = (ui_callback_data_t)DTVMODEL_V2_NTSC
    },
    {   .string   = "DTV3 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_DTVModel_callback,
        .data     = (ui_callback_data_t)DTVMODEL_V3_PAL
    },
    {   .string   = "DTV3 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_DTVModel_callback,
        .data     = (ui_callback_data_t)DTVMODEL_V3_NTSC
    },
    {   .string   = "Hummer PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_DTVModel_callback,
        .data     = (ui_callback_data_t)DTVMODEL_HUMMER_PAL
    },
    {   .string   = "Hummer NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_DTVModel_callback,
        .data     = (ui_callback_data_t)DTVMODEL_HUMMER_NTSC
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(HummerADC)

UI_MENU_DEFINE_FILE_STRING(DTVNTSCV2FlashName)
UI_MENU_DEFINE_FILE_STRING(DTVPALV2FlashName)
UI_MENU_DEFINE_FILE_STRING(DTVNTSCV3FlashName)
UI_MENU_DEFINE_FILE_STRING(DTVPALV3FlashName)
UI_MENU_DEFINE_FILE_STRING(DTVPALHummerFlashName)
UI_MENU_DEFINE_FILE_STRING(DTVNTSCHummerFlashName)

UI_MENU_DEFINE_TOGGLE(c64dtvromrw)
UI_MENU_DEFINE_TOGGLE(FlashTrueFS)
UI_MENU_DEFINE_RADIO(DtvRevision)

const ui_menu_entry_t c64dtv_hardware_menu[] = {
    {   .string   = "Select DTV model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)dtv_model_submenu
    },
    {   .string   = "Joyport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joyport_menu
    },
    {   .string   = "Joystick settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)joystick_c64dtv_menu
    },
    {   .string   = "Userport settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)userport_menu
    },
#ifdef HAVE_MOUSE
    {   .string   = "Mouse emulation",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mouse_c64dtv_menu
    },
#endif
    {   .string   = "SID settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sid_dtv_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("C64DTV Flash ROM images"),
    {   .string   = "NTSC v2",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DTVNTSCV2FlashName_callback,
        .data     = (ui_callback_data_t)"Select C64DTV NTSC v2 ROM image file"
    },
    {   .string   = "PAL v2",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DTVPALV2FlashName_callback,
        .data     = (ui_callback_data_t)"Select C64DTV PAL v2 ROM image file"
    },
    {   .string   = "NTSC v3",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DTVNTSCV3FlashName_callback,
        .data     = (ui_callback_data_t)"Select C64DTV NTSC v3 ROM image file"
    },
    {   .string   = "PAL v3",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DTVPALV3FlashName_callback,
        .data     = (ui_callback_data_t)"Select C64DTV PAL v3 ROM image file"
    },
    {   .string   = "PAL Hummer",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DTVPALHummerFlashName_callback,
        .data     = (ui_callback_data_t)"Select C64DTV PAL Hummer ROM image file"
    },
    {   .string   = "NTSC Hummer",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DTVNTSCHummerFlashName_callback,
        .data     = (ui_callback_data_t)"Select C64DTV NTSC Hummer ROM image file"
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable writes",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_c64dtvromrw_callback
    },
    {   .string   = "True flash filesystem",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_FlashTrueFS_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("DTV revision"),
    {   .string   = "DTV2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DtvRevision_callback,
        .data     = (ui_callback_data_t)DTVREV_2
    },
    {   .string   = "DTV3",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DtvRevision_callback,
        .data     = (ui_callback_data_t)DTVREV_3
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Hummer ADC",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_HummerADC_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Colorfix",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIINewLuminances_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "RAM pattern settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ram_menu
    },
    {   .string   = "Fallback ROM settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)c64dtv_rom_menu
    },
    SDL_MENU_LIST_END
};
