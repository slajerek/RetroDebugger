/*
 * menu_c64_common_expansions.c - C64/C128 expansions menu for SDL UI.
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

#include "cartridge.h"
#include "clockport.h"
#include "ide64.h"
#include "menu_c64_common_expansions.h"
#include "menu_common.h"
#include "uiactions.h"
#include "uimenu.h"


/* DIGIMAX MENU */

UI_MENU_DEFINE_TOGGLE(DIGIMAX)
UI_MENU_DEFINE_RADIO(DIGIMAXbase)

const ui_menu_entry_t digimax_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_DIGIMAX,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DIGIMAX_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Base address"),
    {   .string   = "$DE00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$DE20",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde20
    },
    {   .string   = "$DE40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde40
    },
    {   .string   = "$DE60",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde60
    },
    {   .string   = "$DE80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde80
    },
    {   .string   = "$DEA0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdea0
    },
    {   .string   = "$DEC0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdec0
    },
    {   .string   = "$DEE0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdee0
    },
    {   .string   = "$DF00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    {   .string   = "$DF20",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdf20
    },
    {   .string   = "$DF40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdf40
    },
    {   .string   = "$DF60",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdf60
    },
    {   .string   = "$DF80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdf80
    },
    {   .string   = "$DFA0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdfa0
    },
    {   .string   = "$DFC0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdfc0
    },
    {   .string   = "$DFE0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xdfe0
    },
    SDL_MENU_LIST_END
};


/* DS12C887 RTC MENU */

UI_MENU_DEFINE_TOGGLE(DS12C887RTC)
UI_MENU_DEFINE_TOGGLE(DS12C887RTCRunMode)
UI_MENU_DEFINE_RADIO(DS12C887RTCbase)
UI_MENU_DEFINE_TOGGLE(DS12C887RTCSave)

const ui_menu_entry_t ds12c887rtc_c64_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_DS12C887RTC,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTC_callback
    },
    {   .string   = "Start with running oscillator",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTCRunMode_callback
    },
    {   .string   = "Save RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTCSave_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Base address"),
    {   .string   = "$D500",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xd500
    },
    {   .string   = "$D600",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xd600
    },
    {   .string   = "$D700",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xd700
    },
    {   .string   = "$DE00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$DF00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    SDL_MENU_LIST_END
};


const ui_menu_entry_t ds12c887rtc_c128_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_DS12C887RTC,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTC_callback
    },
    {   .string   = "Start with running oscillator",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTCRunMode_callback
    },
    {   .string   = "Save RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTCSave_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Base address"),
    {   .string   = "$D700",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xd700
    },
    {   .string   = "$DE00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$DF00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    SDL_MENU_LIST_END
};


/* IDE64 CART MENU */

UI_MENU_DEFINE_RADIO(IDE64ClockPort)

static ui_menu_entry_t ide64_clockport_device_menu[CLOCKPORT_MAX_ENTRIES + 1];

UI_MENU_DEFINE_TOGGLE(IDE64RTCSave)
UI_MENU_DEFINE_RADIO(IDE64version)
UI_MENU_DEFINE_TOGGLE(IDE64USBServer)
UI_MENU_DEFINE_STRING(IDE64USBServerAddress)
UI_MENU_DEFINE_FILE_STRING(IDE64Image1)
UI_MENU_DEFINE_FILE_STRING(IDE64Image2)
UI_MENU_DEFINE_FILE_STRING(IDE64Image3)
UI_MENU_DEFINE_FILE_STRING(IDE64Image4)
UI_MENU_DEFINE_TOGGLE(IDE64AutodetectSize1)
UI_MENU_DEFINE_TOGGLE(IDE64AutodetectSize2)
UI_MENU_DEFINE_TOGGLE(IDE64AutodetectSize3)
UI_MENU_DEFINE_TOGGLE(IDE64AutodetectSize4)
UI_MENU_DEFINE_INT(IDE64Cylinders1)
UI_MENU_DEFINE_INT(IDE64Cylinders2)
UI_MENU_DEFINE_INT(IDE64Cylinders3)
UI_MENU_DEFINE_INT(IDE64Cylinders4)
UI_MENU_DEFINE_INT(IDE64Heads1)
UI_MENU_DEFINE_INT(IDE64Heads2)
UI_MENU_DEFINE_INT(IDE64Heads3)
UI_MENU_DEFINE_INT(IDE64Heads4)
UI_MENU_DEFINE_INT(IDE64Sectors1)
UI_MENU_DEFINE_INT(IDE64Sectors2)
UI_MENU_DEFINE_INT(IDE64Sectors3)
UI_MENU_DEFINE_INT(IDE64Sectors4)

static const ui_menu_entry_t ide64_menu_HD_1[] = {
    SDL_MENU_ITEM_TITLE("ATA device 1 settings"),
    {   .string   = "Device 1 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_IDE64Image1_callback,
        .data     = (ui_callback_data_t)"Select Device 1 image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Device 1 geometry"),
    {   .string   = "Autodetect geometry",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IDE64AutodetectSize1_callback
    },
    {   .string   = "Cylinders",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Cylinders1_callback,
        .data     = (ui_callback_data_t)"Enter amount of cylinders (1-65535)"
    },
    {   .string   = "Heads",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Heads1_callback,
        .data     = (ui_callback_data_t)"Enter amount of heads (1-16)"
    },
    {   .string   = "Sectors",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Sectors1_callback,
        .data     = (ui_callback_data_t)"Enter amount of sectors (1-63)"
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ide64_menu_HD_2[] = {
    SDL_MENU_ITEM_TITLE("ATA device 2 settings"),
    {   .string   = "Device 2 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_IDE64Image2_callback,
        .data     = (ui_callback_data_t)"Select Device 2 image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Device 2 geometry"),
    {   .string   = "Autodetect geometry",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IDE64AutodetectSize2_callback
    },
    {   .string   = "Cylinders",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Cylinders2_callback,
        .data     = (ui_callback_data_t)"Enter amount of cylinders (1-1024)"
    },
    {   .string   = "Heads",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Heads2_callback,
        .data     = (ui_callback_data_t)"Enter amount of heads (1-16)"
    },
    {   .string   = "Sectors",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Sectors2_callback,
        .data     = (ui_callback_data_t)"Enter amount of sectors (1-63)"
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ide64_menu_HD_3[] = {
    SDL_MENU_ITEM_TITLE("ATA device 3 settings"),
    {   .string   = "Device 3 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_IDE64Image3_callback,
        .data     = (ui_callback_data_t)"Select Device 3 image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Device 3 geometry"),
    {   .string   = "Autodetect geometry",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IDE64AutodetectSize3_callback
    },
    {   .string   = "Cylinders",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Cylinders3_callback,
        .data     = (ui_callback_data_t)"Enter amount of cylinders (1-1024)"
    },
    {   .string   = "Heads",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Heads3_callback,
        .data     = (ui_callback_data_t)"Enter amount of heads (1-16)"
    },
    {   .string   = "Sectors",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Sectors3_callback,
        .data     = (ui_callback_data_t)"Enter amount of sectors (1-63)"
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ide64_menu_HD_4[] = {
    SDL_MENU_ITEM_TITLE("ATA device 4 settings"),
    {   .string   = "Device 4 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_IDE64Image4_callback,
        .data     = (ui_callback_data_t)"Select Device 4 image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Device 4 geometry"),
    {   .string   = "Autodetect geometry",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IDE64AutodetectSize4_callback
    },
    {   .string   = "Cylinders",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Cylinders4_callback,
        .data     = (ui_callback_data_t)"Enter amount of cylinders (1-1024)"
    },
    {   .string   = "Heads",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Heads4_callback,
        .data     = (ui_callback_data_t)"Enter amount of heads (1-16)"
    },
    {   .string   = "Sectors",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_IDE64Sectors4_callback,
        .data     = (ui_callback_data_t)"Enter amount of sectors (1-63)"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(SBDIGIMAX)
UI_MENU_DEFINE_RADIO(SBDIGIMAXbase)

static const ui_menu_entry_t ide64_digimax_menu[] = {
    SDL_MENU_ITEM_TITLE(CARTRIDGE_NAME_DIGIMAX " settings"),
    {   .string   = "Enable " CARTRIDGE_NAME_DIGIMAX " device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SBDIGIMAX_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE(CARTRIDGE_NAME_DIGIMAX " device address"),
    {   .string   = "$de40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SBDIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde40
    },
    {   .string   = "$de48",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SBDIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0xde48
    },
    SDL_MENU_LIST_END
};

#ifdef HAVE_RAWNET
UI_MENU_DEFINE_TOGGLE(SBETFE)
UI_MENU_DEFINE_RADIO(SBETFEbase)

static const ui_menu_entry_t ide64_etfe_menu[] = {
    SDL_MENU_ITEM_TITLE("ETFE settings"),
    {   .string   = "Enable ETFE device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SBETFE_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("ETFE device address"),
    {   .string   = "$de00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SBETFEbase_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$de10",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SBETFEbase_callback,
        .data     = (ui_callback_data_t)0xde10
    },
    {   .string   = "$df00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SBETFEbase_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    SDL_MENU_LIST_END
};
#endif

const ui_menu_entry_t ide64_menu[] = {
    SDL_MENU_ITEM_TITLE("Cartridge version"),
    {   .string   = "V3",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IDE64version_callback,
        .data     = (ui_callback_data_t)IDE64_VERSION_3
    },
    {   .string   = "V4.1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IDE64version_callback,
        .data     = (ui_callback_data_t)IDE64_VERSION_4_1
    },
    {   .string   = "V4.2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IDE64version_callback,
        .data     = (ui_callback_data_t)IDE64_VERSION_4_2
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Save RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IDE64RTCSave_callback
    },
#ifdef HAVE_NETWORK
    {   .string   = "Enable USB server",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IDE64USBServer_callback
    },
    {   .string   = "USB server address",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_IDE64USBServerAddress_callback,
        .data     = (ui_callback_data_t)"Set USB server address"
    },
#endif
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("ATA Device settings"),
    {   .string   = "ATA Device 1 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu_HD_1
    },
    {   .string   = "ATA Device 2 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu_HD_2
    },
    {   .string   = "ATA Device 3 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu_HD_3
    },
    {   .string   = "ATA Device 4 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu_HD_4
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Clockport device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_clockport_device_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Shortbus Device settings"),
    {   .string   = CARTRIDGE_NAME_DIGIMAX " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_digimax_menu
    },
#ifdef HAVE_RAWNET
    {   .string   = "ETFE settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_etfe_menu
    },
#endif
    SDL_MENU_LIST_END
};

void uiclockport_ide64_menu_create(void)
{
    int i;

    for (i = 0; clockport_supported_devices[i].name; ++i) {
        ide64_clockport_device_menu[i].action   = ACTION_NONE;
        ide64_clockport_device_menu[i].string   = clockport_supported_devices[i].name;
        ide64_clockport_device_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        ide64_clockport_device_menu[i].callback = radio_IDE64ClockPort_callback;
        ide64_clockport_device_menu[i].data     = (ui_callback_data_t)vice_int_to_ptr(clockport_supported_devices[i].id);
    }

    ide64_clockport_device_menu[i].action   = ACTION_NONE;
    ide64_clockport_device_menu[i].string   = NULL;
    ide64_clockport_device_menu[i].type     = MENU_ENTRY_TEXT;
    ide64_clockport_device_menu[i].callback = NULL;
    ide64_clockport_device_menu[i].data     = NULL;
}
