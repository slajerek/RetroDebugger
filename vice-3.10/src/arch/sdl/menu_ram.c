/*
 * menu_ram.c - RAM pattern menu for SDL UI.
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
#include "menu_ram.h"
#include "resources.h"
#include "uimenu.h"

static const char *sdl_ui_menu_ram_slider_helper(int activated, ui_callback_data_t param, const char *resource_name, const int min, const int max)
{
    static char buf[20];
    int previous, new_value;

    if (resources_get_int(resource_name, &previous)) {
        return sdl_menu_text_unknown;
    }

    sprintf(buf, "%3.3f%%", (float)previous / (4096.0f / 100.0f));

    if (activated) {
        new_value = sdl_ui_slider_input_dialog((const char *)param, previous, min, max);
        if (new_value != previous) {
            resources_set_int(resource_name, new_value);
        }
    } else {
        return buf;
    }

    return NULL;
}

#define UI_MENU_DEFINE_SLIDER_RAM(resource, min, max)                              \
    static UI_MENU_CALLBACK(slider_##resource##_callback)                          \
    {                                                                              \
        return sdl_ui_menu_ram_slider_helper(activated, param, #resource, min, max);   \
    }

UI_MENU_DEFINE_RADIO(RAMInitStartValue)
UI_MENU_DEFINE_RADIO(RAMInitValueInvert)

UI_MENU_DEFINE_RADIO(RAMInitPatternInvert)
UI_MENU_DEFINE_RADIO(RAMInitPatternInvertValue)

UI_MENU_DEFINE_RADIO(RAMInitStartRandom)
UI_MENU_DEFINE_RADIO(RAMInitRepeatRandom)
UI_MENU_DEFINE_SLIDER_RAM(RAMInitRandomChance, 0, 0x1000)

static const ui_menu_entry_t RAMInitValueInvert_menu[] = {
    {   .string   = "no",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1 byte.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "2 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)2
    },
    {   .string   = "4 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "8 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "16 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)16
    },
    {   .string   = "32 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)32
    },
    {   .string   = "64 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)64
    },
    {   .string   = "128 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)256
    },
    {   .string   = "512 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)512
    },
    {   .string   = "1024 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)1024
    },

    {   .string   = "2048 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)2048
    },
    {   .string   = "4096 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)4096
    },
    {   .string   = "8192 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)8192
    },
    {   .string   = "16384 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)16384
    },
    {   .string   = "32768 bytes.",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitValueInvert_callback,
        .data     = (ui_callback_data_t)32768
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t RAMInitPatternInvert_menu[] = {
    {   .string   = "0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)2
    },
    {   .string   = "4",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "8",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "16",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)16
    },
    {   .string   = "32",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)32
    },
    {   .string   = "64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)64
    },
    {   .string   = "128",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)256
    },
    {   .string   = "512",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)512
    },
    {   .string   = "1024",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)1024
    },
    {   .string   = "2048",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)2048
    },
    {   .string   = "4096",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)4096
    },
    {   .string   = "8192",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)8192
    },
    {   .string   = "16384",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)16384
    },
    {   .string   = "32768",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvert_callback,
        .data     = (ui_callback_data_t)326768
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t RAMInitStartRandom_menu[] = {
    {   .string   = "none",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)2
    },
    {   .string   = "4",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "8",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "16",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)16
    },
    {   .string   = "32",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)32
    },
    {   .string   = "64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)64
    },
    {   .string   = "128",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartRandom_callback,
        .data     = (ui_callback_data_t)256
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t RAMInitRepeatRandom_menu[] = {
    {   .string   = "none",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)2
    },
    {   .string   = "4",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "8",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "16",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)16
    },
    {   .string   = "32",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)32
    },
    {   .string   = "64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)64
    },
    {   .string   = "128",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitRepeatRandom_callback,
        .data     = (ui_callback_data_t)256
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t ram_menu[] = {
    SDL_MENU_ITEM_TITLE("Value of first byte"),
    {   .string   = "$0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartValue_callback,
        .data     = (ui_callback_data_t)0x00
    },
    {   .string   = "$ff",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitStartValue_callback,
        .data     = (ui_callback_data_t)0xff
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Invert first byte every",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)RAMInitValueInvert_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Value of second byte"),
    {   .string   = "$00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvertValue_callback,
        .data     = (ui_callback_data_t)0x00
    },
    {   .string   = "$66",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvertValue_callback,
        .data     = (ui_callback_data_t)0x66
    },
    {   .string   = "$99",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvertValue_callback,
        .data     = (ui_callback_data_t)0x99
    },
    {   .string   = "$ff",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMInitPatternInvertValue_callback,
        .data     = (ui_callback_data_t)0xff
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Invert with second byte every",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)RAMInitPatternInvert_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Length of random pattern",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)RAMInitStartRandom_menu
    },
    {   .string   = "Repeat random pattern each",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)RAMInitRepeatRandom_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Global random chance:",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = slider_RAMInitRandomChance_callback,
        .data     = (ui_callback_data_t)"Set chance (0-4096)"
    },
    SDL_MENU_LIST_END
};
