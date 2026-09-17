/*
 * xpet_ui.c - Implementation of the PET-specific part of the UI.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#include "debug.h"
#include "actions-display.h"
#include "actions-speed.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_debug.h"
#include "menu_drive.h"
#include "menu_edit.h"
#include "menu_ffmpeg.h"
#include "menu_help.h"
#include "menu_jam.h"
#include "menu_joyport.h"
#include "menu_joystick.h"
#include "menu_log.h"
#include "menu_media.h"
#include "menu_monitor.h"
#include "menu_network.h"
#include "menu_petcart.h"
#include "menu_pethw.h"
#include "menu_printer.h"
#include "menu_reset.h"
#include "menu_sampler.h"
#include "menu_screenshot.h"
#include "menu_settings.h"
#include "menu_sid.h"
#include "menu_snapshot.h"
#include "menu_sound.h"
#include "menu_speed.h"
#include "menu_tape.h"
#include "menu_userport.h"
#include "menu_video.h"
#include "pet.h"
#include "petmem.h"
#include "petrom.h"
#include "pets.h"
#include "petui.h"
#include "pet-resources.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uifonts.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "vkbd.h"


static ui_menu_entry_t xpet_main_menu[] = {
    {   .action    = ACTION_SMART_ATTACH,
        .string    = "Autostart image",
        .type      = MENU_ENTRY_DIALOG,
        .callback  = autostart_callback,
        .activated = MENU_EXIT_UI_STRING
    },
    {   .string   = "Drive",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)drive_menu
    },
    {   .string   = "Tape",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)tape_pet_menu
    },
    {   .string   = "Cartridge",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)petcart_menu
    },
    {   .string   = "Printer",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)printer_ieee_menu
    },
    {   .string   = "Machine settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)pet_hardware_menu
    },
    {   .string   = "Video settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)pet_video_menu
    },
    {   .string   = "Sound settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sound_output_menu
    },
    {   .string   = "Sampler settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)sampler_menu
    },
    {   .string   = "Snapshot",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)snapshot_menu
    },
    {   .string   = "Save media file",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)media_menu
    },
    {   .string   = "Speed settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)speed_menu
    },
    {   .string   = "Reset",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)reset_menu
    },
    {   .string   = "Action on CPU JAM",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)jam_menu
    },
#ifdef HAVE_NETWORK
    {   .string   = "Network",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)network_menu
    },
#endif
    {   .action    = ACTION_PAUSE_TOGGLE,
        .string    = "Pause",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = pause_toggle_display
    },
    {   .action   = ACTION_ADVANCE_FRAME,
        .string   = "Advance Frame",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Monitor",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)monitor_menu
    },
    {   .action    = ACTION_VIRTUAL_KEYBOARD,
        .string    = "Virtual keyboard",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {   .action    = ACTION_SHOW_STATUSBAR_TOGGLE,
        .string    = "Statusbar",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = show_statusbar_toggle_display
    },
#ifdef DEBUG
    {   .string   = "Debug",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)debug_menu
    },
#endif
    {   .string   = "Help",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)help_menu
    },
    {   .string   = "Settings management",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)settings_manager_menu
    },
    {   .string   = "Log settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)log_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Edit",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)edit_menu
    },
#endif
    {   .action   = ACTION_QUIT,
        .string   = "Quit emulator",
        .type     = MENU_ENTRY_OTHER,
    },
    SDL_MENU_LIST_END
};

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

static UI_MENU_CALLBACK(custom_cb2_lowpass_filter_callback)
{
    static char buf[20];
    char *value = NULL;
    int previous, new_value;

    resources_get_int("CB2Lowpass", &previous);

    if (activated) {
        sprintf(buf, "%i", previous);
        value = sdl_ui_text_input_dialog("Enter cutoff frequency in Hz (1.."
                                         STRINGIFY(SOUND_SAMPLE_RATE)
                                         ")", buf);
        if (value) {
            new_value = (int)strtol(value, NULL, 0);
            if (new_value != previous &&
                new_value >= 1 &&
                new_value <= SOUND_SAMPLE_RATE) {
                resources_set_int("CB2Lowpass", new_value);
            }
            lib_free(value);
        }
    } else {
        sprintf(buf, "%i Hz", previous);
        return buf;
    }
    return NULL;
}

const ui_menu_entry_t pet_cb2_lowpass =
    {   .string   = "CB2 lowpass filter",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_cb2_lowpass_filter_callback,
    };

/* FIXME: support all PET keyboards (see pet-resources.h) */

static void petui_set_menu_params(int index, menu_draw_t *menu_draw)
{
    static int old_keymap = -1;
    int cols = petmem_get_rom_columns();
    int keymap;

    menu_draw->max_text_x = cols ? cols : 40;
    menu_draw->extra_x = 24;
    menu_draw->extra_y = (cols == 40) ? 32 : 20;

    menu_draw->max_text_y = (cols == 40) ? 26 : 28;

    resources_get_int("KeyboardType", &keymap);

    if (keymap != old_keymap) {
        if (keymap == KBD_TYPE_GRAPHICS_US) {
            sdl_vkbd_set_vkbd(&vkbd_pet_gr);
        } else {
            sdl_vkbd_set_vkbd(&vkbd_pet_uk);
        }
        old_keymap = keymap;
    }

#define RGB_encode(r,g,b) (((r)<<5)|((g)<<2)|(b))

    /* CRTC */
    switch (pet_colour_type) {
        case PET_COLOUR_TYPE_RGBI:
            menu_draw->color_front = menu_draw->color_default_front = 15;
            menu_draw->color_back = menu_draw->color_default_back = 0;
            menu_draw->color_cursor_back = 3;
            menu_draw->color_cursor_revers = 0;
            menu_draw->color_active_green = 4;
            menu_draw->color_inactive_red = 8;
            menu_draw->color_active_grey = 14;
            menu_draw->color_inactive_grey = 1;
            break;
        case PET_COLOUR_TYPE_ANALOG:
            menu_draw->color_front = menu_draw->color_default_front = RGB_encode(7,7,3);
            menu_draw->color_back = menu_draw->color_default_back = RGB_encode(0,0,0);
            menu_draw->color_cursor_back = RGB_encode(0,0,3);
            menu_draw->color_cursor_revers = RGB_encode(0,0,0);
            menu_draw->color_active_green = RGB_encode(0,7,0);
            menu_draw->color_inactive_red = RGB_encode(7,0,0);
            menu_draw->color_active_grey = RGB_encode(4,4,2);
            menu_draw->color_inactive_grey = RGB_encode(3,3,1);
            break;
        default:
            menu_draw->color_front = menu_draw->color_default_front = 1;
            menu_draw->color_back = menu_draw->color_default_back = 0;
            menu_draw->color_cursor_back = 0;
            menu_draw->color_cursor_revers = 1;
            menu_draw->color_active_green = 1;
            menu_draw->color_inactive_red = 1;
            menu_draw->color_active_grey = 1;
            menu_draw->color_inactive_grey = 1;
            break;
    }
}

/** \brief  Pre-initialize the UI before the canvas window gets created
 *
 * \return  0 on success, -1 on failure
 */
int petui_init_early(void)
{
    return 0;
}

static int patched_main_menu_item = -1;

/** \brief  Adapt the Sound menu by insterting a PET-only item.
 *
 * \return  void
 */
static void pet_sound_menu_fixup(void)
{
    int num_items = 0;
    const ui_menu_entry_t *menu = sound_output_menu;
    ui_menu_entry_t *new_menu;
    int i, j;

    /* Count the size of the sound_output_menu */
    while (menu[num_items].string != NULL) {
        ++num_items;
    }

    /* Allocate a new version, 1 item bigger, 1 terminator */
    new_menu = lib_calloc(num_items + 2, sizeof(ui_menu_entry_t));

    /* Insert a new item into it, while copying all original items */
    for (i = j = 0; menu[i].string; i++, j++) {
        new_menu[j] = menu[i];

        /* Insert our new item after "Volume" */
        if (strcmp(menu[i].string, "Volume") == 0) {
            j++;
            new_menu[j] = pet_cb2_lowpass;
        }
    }

    /* Copy terminating entry */
    new_menu[j] = menu[i];

    /* Replace the old sound_output_menu (in the main menu) with our new one */
    for (i = 0; xpet_main_menu[i].string; i++) {
        if (xpet_main_menu[i].data == (ui_callback_data_t)menu) {
            xpet_main_menu[i].data = (ui_callback_data_t)new_menu;
            patched_main_menu_item = i;
            break;
        }
    }
}

/** \brief  Undo the effect of pet_sound_menu_fixup().
 *
 * \return  void
 */
static void pet_sound_menu_shutdown(void)
{
    if (patched_main_menu_item >= 0) {
        lib_free(xpet_main_menu[patched_main_menu_item].data);
        xpet_main_menu[patched_main_menu_item].data =
            (ui_callback_data_t)sound_output_menu;
        patched_main_menu_item = -1;
    }
}

/** \brief  Initialize the UI
 *
 * \return  0 on success, -1 on failure
 */
int petui_init(void)
{

#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif

    sdl_ui_set_menu_params = petui_set_menu_params;
    uijoyport_menu_create(0, 0, 1, 1, 1, 0);
    uiuserport_menu_create(1);
    uisampler_menu_create();
    uidrive_menu_create(1);
    uitape_menu_create(1);
    uikeyboard_menu_create();
    uipalette_menu_create("Crtc", NULL);
    uisid_menu_create();
    uimedia_menu_create();
    pet_sound_menu_fixup();

    sdl_ui_set_main_menu(xpet_main_menu);
    sdl_ui_font_init(PET_CHARGEN2_NAME, 0, 0x400, 0);

    sdl_menu_ffmpeg_init();

    uistatusbar_realize();
    return 0;
}

void petui_shutdown(void)
{
#ifdef SDL_DEBUG
    fprintf(stderr, "%s\n", __func__);
#endif
    uisound_output_menu_shutdown();
    uikeyboard_menu_shutdown();
    uisid_menu_shutdown();
    uipalette_menu_shutdown();
    uijoyport_menu_shutdown();
    uijoystick_menu_shutdown();
    uiuserport_menu_shutdown();
    uitapeport_menu_shutdown();
    uimedia_menu_shutdown();
    pet_sound_menu_shutdown();
    sdl_menu_ffmpeg_shutdown();
    sdl_ui_font_shutdown();
}
