/*
 * menu_speed.c - Implementation of the speed settings menu for the SDL UI.
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
#include <stdlib.h>

#include "actions-speed.h"
#include "lib.h"
#include "menu_common.h"
#include "menu_speed.h"
#include "resources.h"
#include "uiactions.h"
#include "uimenu.h"
#include "vsync.h"


static UI_MENU_CALLBACK(custom_Speed_callback)
{
    static char buf[20];
    char *value = NULL;
    int previous, new_value;

    resources_get_int("Speed", &previous);

    if (activated) {
        sprintf(buf, "%i", previous > 0 ? previous : 0);
        value = sdl_ui_text_input_dialog("Enter custom maximum speed", buf);
        if (value) {
            new_value = (int)strtol(value, NULL, 0);
            if (new_value != previous) {
                resources_set_int("Speed", new_value);
            }
            lib_free(value);
        }
        ui_action_finish(ACTION_SPEED_CPU_CUSTOM);
    } else {
        if ((previous > 0) && (previous != 10) && (previous != 25) &&
            (previous != 50) && (previous != 100) && (previous != 200)) {
            sprintf(buf, "%i%%", previous);
            return buf;
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(custom_Fps_callback)
{
    static char buf[32];
    char *value = NULL;
    int previous, new_value;

    resources_get_int("Speed", &previous);

    if (activated) {
        sprintf(buf, "%i", previous < 0 ? -previous : 0);
        value = sdl_ui_text_input_dialog("Enter target Fps", buf);
        if (value) {
            new_value = -(int)strtol(value, NULL, 0);
            if (new_value != previous) {
                resources_set_int("Speed", new_value);
            }
            lib_free(value);
        }
        ui_action_finish(ACTION_SPEED_FPS_CUSTOM);
    } else {
        if (previous < 0) {
            sprintf(buf, "%i Fps", -previous);
            return buf;
        }
    }
    return NULL;
}


const ui_menu_entry_t speed_menu[] = {
    {   .action    = ACTION_WARP_MODE_TOGGLE,
        .string    = "Warp mode",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = warp_mode_toggle_display
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Maximum CPU speed"),
    {   .action   = ACTION_SPEED_CPU_10,
        .string   = "10%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)10,
    },
    {   .action   = ACTION_SPEED_CPU_25,
        .string   = "25%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)25
    },
    {   .action   = ACTION_SPEED_CPU_50,
        .string   = "50%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)50
    },
    {   .action   = ACTION_SPEED_CPU_100,
        .string   = "100%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)100
    },
    {   .action   = ACTION_SPEED_CPU_200,
        .string   = "200%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)200
    },
    {   .action   = ACTION_SPEED_CPU_CUSTOM,
        .string   = "Custom CPU speed",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_Speed_callback   /* used to display the "dialog" and
                                               the resource value */
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Target Fps"),
    {   .action   = ACTION_SPEED_FPS_50,
        .string   = "50 Fps",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)-50
    },
    {   .action   = ACTION_SPEED_FPS_60,
        .string   = "60 Fps",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)-60
    },
    {   .action   = ACTION_SPEED_FPS_CUSTOM,
        .string   = "Custom Fps",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_Fps_callback     /* used to display the "dialog" and
                                               the resource value */
    },
    SDL_MENU_LIST_END
};


/* VSID specific speed menu */
const ui_menu_entry_t speed_menu_vsid[] = {
    {   .action    = ACTION_WARP_MODE_TOGGLE,
        .string    = "Warp mode",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = warp_mode_toggle_display
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Maximum speed"),
    {   .action   = ACTION_SPEED_CPU_10,
        .string   = "10%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)10
    },
    {   .action   = ACTION_SPEED_CPU_25,
        .string   = "25%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)25
    },
    {   .action   = ACTION_SPEED_CPU_50,
        .string   = "50%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)50
    },
    {   .action   = ACTION_SPEED_CPU_100,
        .string   = "100%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)100
    },
    {   .action   = ACTION_SPEED_CPU_200,
        .string   = "200%",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .resource = "Speed",
        .data     = (ui_callback_data_t)200
    },
    {   .action   = ACTION_SPEED_CPU_CUSTOM,
        .string   = "Custom speed",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_Speed_callback
    },
    SDL_MENU_LIST_END
};
