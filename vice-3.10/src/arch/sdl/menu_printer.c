/*
 * menu_printer.h - Printer menu for SDL UI.
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

#include <stdio.h>

#include "types.h"

#include "menu_common.h"
#include "printer.h"
#include "resources.h"
#include "uimenu.h"
#include "userport.h"

UI_MENU_DEFINE_RADIO(UserportDevice)

#define VICE_SDL_PRINTER_DRIVER_MENU(prn)                               \
    UI_MENU_DEFINE_RADIO(Printer##prn##Driver)                          \
    static const ui_menu_entry_t printer_##prn##_driver_submenu[] = {   \
        {   .string   = "ASCII",                                        \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"ascii"                     \
        },                                                              \
        {   .string   = "Commodore 2022",                               \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"2022"                      \
        },                                                              \
        {   .string   = "Commodore 4023",                               \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"4023"                      \
        },                                                              \
        {   .string   = "Commodore 8023",                               \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"8023"                      \
        },                                                              \
        {   .string   = "MPS801",                                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"mps801"                    \
        },                                                              \
        {   .string   = "MPS802",                                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"mps802"                    \
        },                                                              \
        {   .string   = "MPS803",                                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"mps803"                    \
        },                                                              \
        {   .string   = "NL10",                                         \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"nl10"                      \
        },                                                              \
        {   .string   = "Raw",                                          \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Driver_callback,            \
            .data     = (ui_callback_data_t)"raw"                       \
        },                                                              \
        SDL_MENU_LIST_END                                               \
    };

VICE_SDL_PRINTER_DRIVER_MENU(4)
VICE_SDL_PRINTER_DRIVER_MENU(5)

UI_MENU_DEFINE_RADIO(PrinterUserportDriver)

static const ui_menu_entry_t printer_Userport_driver_submenu[] = {
    {   .string   = "ASCII",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PrinterUserportDriver_callback,
        .data     = (ui_callback_data_t)"ascii"
    },
    {   .string   = "NL10",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PrinterUserportDriver_callback,
        .data     = (ui_callback_data_t)"nl10"
    },
    {   .string   = "Raw",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_PrinterUserportDriver_callback,
        .data     = (ui_callback_data_t)"raw"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(Printer6Driver)

static const ui_menu_entry_t printer_6_driver_submenu[] = {
    {   .string   = "1520",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Printer6Driver_callback,
        .data     = (ui_callback_data_t)"1520"
    },
    {   .string   = "Raw",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_Printer6Driver_callback,
        .data     = (ui_callback_data_t)"raw"
    },
    SDL_MENU_LIST_END
};

#define VICE_SDL_PRINTER_DEVICE_MENU(prn)                               \
    UI_MENU_DEFINE_RADIO(Printer##prn##TextDevice)                      \
    static const ui_menu_entry_t printer_##prn##_device_submenu[] = {   \
        {   .string   = "1",                                            \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##TextDevice_callback,        \
            .data     = (ui_callback_data_t)PRINTER_TEXT_DEVICE_1       \
        },                                                              \
        {   .string   = "2",                                            \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##TextDevice_callback,        \
            .data     = (ui_callback_data_t)PRINTER_TEXT_DEVICE_2       \
        },                                                              \
        {   .string   = "3",                                            \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##TextDevice_callback,        \
            .data     = (ui_callback_data_t)PRINTER_TEXT_DEVICE_3       \
        },                                                              \
        SDL_MENU_LIST_END                                               \
    };

VICE_SDL_PRINTER_DEVICE_MENU(4)
VICE_SDL_PRINTER_DEVICE_MENU(5)
VICE_SDL_PRINTER_DEVICE_MENU(6)
VICE_SDL_PRINTER_DEVICE_MENU(Userport)


#define VICE_SDL_PRINTER_OUTPUT_MENU(prn)                               \
    UI_MENU_DEFINE_RADIO(Printer##prn##Output)                          \
    static const ui_menu_entry_t printer_##prn##_output_submenu[] = {   \
        {   .string   = "Text",                                         \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Output_callback,            \
            .data     = (ui_callback_data_t)"text"                      \
        },                                                              \
        {   .string   = "Graphics",                                     \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
            .callback = radio_Printer##prn##Output_callback,            \
            .data     = (ui_callback_data_t)"graphics"                  \
        },                                                              \
        SDL_MENU_LIST_END                                               \
    };

VICE_SDL_PRINTER_OUTPUT_MENU(4)
VICE_SDL_PRINTER_OUTPUT_MENU(5)
VICE_SDL_PRINTER_OUTPUT_MENU(6)
VICE_SDL_PRINTER_OUTPUT_MENU(Userport)

#ifdef HAVE_REALDEVICE

#define VICE_SDL_PRINTER_TYPE_MENU(prn)                             \
    UI_MENU_DEFINE_RADIO(Printer##prn)                              \
    static const ui_menu_entry_t printer_##prn##_type_submenu[] = { \
        {   .string   = "None",                                     \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_NONE     \
        },                                                          \
        {   .string   = "File system access",                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_FS       \
        },                                                          \
        {   .string   = "Real device access",                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_REAL     \
        },                                                          \
        SDL_MENU_LIST_END                                           \
    };

#define VICE_SDL_DEVICE_TYPE_MENU(prn)                              \
    UI_MENU_DEFINE_RADIO(Printer##prn)                              \
    static const ui_menu_entry_t device_##prn##_type_submenu[] = {  \
        {   .string   = "None",                                     \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_NONE     \
        },                                                          \
        {   .string   = "Real device access",                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_REAL     \
        },                                                          \
        SDL_MENU_LIST_END                                           \
    };

#else   /* !HAVE_REALDEVICE */

#define VICE_SDL_PRINTER_TYPE_MENU(prn)                             \
    UI_MENU_DEFINE_RADIO(Printer##prn)                              \
    static const ui_menu_entry_t printer_##prn##_type_submenu[] = { \
        {   .string   = "None",                                     \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_NONE     \
        },                                                          \
        {   .string   = "File system access",                       \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
            .callback = radio_Printer##prn##_callback,              \
            .data     = (ui_callback_data_t)PRINTER_DEVICE_FS },    \
        SDL_MENU_LIST_END                                           \
    };

#define VICE_SDL_DEVICE_TYPE_MENU(prn)

#endif

VICE_SDL_PRINTER_TYPE_MENU(4)
VICE_SDL_PRINTER_TYPE_MENU(5)
VICE_SDL_PRINTER_TYPE_MENU(6)

#ifdef HAVE_REALDEVICE
VICE_SDL_DEVICE_TYPE_MENU(7)
UI_MENU_DEFINE_TOGGLE(TrapDevice7)
UI_MENU_DEFINE_TOGGLE(BusDevice7)
#endif

UI_MENU_DEFINE_TOGGLE(TrapDevice4)
UI_MENU_DEFINE_TOGGLE(TrapDevice5)
UI_MENU_DEFINE_TOGGLE(TrapDevice6)
UI_MENU_DEFINE_TOGGLE(BusDevice4)
UI_MENU_DEFINE_TOGGLE(BusDevice5)
UI_MENU_DEFINE_TOGGLE(BusDevice6)

static UI_MENU_CALLBACK(uiprinter_formfeed_callback)
{
    printer_formfeed(vice_ptr_to_uint(param));
    return NULL;
}


#define VICE_SDL_PRINTER_COMMON_4_MENU_ITEMS                        \
    {   .string   = "Printer #4 emulation",                         \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_4_type_submenu      \
    },                                                              \
    {   .string   = "Printer #4 driver",                            \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_4_driver_submenu    \
    },                                                              \
    {   .string   = "Printer #4 output type",                       \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_4_output_submenu    \
    },                                                              \
    {   .string   = "Printer #4 output device",                     \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_4_device_submenu    \
    },                                                              \
    {   .string   = "Printer #4 form feed",                         \
        .type     = MENU_ENTRY_OTHER,                               \
        .callback = uiprinter_formfeed_callback,                    \
        .data     = (ui_callback_data_t)PRINTER_IEC_4               \
    },

#define VICE_SDL_PRINTER_COMMON_5_MENU_ITEMS                        \
    {   .string   = "Printer #5 emulation",                         \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_5_type_submenu      \
    },                                                              \
    {   .string   = "Printer #5 driver",                            \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_5_driver_submenu    \
    },                                                              \
    {   .string   = "Printer #5 output type",                       \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_5_output_submenu    \
    },                                                              \
    {   .string   = "Printer #5 output device",                     \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_5_device_submenu    \
    },                                                              \
    {   .string   = "Printer #5 form feed",                         \
        .type     = MENU_ENTRY_OTHER,                               \
        .callback = uiprinter_formfeed_callback,                    \
        .data     = (ui_callback_data_t)PRINTER_IEC_5 \
    },

#define VICE_SDL_PRINTER_COMMON_6_MENU_ITEMS                        \
    {   .string   = "Printer #6 emulation",                         \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_6_type_submenu      \
    },                                                              \
    {   .string   = "Printer #6 driver",                            \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_6_driver_submenu    \
    },                                                              \
    {   .string   = "Printer #6 output type",                       \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_6_output_submenu    \
    },                                                              \
    {   .string   = "Printer #6 output device",                     \
        .type     = MENU_ENTRY_SUBMENU,                             \
        .callback = submenu_radio_callback,                         \
        .data     = (ui_callback_data_t)printer_6_device_submenu    \
    },                                                              \
    {   .string   = "Printer #6 form feed",                         \
        .type     = MENU_ENTRY_OTHER,                               \
        .callback = uiprinter_formfeed_callback,                    \
        .data     = (ui_callback_data_t)PRINTER_IEC_6               \
    },

#ifdef HAVE_REALDEVICE
#define VICE_SDL_DEVICE_COMMON_7_MENU_ITEMS                     \
    {   .string   = "Device #7 emulation",                      \
        .type     = MENU_ENTRY_SUBMENU,                         \
        .callback = submenu_radio_callback,                     \
        .data     = (ui_callback_data_t)device_7_type_submenu   \
    },
#endif

#define VICE_SDL_PRINTER_USERPORT_MENU_ITEMS                            \
    {   .string   = "Userport printer emulation",                       \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                          \
        .callback = radio_UserportDevice_callback,                      \
        .data     = (ui_callback_data_t)USERPORT_DEVICE_PRINTER         \
    },                                                                  \
    {   .string   = "Userport printer driver",                          \
        .type     = MENU_ENTRY_SUBMENU,                                 \
        .callback = submenu_radio_callback,                             \
        .data     = (ui_callback_data_t)printer_Userport_driver_submenu \
    },                                                                  \
    {   .string   = "Userport printer output type",                     \
        .type     = MENU_ENTRY_SUBMENU,                                 \
        .callback = submenu_radio_callback,                             \
        .data     = (ui_callback_data_t)printer_Userport_output_submenu \
    },                                                                  \
    {   .string   = "Userport printer output device",                   \
        .type     = MENU_ENTRY_SUBMENU,                                 \
        .callback = submenu_radio_callback,                             \
        .data     = (ui_callback_data_t)printer_Userport_device_submenu \
    },                                                                  \
    {   .string   = "Userport printer form feed",                       \
        .type     = MENU_ENTRY_OTHER,                                   \
        .callback = uiprinter_formfeed_callback,                        \
        .data     = (ui_callback_data_t)PRINTER_USERPORT                \
    },

UI_MENU_DEFINE_STRING(PrinterTextDevice1)
UI_MENU_DEFINE_STRING(PrinterTextDevice2)
UI_MENU_DEFINE_STRING(PrinterTextDevice3)

#define VICE_SDL_PRINTER_DEVICEFILE_MENU_ITEMS                  \
    {   .string   = "Device 1",                                 \
        .type     = MENU_ENTRY_RESOURCE_STRING,                 \
        .callback = string_PrinterTextDevice1_callback,         \
        .data     = (ui_callback_data_t)"Printer device 1 file" \
    },                                                          \
    {   .string   = "Device 2",                                 \
        .type     = MENU_ENTRY_RESOURCE_STRING,                 \
        .callback = string_PrinterTextDevice2_callback,         \
        .data     = (ui_callback_data_t)"Printer device 2 file" \
    },                                                          \
    {   .string   = "Device 3",                                 \
        .type     = MENU_ENTRY_RESOURCE_STRING,                 \
        .callback = string_PrinterTextDevice3_callback,         \
        .data     = (ui_callback_data_t)"Printer device 3 file" \
    },

const ui_menu_entry_t printer_iec_menu[] = {
    VICE_SDL_PRINTER_COMMON_4_MENU_ITEMS
    {   .string   = "Printer #4 enable virt. (trap) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TrapDevice4_callback
    },
    {   .string   = "Printer #4 enable virt. (bus) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_BusDevice4_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    VICE_SDL_PRINTER_COMMON_5_MENU_ITEMS
    {   .string   = "Printer #5 enable virt. (trap) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TrapDevice5_callback
    },
    {   .string   = "Printer #5 enable virt. (bus) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_BusDevice5_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    VICE_SDL_PRINTER_COMMON_6_MENU_ITEMS
    {   .string   = "Printer #6 enable virt. (trap) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TrapDevice6_callback
    },
    {   .string   = "Printer #6 enable virt. (bus) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_BusDevice6_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

#ifdef HAVE_REALDEVICE
    VICE_SDL_DEVICE_COMMON_7_MENU_ITEMS
    {   .string   = "Device #7 enable virt. (trap) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TrapDevice7_callback
    },
    {   .string   = "Device #7 enable virt. (bus) device",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_BusDevice7_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

#endif
    VICE_SDL_PRINTER_USERPORT_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_DEVICEFILE_MENU_ITEMS
    SDL_MENU_LIST_END
};

const ui_menu_entry_t printer_ieee_menu[] = {
    VICE_SDL_PRINTER_COMMON_4_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_COMMON_5_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_COMMON_6_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_USERPORT_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_DEVICEFILE_MENU_ITEMS
    SDL_MENU_LIST_END
};

const ui_menu_entry_t printer_iec_nouserport_menu[] = {
    VICE_SDL_PRINTER_COMMON_4_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_COMMON_5_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
    VICE_SDL_PRINTER_COMMON_6_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
#ifdef HAVE_REALDEVICE
    VICE_SDL_DEVICE_COMMON_7_MENU_ITEMS
    SDL_MENU_ITEM_SEPARATOR,
#endif
    VICE_SDL_PRINTER_DEVICEFILE_MENU_ITEMS
    SDL_MENU_LIST_END
};
