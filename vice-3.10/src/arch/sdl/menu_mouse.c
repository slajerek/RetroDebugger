/*
 * menu_mouse.c - Mouse menu for SDL UI.
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

#ifdef HAVE_MOUSE

#include "menu_common.h"
#include "uiactions.h"
#include "uimenu.h"


UI_MENU_DEFINE_TOGGLE(SmartMouseRTCSave)
UI_MENU_DEFINE_TOGGLE(ps2mouse)

const ui_menu_entry_t mouse_menu[] = {
    {   .action   = ACTION_MOUSE_GRAB_TOGGLE,
        .string   = "Grab mouse events",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "Mouse"
    },
    SDL_MENU_ITEM_SEPARATOR,
    {   .string   = "Save Smart Mouse RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SmartMouseRTCSave_callback
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t mouse_grab_menu[] = {
    {   .action   = ACTION_MOUSE_GRAB_TOGGLE,
        .string   = "Grab mouse events",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "Mouse"
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t mouse_c64dtv_menu[] = {
    {   .string   = "Enable PS/2 mouse on userport",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ps2mouse_callback
    },
    SDL_MENU_ITEM_SEPARATOR,
    {   .action   = ACTION_MOUSE_GRAB_TOGGLE,
        .string   = "Grab mouse events",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "Mouse"
    },
    SDL_MENU_LIST_END
};

#endif
