/*
 * menu_rs232.c - RS-232 menus for SDL UI.
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

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)

#include <stdio.h>

#include "acia.h"
#include "menu_common.h"
#include "resources.h"
#include "rsuser.h"
#include "types.h"
#include "uimenu.h"
#include "userport.h"

#include "menu_rs232.h"


UI_MENU_DEFINE_RADIO(RsDevice1Baud)
UI_MENU_DEFINE_RADIO(RsDevice2Baud)
UI_MENU_DEFINE_RADIO(RsDevice3Baud)
UI_MENU_DEFINE_RADIO(RsDevice4Baud)

#define RS_BAUD_MENU(x)                                     \
    static const ui_menu_entry_t rs##x##baud_menu[] = {     \
        {   .string   = "300",                              \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)300             \
        },                                                  \
        {   .string   = "600",                              \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)300             \
        },                                                  \
        {   .string   = "1200",                             \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)1200            \
        },                                                  \
        {   .string   = "2400",                             \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)2400            \
        },                                                  \
        {   .string   = "9600",                             \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)9600            \
        },                                                  \
        {   .string   = "19200",                            \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)19200           \
        },                                                  \
        {   .string   = "38400",                            \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)38400           \
        },                                                  \
        {   .string   = "57600",                            \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)57600           \
        },                                                  \
        {   .string   = "115200",                           \
            .type     = MENU_ENTRY_RESOURCE_RADIO,          \
            .callback = radio_RsDevice##x##Baud_callback,   \
            .data     = (ui_callback_data_t)115200          \
        },                                                  \
        SDL_MENU_LIST_END                                   \
    };

RS_BAUD_MENU(1)
RS_BAUD_MENU(2)
RS_BAUD_MENU(3)
RS_BAUD_MENU(4)

/* Common menus */

UI_MENU_DEFINE_STRING(RsDevice1)
UI_MENU_DEFINE_STRING(RsDevice2)
UI_MENU_DEFINE_STRING(RsDevice3)
UI_MENU_DEFINE_STRING(RsDevice4)

UI_MENU_DEFINE_TOGGLE(RsDevice1ip232)
UI_MENU_DEFINE_TOGGLE(RsDevice2ip232)
UI_MENU_DEFINE_TOGGLE(RsDevice3ip232)
UI_MENU_DEFINE_TOGGLE(RsDevice4ip232)

UI_MENU_DEFINE_TOGGLE(Acia1Enable)
UI_MENU_DEFINE_RADIO(Acia1Dev)

static const ui_menu_entry_t acia1dev_menu[] = {
    {   .string   = "1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Dev_callback,
        .data     = (ui_callback_data_t)ACIA_DEVICE_1
    },
    {   .string   = "2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Dev_callback,
        .data     = (ui_callback_data_t)ACIA_DEVICE_2
    },
    {   .string   = "3",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Dev_callback,
        .data     = (ui_callback_data_t)ACIA_DEVICE_3
    },
    {   .string   = "4",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Dev_callback,
        .data     = (ui_callback_data_t)ACIA_DEVICE_4
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(Acia1Irq)

static const ui_menu_entry_t acia1irq_menu[] = {
    {   .string   = "No IRQ/NMI",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Irq_callback,
        .data     = (ui_callback_data_t)ACIA_INT_NONE
    },
    {   .string   = "NMI",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Irq_callback,
        .data     = (ui_callback_data_t)ACIA_INT_NMI
    },
    {   .string   = "IRQ",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Irq_callback,
        .data     = (ui_callback_data_t)ACIA_INT_IRQ
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(Acia1Mode)

static const ui_menu_entry_t acia1mode_menu[] = {
    {   .string   = "Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Mode_callback,
        .data     = (ui_callback_data_t)ACIA_MODE_NORMAL
    },
    {   .string   = "Swiftlink",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Mode_callback,
        .data     = (ui_callback_data_t)ACIA_MODE_SWIFTLINK
    },
    {   .string   = "Turbo232",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Mode_callback,
        .data     = (ui_callback_data_t)ACIA_MODE_TURBO232
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(Acia1Base)

static const ui_menu_entry_t acia1base_c64_menu[] = {
    {   .string   = "$DE00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$DF00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t acia1base_c128_menu[] = {
    {   .string   = "$D700",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0xd700
    },
    {   .string   = "$DE00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0xde00
    },
    {   .string   = "$DF00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0xdf00
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t acia1base_vic20_menu[] = {
    {   .string   = "$9800",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0x9800
    },
    {   .string   = "$9C00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Acia1Base_callback,
        .data     = (ui_callback_data_t)0x9c00
    },
    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(radio_UserportDevice_callback)
{
    int val = USERPORT_DEVICE_NONE;
    resources_get_int("UserportDevice", &val);
    if (activated) {
        if (val == USERPORT_DEVICE_RS232_MODEM) {
            resources_set_int("UserportDevice", USERPORT_DEVICE_NONE);
        } else {
            resources_set_int("UserportDevice", USERPORT_DEVICE_RS232_MODEM);
        }
        return NULL;
    }
    return (val == USERPORT_DEVICE_RS232_MODEM) ? sdl_menu_text_tick : NULL;
}

UI_MENU_DEFINE_TOGGLE(RsUserUP9600)
UI_MENU_DEFINE_TOGGLE(RsUserRTSInv)
UI_MENU_DEFINE_TOGGLE(RsUserCTSInv)
UI_MENU_DEFINE_TOGGLE(RsUserDSRInv)
UI_MENU_DEFINE_TOGGLE(RsUserDCDInv)
UI_MENU_DEFINE_TOGGLE(RsUserDTRInv)
UI_MENU_DEFINE_TOGGLE(RsUserRIInv)

UI_MENU_DEFINE_RADIO(RsUserBaud)

static const ui_menu_entry_t rsuserbaud_menu[] = {
    {   .string   = "300",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserBaud_callback,
        .data     = (ui_callback_data_t)300
    },
    {   .string   = "1200",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserBaud_callback,
        .data     = (ui_callback_data_t)1200
    },
    {   .string   = "2400",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserBaud_callback,
        .data     = (ui_callback_data_t)2400
    },
    {   .string   = "9600",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserBaud_callback,
        .data     = (ui_callback_data_t)9600
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(RsUserDev)

static const ui_menu_entry_t rsuserdev_menu[] = {
    {   .string   = "1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserDev_callback,
        .data     = (ui_callback_data_t)RS_USER_DEVICE_1
    },
    {   .string   = "2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserDev_callback,
        .data     = (ui_callback_data_t)RS_USER_DEVICE_2
    },
    {   .string   = "3",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserDev_callback,
        .data     = (ui_callback_data_t)RS_USER_DEVICE_3
    },
    {   .string   = "4",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RsUserDev_callback,
        .data     = (ui_callback_data_t)RS_USER_DEVICE_4
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t rs232_nouser_menu[] = {
    {   .string   = "ACIA host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1dev_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Host settings"),
    {   .string   = "Device 1",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice1_callback,
        .data     = (ui_callback_data_t)"RS232 host device 1"
    },
    {   .string   = "Device 1 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs1baud_menu
    },
    {   .string   = "Device 1 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice1ip232_callback
    },
    {   .string   = "Device 2",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice2_callback,
        .data     = (ui_callback_data_t)"RS232 host device 2"
    },
    {   .string   = "Device 2 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs2baud_menu
    },
    {   .string   = "Device 2 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice2ip232_callback
    },
    {   .string   = "Device 3",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice3_callback,
        .data     = (ui_callback_data_t)"RS232 host device 3"
    },
    {   .string   = "Device 3 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs3baud_menu
    },
    {   .string   = "Device 3 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice3ip232_callback
    },
    {   .string   = "Device 4",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice4_callback,
        .data     = (ui_callback_data_t)"RS232 host device 4"
    },
    {   .string   = "Device 4 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs4baud_menu
    },
    {   .string   = "Device 4 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice4ip232_callback
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t rs232_c64_menu[] = {
    {   .string   = "ACIA RS232 interface emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Acia1Enable_callback
    },
    {   .string   = "ACIA base address",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1base_c64_menu
    },
    {   .string   = "ACIA host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1dev_menu
    },
    {   .string   = "ACIA interrupt",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1irq_menu
    },
    {   .string   = "ACIA emulation mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1mode_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Userport RS232 emulation",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_UserportDevice_callback,
        .data     = (ui_callback_data_t)USERPORT_DEVICE_RS232_MODEM
    },
    {   .string   = "Userport RS232 host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rsuserdev_menu
    },
    {   .string   = "Userport RS232 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rsuserbaud_menu
    },
    {   .string   = "use UP9600 interface emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserUP9600_callback
    },
    {   .string   = "invert RTS line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserRTSInv_callback
    },
    {   .string   = "invert CTS line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserCTSInv_callback
    },
    {   .string   = "invert DSR line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDSRInv_callback
    },
    {   .string   = "invert DCD line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDCDInv_callback
    },
    {   .string   = "invert DTR line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDTRInv_callback
    },
    {   .string   = "invert RI line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserRIInv_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Host settings"),
    {   .string   = "Device 1",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice1_callback,
        .data     = (ui_callback_data_t)"RS232 host device 1"
    },
    {   .string   = "Device 1 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs1baud_menu
    },
    {   .string   = "Device 1 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice1ip232_callback
    },
    {   .string   = "Device 2",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice2_callback,
        .data     = (ui_callback_data_t)"RS232 host device 2"
    },
    {   .string   = "Device 2 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs2baud_menu
    },
    {   .string   = "Device 2 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice2ip232_callback
    },
    {   .string   = "Device 3",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice3_callback,
        .data     = (ui_callback_data_t)"RS232 host device 3"
    },
    {   .string   = "Device 3 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs3baud_menu
    },
    {   .string   = "Device 3 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice3ip232_callback
    },
    {   .string   = "Device 4",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice4_callback,
        .data     = (ui_callback_data_t)"RS232 host device 4"
    },
    {   .string   = "Device 4 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs4baud_menu
    },
    {   .string   = "Device 4 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice4ip232_callback
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t rs232_c128_menu[] = {
    {   .string   = "ACIA RS232 interface emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Acia1Enable_callback
    },
    {   .string   = "ACIA base address",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1base_c128_menu
    },
    {   .string   = "ACIA host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1dev_menu
    },
    {   .string   = "ACIA interrupt",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1irq_menu
    },
    {   .string   = "ACIA emulation mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1mode_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Userport RS232 emulation",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_UserportDevice_callback,
        .data     = (ui_callback_data_t)USERPORT_DEVICE_RS232_MODEM
    },
    {   .string   = "Userport RS232 host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rsuserdev_menu
    },
    {   .string   = "Userport RS232 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rsuserbaud_menu
    },
    {   .string   = "use UP9600 interface emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserUP9600_callback
    },
    {   .string   = "invert RTS line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserRTSInv_callback
    },
    {   .string   = "invert CTS line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserCTSInv_callback
    },
    {   .string   = "invert DSR line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDSRInv_callback
    },
    {   .string   = "invert DCD line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDCDInv_callback
    },
    {   .string   = "invert DTR line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDTRInv_callback
    },
    {   .string   = "invert RI line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserRIInv_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Host settings"),
    {   .string   = "Device 1",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice1_callback,
        .data     = (ui_callback_data_t)"RS232 host device 1"
    },
    {   .string   = "Device 1 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs1baud_menu
    },
    {   .string   = "Device 1 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice1ip232_callback
    },
    {   .string   = "Device 2",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice2_callback,
        .data     = (ui_callback_data_t)"RS232 host device 2"
    },
    {   .string   = "Device 2 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs2baud_menu
    },
    {   .string   = "Device 2 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice2ip232_callback
    },
    {   .string   = "Device 3",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice3_callback,
        .data     = (ui_callback_data_t)"RS232 host device 3"
    },
    {   .string   = "Device 3 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs3baud_menu
    },
    {   .string   = "Device 3 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice3ip232_callback
    },
    {   .string   = "Device 4",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice4_callback,
        .data     = (ui_callback_data_t)"RS232 host device 4"
    },
    {   .string   = "Device 4 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs4baud_menu
    },
    {   .string   = "Device 4 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice4ip232_callback,
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t rs232_vic20_menu[] = {
    {   .string   = "ACIA RS232 interface emulation (MasC=uerade)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Acia1Enable_callback
    },
    {   .string   = "ACIA base address",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1base_vic20_menu
    },
    {   .string   = "ACIA host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1dev_menu
    },
    {   .string   = "ACIA interrupt",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1irq_menu
    },
    {   .string   = "ACIA emulation mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)acia1mode_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Userport RS232 emulation",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_UserportDevice_callback,
        .data     = (ui_callback_data_t)USERPORT_DEVICE_RS232_MODEM
    },
    {   .string   = "Userport RS232 host device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rsuserdev_menu
    },
    {   .string   = "Userport RS232 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rsuserbaud_menu
    },
    {   .string   = "use UP9600 interface emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserUP9600_callback,
    },
    {   .string   = "invert RTS line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserRTSInv_callback
    },
    {   .string   = "invert CTS line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserCTSInv_callback
    },
    {   .string   = "invert DSR line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDSRInv_callback
    },
    {   .string   = "invert DCD line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDCDInv_callback
    },
    {   .string   = "invert DTR line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserDTRInv_callback
    },
    {   .string   = "invert RI line",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsUserRIInv_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Host settings"),
    {   .string   = "Device 1",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice1_callback,
        .data     = (ui_callback_data_t)"RS232 host device 1"
    },
    {   .string   = "Device 1 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs1baud_menu
    },
    {   .string   = "Device 1 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice1ip232_callback
    },
    {   .string   = "Device 2",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice2_callback,
        .data     = (ui_callback_data_t)"RS232 host device 2"
    },
    {   .string   = "Device 2 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs2baud_menu
    },
    {   .string   = "Device 2 use IP232 protocol",
        .type     =  MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice2ip232_callback
    },
    {   .string   = "Device 3",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice3_callback,
        .data     = (ui_callback_data_t)"RS232 host device 3"
    },
    {   .string   = "Device 3 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs3baud_menu
    },
    {   .string   = "Device 3 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice3ip232_callback
    },
    {   .string   = "Device 4",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_RsDevice4_callback,
        .data     = (ui_callback_data_t)"RS232 host device 4"
    },
    {   .string   = "Device 4 baud rate",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)rs4baud_menu
    },
    {   .string   = "Device 4 use IP232 protocol",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RsDevice4ip232_callback
    },
    SDL_MENU_LIST_END
};
#endif
