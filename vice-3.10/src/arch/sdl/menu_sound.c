/*
 * menu_sound.c - Implementation of the sound settings menu for the SDL UI.
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
#include <string.h>

#include "lib.h"
#include "menu_common.h"
#include "resources.h"
#include "sound.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "util.h"

#include "menu_sound.h"


UI_MENU_DEFINE_TOGGLE(Sound)
UI_MENU_DEFINE_TOGGLE(SoundEmulateOnWarp)
UI_MENU_DEFINE_RADIO(SoundSampleRate)
UI_MENU_DEFINE_RADIO(SoundFragmentSize)
UI_MENU_DEFINE_RADIO(SoundDeviceName)
UI_MENU_DEFINE_RADIO(SoundOutput)

static UI_MENU_CALLBACK(custom_volume_callback)
{
    static char buf[20];
    int previous, new_value;

    resources_get_int("SoundVolume", &previous);

    if (activated) {
        new_value = sdl_ui_slider_input_dialog("Select volume", previous, 0, 100);
        if (new_value != previous) {
            resources_set_int("SoundVolume", new_value);
        }
    } else {
        sprintf(buf, "%i%%", previous);
        return buf;
    }
    return NULL;
}

static UI_MENU_CALLBACK(custom_buffer_size_callback)
{
    static char buf[20];
    char *value = NULL;
    int previous, new_value;

    resources_get_int("SoundBufferSize", &previous);

    if (activated) {
        sprintf(buf, "%i", previous);
        value = sdl_ui_text_input_dialog("Enter buffer size in msec", buf);
        if (value) {
            new_value = (int)strtol(value, NULL, 0);
            if (new_value != previous) {
                resources_set_int("SoundBufferSize", new_value);
            }
            lib_free(value);
        }
    } else {
        sprintf(buf, "%i msec", previous);
        return buf;
    }
    return NULL;
}

static UI_MENU_CALLBACK(custom_frequency_callback)
{
    static char buf[20];
    char *value = NULL;
    int previous, new_value;

    resources_get_int("SoundSampleRate", &previous);

    if (activated) {
        sprintf(buf, "%i", previous);
        value = sdl_ui_text_input_dialog("Enter frequency in Hz", buf);
        if (value) {
            new_value = (int)strtol(value, NULL, 0);
            if (new_value != previous) {
                resources_set_int("SoundSampleRate", new_value);
            }
            lib_free(value);
        }
    } else {
        if (previous != 22050 && previous != 44100 && previous != 48000) {
            sprintf(buf, "%i Hz", previous);
            return buf;
        }
    }
    return NULL;
}

static ui_menu_entry_t sound_output_dyn_menu[SOUND_DEVICE_PLAYBACK_MAX + 1];

static int sound_output_dyn_menu_init = 0;

static void sdl_menu_sound_output_free(void)
{
    int i;

    for (i = 0; sound_output_dyn_menu[i].string != NULL; i++) {
        lib_free(sound_output_dyn_menu[i].string);
        lib_free(sound_output_dyn_menu[i].data);
    }
}

static UI_MENU_CALLBACK(SoundOutput_dynmenu_callback)
{
    sound_desc_t *devices = sound_get_valid_devices(SOUND_PLAYBACK_DEVICE, 1);
    int i;

    /* rebuild menu if it already exists. */
    if (sound_output_dyn_menu_init != 0) {
        sdl_menu_sound_output_free();
    } else {
        sound_output_dyn_menu_init = 1;
    }

    for (i = 0; devices[i].name; ++i) {
        sound_output_dyn_menu[i].action   = ACTION_NONE;
        sound_output_dyn_menu[i].string   = lib_strdup(devices[i].description);
        sound_output_dyn_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        sound_output_dyn_menu[i].callback = radio_SoundDeviceName_callback;
        sound_output_dyn_menu[i].data     = (ui_callback_data_t)lib_strdup(devices[i].name);
    }
    sound_output_dyn_menu[i].string = NULL;

    lib_free(devices);

    return MENU_SUBMENU_STRING;
}

static ui_menu_entry_t sound_output_mode_menu[] = {
    {   .string   = "System",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundOutput_callback,
        .data     = (ui_callback_data_t)SOUND_OUTPUT_SYSTEM
    },
    {   .string   = "Mono",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundOutput_callback,
        .data     = (ui_callback_data_t)SOUND_OUTPUT_MONO
    },
    {   .string   = "Stereo",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundOutput_callback,
        .data     = (ui_callback_data_t)SOUND_OUTPUT_STEREO
    },
    SDL_MENU_LIST_END
};

static ui_menu_entry_t fragment_size_menu[] = {
    {   .string   = "Very small",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundFragmentSize_callback,
        .data     = (ui_callback_data_t)SOUND_FRAGMENT_VERY_SMALL
    },
    {   .string   = "Small",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundFragmentSize_callback,
        .data     = (ui_callback_data_t)SOUND_FRAGMENT_SMALL
    },
    {   .string   = "Medium",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundFragmentSize_callback,
        .data     = (ui_callback_data_t)SOUND_FRAGMENT_MEDIUM
    },
    {   .string   = "Large",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundFragmentSize_callback,
        .data     = (ui_callback_data_t)SOUND_FRAGMENT_LARGE
    },
    {   .string   = "Very large",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundFragmentSize_callback,
        .data     = (ui_callback_data_t)SOUND_FRAGMENT_VERY_LARGE
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t sound_output_menu[] = {
    {   .string   = "Sound",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_Sound_callback
    },
    {   .string   = "Emulate in warp mode",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SoundEmulateOnWarp_callback
    },
    {   .string   = "Volume",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_volume_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Output driver",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = SoundOutput_dynmenu_callback,
        .data     = (ui_callback_data_t)sound_output_dyn_menu
    },
    {   .string   = "Output Mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)sound_output_mode_menu
    },
    {   .string   = "Buffer size",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_buffer_size_callback
    },
    {   .string   = "Fragment size",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)fragment_size_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Frequency"),
    {   .string   = "22050 Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundSampleRate_callback,
        .data     = (ui_callback_data_t)22050
    },
    {   .string   = "44100 Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundSampleRate_callback,
        .data     = (ui_callback_data_t)44100
    },
    {   .string   = "48000 Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SoundSampleRate_callback,
        .data     = (ui_callback_data_t)48000
    },
    {   .string   = "Custom frequency",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_frequency_callback
    },
    SDL_MENU_LIST_END
};

/** \brief  Clean up memory used by the dynamically created sound output menus
 */
void uisound_output_menu_shutdown(void)
{
    if (sound_output_dyn_menu_init) {
        sdl_menu_sound_output_free();
    }
}
