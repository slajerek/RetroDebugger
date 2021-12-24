/*
 * menu_lightpen.c - Lightpen menu for SDL UI.
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
#include "lightpen.h"
#include "uimenu.h"

UI_MENU_DEFINE_TOGGLE(Lightpen)
UI_MENU_DEFINE_RADIO(LightpenType)

const ui_menu_entry_t lightpen_menu[] = {
    { "Enable lightpen",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_Lightpen_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Lightpen type"),
    { "Pen with button Up",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_LightpenType_callback,
      (ui_callback_data_t)LIGHTPEN_TYPE_PEN_U },
    { "Pen with button Left",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_LightpenType_callback,
      (ui_callback_data_t)LIGHTPEN_TYPE_PEN_L },
    { "Datel Pen",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_LightpenType_callback,
      (ui_callback_data_t)LIGHTPEN_TYPE_PEN_DATEL },
    { "Magnum Light Phaser",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_LightpenType_callback,
      (ui_callback_data_t)LIGHTPEN_TYPE_GUN_Y },
    { "Stack Light Rifle",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_LightpenType_callback,
      (ui_callback_data_t)LIGHTPEN_TYPE_GUN_L },
    SDL_MENU_LIST_END
};

#endif
