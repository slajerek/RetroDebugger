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

#include <stdio.h>

#include "types.h"

#ifdef HAVE_MOUSE

#include "menu_common.h"
#include "mouse.h"
#include "uimenu.h"

UI_MENU_DEFINE_TOGGLE(Mouse)
UI_MENU_DEFINE_RADIO(Mousetype)
UI_MENU_DEFINE_RADIO(Mouseport)

const ui_menu_entry_t mouse_menu[] = {
    { "Enable mouse",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_Mouse_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Mouse type"),
    { "1351",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_1351 },
    { "NEOS",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_NEOS },
    { "Amiga",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_AMIGA },
    { "Paddles",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_PADDLE },
    { "Atari CX-22",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_CX22 },
    { "Atari ST",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_ST },
    { "Smart",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_SMART },
    { "MicroMys",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mousetype_callback,
      (ui_callback_data_t)MOUSE_TYPE_MICROMYS },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Mouse port"),
    { "Port 1",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mouseport_callback,
      (ui_callback_data_t)1 },
    { "Port 2",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_Mouseport_callback,
      (ui_callback_data_t)2 },
    SDL_MENU_LIST_END
};
#endif
