/** \file   actions-display.c
 * \brief   UI action implementations for display-related dialogs and settings (SDL)
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
#include <stdbool.h>

#include "menu_common.h"
#include "resources.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "videoarch.h"
#include "vicii.h"
#include "vkbd.h"

#include "actions-display.h"

/** \brief  Activate virtual keyboard action
 *
 * \param[in]   self    action map
 */
static void virtual_keyboard_action(ui_action_map_t *self)
{
    sdl_vkbd_activate();
}

/** \brief  Get video chip name for canvas
 *
 * \param[in]   index   canvas index
 *
 * \return  chip name
 */
static const char *get_chip_name_for_canvas(int index)
{
    const char *chip_name;
#ifdef USE_SDL2UI
    video_canvas_t *canvas = sdl2_get_canvas_from_index(index);
    chip_name = canvas->videoconfig->chip_name;
#else
    /* Dumb hack for SDL1: no dual canvases and switching canvases just to get
     * a chip name seems like a bad idea. So we just hardcode the chip names
     * here: */
    if (index > 0) {
        chip_name = "VDC";  /* only one emu with dual display */
    } else {
        switch (machine_class) {
            case VICE_MACHINE_C128:     /* fall through */
            case VICE_MACHINE_C64:      /* fall through */
            case VICE_MACHINE_C64SC:    /* fall through */
            case VICE_MACHINE_C64DTV:   /* fall through */
            case VICE_MACHINE_SCPU64:   /* fall through */
            case VICE_MACHINE_CBM5x0:
                chip_name = "VICII";
                break;
            case VICE_MACHINE_VIC20:
                chip_name = "VIC";
                break;
            case VICE_MACHINE_PLUS4:
                chip_name = "TED";
                break;
            case VICE_MACHINE_CBM6x0:   /* fall through */
            case VICE_MACHINE_PET:
                chip_name = "CRTC";
                break;
            default:
                chip_name = NULL;
        }
    }
#endif
    return chip_name;
}

/** \brief  Get resource value for ShowStatusbar for a canvas
 *
 * \param[in]   index   canvas index
 *
 * \return  show-statusbar state
 */
static bool get_show_statusbar_for_canvas(int index)
{
    int         show = 0;
    const char *chip_name = get_chip_name_for_canvas(index);

    resources_get_int_sprintf("%sShowStatusbar", &show, chip_name);
    return show ? true : false;
}

/** \brief  Set resource value for {CHIP}ShowStatusbar for a canvas
 *
 * \param[in]   index   canvas index
 * \param[in]   show    new value for resource
 */
static void set_show_statusbar_for_canvas(int index, bool show)
{
    const char *chip_name = get_chip_name_for_canvas(index);

    resources_set_int_sprintf("%sShowStatusbar", (int)show, chip_name);
}

/** \brief  Toggle {CHIP}ShowStatusbar resource action
 *
 * \param[in]   self    action map
 */
static void show_statusbar_toggle_action(ui_action_map_t *self)
{
    int  index = vice_ptr_to_int(self->data);
    bool show  = get_show_statusbar_for_canvas(index);

    show = !show;
    set_show_statusbar_for_canvas(index, show);
    if (show) {
        if (index == 0) {
            uistatusbar_open();
        } else {
            uistatusbar_open_vdc();
        }
    } else {
        if (index == 0) {
            uistatusbar_close();
        } else {
            uistatusbar_close_vdc();
        }
    }
}

/** \brief  Set border mode action
 *
 * \param[in]   self    action map
 */
static void border_mode_action(ui_action_map_t *self)
{
    /* VDC doesn't have the BorderMode resource, so we can just use 0 here */
    const char *chip = get_chip_name_for_canvas(0);

    resources_set_int_sprintf("%sBorderMode", vice_ptr_to_int(self->data), chip);
}

/** \brief  Restore display action
 *
 * Resize display (window) to optimal size for emulated screen and render options.
 *
 *  \param[in]   self    action map
 */
static void restore_display_action(ui_action_map_t *self)
{
    sdl_video_restore_size();
}

/** \brief  Toggle fullscreen action
 *
 * \param[in]   self    action map
 */
static void fullscreen_toggle_action(ui_action_map_t *self)
{
    const char *chip_name  = sdl_active_canvas->videoconfig->chip_name;
    int         fullscreen = 0;

    resources_get_int_sprintf("%sFullscreen", &fullscreen, chip_name);
    resources_set_int_sprintf("%sFullscreen", !fullscreen, chip_name);
}


/** \brief  List of mappings for display-related actions */
static const ui_action_map_t display_actions[] = {
    {   .action  = ACTION_VIRTUAL_KEYBOARD,
        .handler = virtual_keyboard_action
    },
    {   .action  = ACTION_SHOW_STATUSBAR_TOGGLE,
        .handler = show_statusbar_toggle_action,
        .data    = vice_int_to_ptr(0)   /* canvas index */
    },
    {   .action  = ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE,
        .handler = show_statusbar_toggle_action,
        .data    = vice_int_to_ptr(1)   /* canvas index */
    },

    /* Border modes (we use the VICII constants here, the TED and VIC constants
     * map to the same values) */
    {   .action  = ACTION_BORDER_MODE_NORMAL,
        .handler = border_mode_action,
        .data    = vice_int_to_ptr(VICII_NORMAL_BORDERS)
    },
    {   .action  = ACTION_BORDER_MODE_FULL,
        .handler = border_mode_action,
        .data    = vice_int_to_ptr(VICII_FULL_BORDERS)
    },
    {   .action  = ACTION_BORDER_MODE_DEBUG,
        .handler = border_mode_action,
        .data    = vice_int_to_ptr(VICII_DEBUG_BORDERS)
    },
    {   .action  = ACTION_BORDER_MODE_NONE,
        .handler = border_mode_action,
        .data    = vice_int_to_ptr(VICII_NO_BORDERS)
    },

    {   .action  = ACTION_RESTORE_DISPLAY,
        .handler = restore_display_action
    },
    {   .action  = ACTION_FULLSCREEN_TOGGLE,
        .handler = fullscreen_toggle_action
    },

    UI_ACTION_MAP_TERMINATOR
};



/** \brief  Register display-related actions */
void actions_display_register(void)
{
    ui_actions_register(display_actions);
}


/** \brief  Display helper for show-statusbar-[secondary-]toggle action
 *
 * \param[in]   item    menu item
 *
 * \return  string to display
 */
const char *show_statusbar_toggle_display(ui_menu_entry_t *item)
{
    int index = 0;

    /* determine which canvas we need */
    if (item->action == ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE) {
        index = 1;
    }
    return get_show_statusbar_for_canvas(index) ? sdl_menu_text_tick : NULL;
}
