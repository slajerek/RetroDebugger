/*
 * menu_sampler.c - Sampler menu for SDL UI.
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

#include "menu_common.h"
#include "sampler.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"

#include "menu_sampler.h"

/* DIGIMAX MENU */

UI_MENU_DEFINE_RADIO(SamplerDevice)
UI_MENU_DEFINE_INT(SamplerGain)
UI_MENU_DEFINE_FILE_STRING(SampleName)

static ui_menu_entry_t sampler_device_menu[SAMPLER_MAX_DEVICES + 1];

ui_menu_entry_t sampler_menu[] = {
    {   .string   = "Sampler device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sampler_device_menu
    },
    {   .string   = "Sampler gain",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_SamplerGain_callback,
        .data     = (ui_callback_data_t)"Enter sampler gain (1-200)"
    },
    {   .string   = "Sampler input media file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_SampleName_callback,
        .data     = (ui_callback_data_t)"Select sampler input media file"
    },
    SDL_MENU_LIST_END
};

void uisampler_menu_create(void)
{
    sampler_device_t *devices = sampler_get_devices();
    int i;

    for (i = 0; devices[i].name; ++i) {
        sampler_device_menu[i].action   = ACTION_NONE;
        sampler_device_menu[i].string   = (char*)devices[i].name;   /* yuck! */
        sampler_device_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        sampler_device_menu[i].callback = radio_SamplerDevice_callback;
        sampler_device_menu[i].data     = (ui_callback_data_t)vice_int_to_ptr(i);
    }

    sampler_device_menu[i].string = NULL;
}
