/*
 * menu_screenshot.c - SDL screenshot saving functions.
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

#include <stdlib.h>

#include "gfxoutput.h"
#include "lib.h"
#include "log.h"
#include "menu_common.h"
#include "menu_ffmpeg.h"
#include "menu_screenshot.h"
#include "resources.h"
#include "screenshot.h"
#include "types.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "util.h"
#include "videoarch.h"

UI_MENU_DEFINE_RADIO(QuicksaveScreenshotFormat)
UI_MENU_DEFINE_RADIO(OCPOversizeHandling)
UI_MENU_DEFINE_RADIO(OCPUndersizeHandling)
UI_MENU_DEFINE_RADIO(OCPMultiColorHandling)
UI_MENU_DEFINE_RADIO(OCPTEDLumHandling)
UI_MENU_DEFINE_RADIO(KoalaOversizeHandling)
UI_MENU_DEFINE_RADIO(KoalaUndersizeHandling)
UI_MENU_DEFINE_RADIO(KoalaTEDLumHandling)
UI_MENU_DEFINE_RADIO(MinipaintOversizeHandling)
UI_MENU_DEFINE_RADIO(MinipaintUndersizeHandling)
UI_MENU_DEFINE_RADIO(MinipaintTEDLumHandling)

static UI_MENU_CALLBACK(save_screenshot_callback)
{
    char title[32];
    char *filename;
    uint8_t *srcbuf;

    if (activated) {
        sprintf(title, "Choose %s file", (char *)param);
        filename = sdl_ui_file_selection_dialog(title, FILEREQ_MODE_SAVE_FILE);
        if (filename != NULL) {
            util_add_extension(&filename, screenshot_get_fext_for_format((char *)param));

            srcbuf = sdl_ui_get_draw_buffer();
            if (srcbuf != NULL) {
                memcpy(sdl_active_canvas->draw_buffer->draw_buffer, srcbuf,
                    sdl_active_canvas->draw_buffer->draw_buffer_width *
                    sdl_active_canvas->draw_buffer->draw_buffer_height);
            }

            if (screenshot_save((char *) param, filename, sdl_active_canvas) < 0) {
                log_error(LOG_DEFAULT, "Failed to save screenshot.");
                ui_error("Failed to save screenshot.");
            } else {
                log_message(LOG_DEFAULT, "Saved %s screenshot to %s", (char *) param, filename);
            }
            lib_free(filename);
        }
    }
    return NULL;
}


static const ui_menu_entry_t koala_tedlum_handling_menu[] = {
    {   .string   = "Ignore",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaTEDLumHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_TED_LUM_IGNORE
    },
    {   .string   = "Best cell colors",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaTEDLumHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_TED_LUM_DITHER
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t koala_undersize_handling_menu[] = {
    {   .string   = "Scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaUndersizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_UNDERSIZE_SCALE
    },
    {   .string   = "Borderize",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaUndersizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_UNDERSIZE_BORDERIZE
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t koala_oversize_handling_menu[] = {
    {   .string   = "Scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_SCALE
    },
    {   .string   = "Crop left top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_TOP
    },
    {   .string   = "Crop middle top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER_TOP
    },
    {   .string   = "Crop right top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP
    },
    {   .string   = "Crop left center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER
    },
    {   .string   = "Crop middle center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER
    },
    {   .string   = "Crop right center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER
    },
    {   .string   = "Crop left bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM
    },
    {   .string   = "Crop middle bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM
    },
    {   .string   = "Crop right bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KoalaOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t minipaint_tedlum_handling_menu[] = {
    {   .string   = "Ignore",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintTEDLumHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_TED_LUM_IGNORE
    },
    {   .string   = "Best cell colors",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintTEDLumHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_TED_LUM_DITHER
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t minipaint_undersize_handling_menu[] = {
    {   .string   = "Scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintUndersizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_UNDERSIZE_SCALE
    },
    {   .string   = "Borderize",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintUndersizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_UNDERSIZE_BORDERIZE
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t minipaint_oversize_handling_menu[] = {
    {   .string   = "Scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_SCALE
    },
    {   .string   = "Crop left top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_TOP
    },
    {   .string   = "Crop middle top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER_TOP
    },
    {   .string   = "Crop right top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP
    },
    {   .string   = "Crop left center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER
    },
    {   .string   = "Crop middle center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER
    },
    {   .string   = "Crop right center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER
    },
    {   .string   = "Crop left bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM
    },
    {   .string   = "Crop middle bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM
    },
    {   .string   = "Crop right bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MinipaintOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_tedlum_handling_menu[] = {
    {   .string   = "Ignore",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPTEDLumHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_TED_LUM_IGNORE
    },
    {   .string   = "Best cell colors",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPTEDLumHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_TED_LUM_DITHER
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_multicolor_handling_menu[] = {
    {   .string   = "Black & white",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPMultiColorHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_MC2HR_BLACK_WHITE
    },
    {   .string   = "2 colors",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPMultiColorHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_MC2HR_2_COLORS
    },
    {   .string   = "4 colors",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPMultiColorHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_MC2HR_4_COLORS
    },
    {   .string   = "Gray scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPMultiColorHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_MC2HR_GRAY
    },
    {   .string   = "Best cell colors",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPMultiColorHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_MC2HR_DITHER
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_undersize_handling_menu[] = {
    {   .string   = "Scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPUndersizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_UNDERSIZE_SCALE
    },
    {   .string   = "Borderize",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPUndersizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_UNDERSIZE_BORDERIZE
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_oversize_handling_menu[] = {
    {   .string   = "Scale",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_SCALE
    },
    {   .string   = "Crop left top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_TOP
    },
    {   .string   = "Crop middle top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER_TOP
    },
    {   .string   = "Crop right top",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP
    },
    {   .string   = "Crop left center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER
    },
    {   .string   = "Crop middle center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER
    },
    {   .string   = "Crop right center",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER
    },
    {   .string   = "Crop left bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM
    },
    {   .string   = "Crop middle bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM
    },
    {   .string   = "Crop right bottom",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_OCPOversizeHandling_callback,
        .data     = (ui_callback_data_t)NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t koala_settings_vic_vicii_vdc_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_undersize_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t koala_settings_ted_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_undersize_handling_menu
    },
    {   .string   = "TED luminosity handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_tedlum_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t koala_settings_crtc_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)koala_undersize_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t minipaint_settings_vic_vicii_vdc_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_undersize_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t minipaint_settings_ted_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_undersize_handling_menu
    },
    {   .string   = "TED luminosity handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_tedlum_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t minipaint_settings_crtc_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)minipaint_undersize_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_settings_vic_vicii_vdc_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_undersize_handling_menu
    },
    {   .string   = "Multicolor handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_multicolor_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_settings_ted_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_undersize_handling_menu
    },
    {   .string   = "Multicolor handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_multicolor_handling_menu
    },
    {   .string   = "TED luminosity handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_tedlum_handling_menu
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t artstudio_settings_crtc_menu[] = {
    {   .string   = "Oversize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_oversize_handling_menu
    },
    {   .string   = "Undersize handling",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)artstudio_undersize_handling_menu
    },
    SDL_MENU_LIST_END
};


static ui_menu_entry_t *quicksave_format_selection_menu = NULL;
ui_menu_entry_t *screenshot_vic_vicii_vdc_menu = NULL;
ui_menu_entry_t *screenshot_ted_menu = NULL;
ui_menu_entry_t *screenshot_crtc_menu = NULL;


static void ui_menu_append_item(
    ui_menu_entry_t **menu, size_t *nitems, size_t *nmax,
    char *string,
    ui_menu_entry_type_t type,
    ui_callback_t callback,
    ui_callback_data_t data)
{
    ui_menu_entry_t *entry;

    if (*nitems + 1 >= *nmax) {
        *nmax += 16;
        *menu = lib_realloc(*menu, sizeof(ui_menu_entry_t) * (*nmax));
        if (*menu == NULL)
            return;
    }

    entry = (*menu) + *nitems;
    entry->action   = ACTION_NONE;
    entry->string   = string != NULL ? lib_strdup(string) : string;
    entry->type     = type;
    entry->callback = callback;
    entry->data     = data;

    (*nitems)++;
}


static void ui_menu_append_separator(
    ui_menu_entry_t **menu, size_t *nitems, size_t *nmax)
{
    ui_menu_append_item(menu, nitems, nmax,
        "",
        MENU_ENTRY_TEXT,
        seperator_callback,
        NULL);
}


static void ui_menu_append_end(
    ui_menu_entry_t **menu, size_t *nitems, size_t *nmax)
{
    ui_menu_append_item(menu, nitems, nmax, NULL, MENU_ENTRY_TEXT, NULL, NULL);
}


static void uiscreenshot_menu_append_drivers(
    ui_menu_entry_t **menu, size_t *nitems, size_t *nmax,
    const int driver_type,
    const char *prefix,
    const char *strip,
    ui_menu_entry_type_t type,
    ui_callback_t callback)
{
    char tmp[64], *pos;
    gfxoutputdrv_t *driver = gfxoutput_drivers_iter_init();

    while (driver != NULL) {
        if (driver->type == driver_type) {
            snprintf(tmp, sizeof(tmp), "%s%s", prefix, driver->displayname);

            /* XXX TODO: This is a bit stupid ... */
            if (strip != NULL && (pos = strstr(tmp, strip)) != NULL) {
                *pos = 0;
            }

            ui_menu_append_item(menu, nitems, nmax,
                tmp,
                type,
                callback,
                (ui_callback_t) driver->name);
        }
        driver = gfxoutput_drivers_iter_next();
    }
}


static void uiscreenshot_menu_append_drivers_order(
    ui_menu_entry_t **menu, size_t *nitems, size_t *nmax,
    const char *prefix,
    const char *strip,
    ui_menu_entry_type_t type,
    ui_callback_t callback)
{
    uiscreenshot_menu_append_drivers(menu, nitems, nmax,
        GFXOUTPUTDRV_TYPE_SCREENSHOT_NATIVE,
        prefix, strip, type, callback);

    ui_menu_append_separator(menu, nitems, nmax);

    uiscreenshot_menu_append_drivers(menu, nitems, nmax,
        GFXOUTPUTDRV_TYPE_SCREENSHOT_IMAGE,
        prefix, strip, type, callback);
}


static void uiscreenshot_menu_poop(ui_menu_entry_t **menu,
    ui_callback_data_t artstudio_data,
    ui_callback_data_t koala_data,
    ui_callback_data_t minipaint_data)
{
    size_t nitems, nmax;
    nitems = nmax = 0;

    ui_menu_append_item(menu, &nitems, &nmax,
        "Quicksave screenshot format",
        MENU_ENTRY_SUBMENU,
        submenu_radio_callback,
        quicksave_format_selection_menu);

    ui_menu_append_item(menu, &nitems, &nmax,
        "Artstudio screenshot settings",
        MENU_ENTRY_SUBMENU,
        submenu_callback,
        artstudio_data);

    ui_menu_append_item(menu, &nitems, &nmax,
        "Koalapainter screenshot settings",
        MENU_ENTRY_SUBMENU,
        submenu_callback,
        koala_data);

    ui_menu_append_item(menu, &nitems, &nmax,
        "Minipaint screenshot settings",
        MENU_ENTRY_SUBMENU,
        submenu_callback,
        minipaint_data);

    ui_menu_append_separator(menu, &nitems, &nmax);

    uiscreenshot_menu_append_drivers_order(menu, &nitems, &nmax,
        "Save ", NULL,
        MENU_ENTRY_DIALOG, save_screenshot_callback);

    ui_menu_append_end(menu, &nitems, &nmax);
}


void uiscreenshot_menu_create(void)
{
    size_t nitems, nmax;

    /* Create quicksave format selection submenu */
    nitems = nmax = 0;
    uiscreenshot_menu_append_drivers_order(
        &quicksave_format_selection_menu, &nitems, &nmax,
        "", " screenshot",
        MENU_ENTRY_RESOURCE_RADIO,
        radio_QuicksaveScreenshotFormat_callback);

    ui_menu_append_end(&quicksave_format_selection_menu, &nitems, &nmax);

    /* Create other menus */
    uiscreenshot_menu_poop(
        &screenshot_vic_vicii_vdc_menu,
        (ui_callback_data_t) artstudio_settings_vic_vicii_vdc_menu,
        (ui_callback_data_t) koala_settings_vic_vicii_vdc_menu,
        (ui_callback_data_t) minipaint_settings_vic_vicii_vdc_menu);

    uiscreenshot_menu_poop(
        &screenshot_ted_menu,
        (ui_callback_data_t) artstudio_settings_ted_menu,
        (ui_callback_data_t) koala_settings_ted_menu,
        (ui_callback_data_t) minipaint_settings_ted_menu);

    uiscreenshot_menu_poop(
        &screenshot_crtc_menu,
        (ui_callback_data_t) artstudio_settings_crtc_menu,
        (ui_callback_data_t) koala_settings_crtc_menu,
        (ui_callback_data_t) minipaint_settings_crtc_menu);
}


static void uiscreenshot_menu_free(ui_menu_entry_t *menu)
{
    if (menu) {
        ui_menu_entry_t *entry = menu;
        while (entry->string != NULL) {
            lib_free(entry->string);
            entry++;
        }
        lib_free(menu);
    }
}


void uiscreenshot_menu_shutdown(void)
{
    uiscreenshot_menu_free(quicksave_format_selection_menu);

    uiscreenshot_menu_free(screenshot_vic_vicii_vdc_menu);
    uiscreenshot_menu_free(screenshot_ted_menu);
    uiscreenshot_menu_free(screenshot_crtc_menu);
}
