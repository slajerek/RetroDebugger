/*
 * menu_debug.c - Debug menu for SDL UI.
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
#include "debug.h"

#ifdef DEBUG

#include <stdio.h>

#include "types.h"
#include "vice_sdl.h"

#include "lib.h"
#include "menu_common.h"
#include "menu_debug.h"
#include "resources.h"
#include "uiactions.h"
#include "uimenu.h"

static UI_MENU_CALLBACK(custom_auto_playback_frames)
{
    static char buf[20];
    char *value = NULL;
    int previous, new_value;

    resources_get_int("AutoPlaybackFrames", &previous);

    if (activated) {
        sprintf(buf, "%i", previous);
        value = sdl_ui_text_input_dialog("Enter number of auto playback frames", buf);
        if (value) {
            new_value = (int)strtol(value, NULL, 0);
            if (new_value != previous) {
                resources_set_int("AutoPlaybackFrames", new_value);
            }
            lib_free(value);
        }
    } else {
        sprintf(buf, "%i", previous);
        return buf;
    }
    return NULL;
}

#if 0
UI_MENU_DEFINE_TOGGLE(MainCPU_TRACE)
UI_MENU_DEFINE_TOGGLE(IEC_TRACE)
UI_MENU_DEFINE_TOGGLE(IEEE_TRACE)
UI_MENU_DEFINE_TOGGLE(Drive0CPU_TRACE)
UI_MENU_DEFINE_TOGGLE(Drive1CPU_TRACE)
UI_MENU_DEFINE_TOGGLE(Drive2CPU_TRACE)
UI_MENU_DEFINE_TOGGLE(Drive3CPU_TRACE)
#endif
UI_MENU_DEFINE_RADIO(TraceMode)

static UI_MENU_CALLBACK(show_font_callback)
{
    int active = 1, refresh = 1, uppercase = MENU_FONT_ASCII;
    int i, j;
    char fontchars[] = "0123456789abcdef";
    char *modestr = "";

    if (activated) {
        while (refresh) {
            sdl_ui_clear();
            sdl_ui_print("   0123456789ABCDEF", 0, 0);
            for (j = 0; j < 16; ++j) {
                fontchars[0] = "0123456789ABCDEF"[j];
                fontchars[1] = 'x';
                fontchars[2] = 0;
                sdl_ui_print(fontchars, 0, 1 + j);
            }
            switch (uppercase) {
                case MENU_FONT_ASCII:
                    sdl_ui_set_active_font(MENU_FONT_ASCII);
                    modestr = "menu/ascii";
                    break;
                case MENU_FONT_IMAGES:
                    sdl_ui_set_active_font(MENU_FONT_IMAGES);
                    modestr = "image/petscii";
                    break;
                case MENU_FONT_MONITOR:
                    sdl_ui_set_active_font(MENU_FONT_MONITOR);
                    modestr = "monitor/petscii";
                    break;
            }
            sdl_ui_print("\xff\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 3, 1);
            for (j = 1; j < 16; ++j) {
                for (i = 0; i < 16; ++i) {
                    fontchars[i] = (char)(j * 16 + i);
                }
                sdl_ui_print(fontchars, 3, 1 + j);
            }
            sdl_ui_set_active_font(MENU_FONT_ASCII);
            sdl_ui_print(modestr, 0, 1 + 17);
            sdl_ui_refresh();
            active = 1;
            while (active) {
                switch (sdl_ui_menu_poll_input()) {
                    case MENU_ACTION_CANCEL:
                    case MENU_ACTION_EXIT:
                        active = 0;
                        refresh = 0;
                        break;
                    case MENU_ACTION_LEFT:
                        active = 0;
                        uppercase = MENU_FONT_ASCII;
                        break;
                    case MENU_ACTION_RIGHT:
                        active = 0;
                        uppercase = MENU_FONT_IMAGES;
                        break;
                    case MENU_ACTION_DOWN:
                        active = 0;
                        uppercase = MENU_FONT_MONITOR;
                        break;
                    default:
                        SDL_Delay(20);
                        break;
                }
            }
        }
        sdl_ui_set_active_font(MENU_FONT_ASCII);
    }
    return NULL;
}

const ui_menu_entry_t debug_menu[] = {
    SDL_MENU_ITEM_TITLE("Trace mode"),
    {   .string   = "Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_NORMAL
    },
    {   .string   = "Small",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_SMALL
    },
    {   .string   = "History",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_HISTORY
    },
    {   .string   = "History autoplay",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_AUTOPLAY
    },

    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
        .string   = "Auto playback frames",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_auto_playback_frames
    },

    SDL_MENU_ITEM_SEPARATOR,

    {   .action    = ACTION_DEBUG_TRACE_CPU_TOGGLE,
        .string    = "Main CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "MainCPU_TRACE",
    },
    {   .action    = ACTION_DEBUG_TRACE_IEC_TOGGLE,
        .string    = "IEC bus trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "IEC_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_IEEE488_TOGGLE,
        .string    = "IEEE-488 bus trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "IEEE_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE,
        .string    = "Drive0 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "Drive0CPU_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE,
        .string    = "Drive1 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "Drive1CPU_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE,
        .string    = "Drive2 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "Drive2CPU_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE,
        .string    = "Drive3 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "Drive3CPU_TRACE"
    },

    SDL_MENU_ITEM_SEPARATOR,
    {   .string   = "Show font",
        .type     = MENU_ENTRY_DIALOG,
        .callback = show_font_callback,
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t debug_menu_dtv[] = {
    SDL_MENU_ITEM_TITLE("Trace mode"),
    {   .string   = "Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_NORMAL
    },
    {   .string   = "Small",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_SMALL
    },
    {   .string   = "History",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_HISTORY
    },
    {   .string   = "History autoplay",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_TraceMode_callback,
        .data     = (ui_callback_data_t)DEBUG_AUTOPLAY
    },

    SDL_MENU_ITEM_SEPARATOR,

    {   .action    = ACTION_DEBUG_TRACE_CPU_TOGGLE,
        .string    = "Main CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "MainCPU_TRACE"
    },

    SDL_MENU_ITEM_SEPARATOR,

    {   .action    = ACTION_DEBUG_TRACE_IEC_TOGGLE,
        .string    = "IEC bus trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "IEC_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE,
        .string    = "Drive0 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DRIVE0CPU_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE,
        .string    = "Drive1 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DRIVE1CPU_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE,
        .string    = "Drive2 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DRIVE2CPU_TRACE"
    },
    {   .action    = ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE,
        .string    = "Drive3 CPU trace",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DRIVE3CPU_TRACE"
    },

    SDL_MENU_ITEM_SEPARATOR,

    {   .action    = ACTION_DEBUG_BLITTER_LOG_TOGGLE,
        .string    = "Blitter Log",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DtvBlitterLog"
    },
    {   .action    = ACTION_DEBUG_DMA_LOG_TOGGLE,
        .string    = "DMA Log",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DtvDmaLog"
    },
    {   .action    = ACTION_DEBUG_FLASH_LOG_TOGGLE,
        .string    = "Flash Log",
        .type      = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource  = "DtvFlashLog"
    },

    SDL_MENU_ITEM_SEPARATOR,

    {   .string    = "Show font",
        .type      = MENU_ENTRY_DIALOG,
        .callback  = show_font_callback
    },
    SDL_MENU_LIST_END
};

#endif
