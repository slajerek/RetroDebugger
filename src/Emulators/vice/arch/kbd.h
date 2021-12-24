/*
 * kbd.h - SDL specfic keyboard driver.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README file for copyright notice.
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

#ifndef VICE_KBD_H
#define VICE_KBD_H

#include "vice_sdl.h"
#include "uimenu.h"

extern void kbd_arch_init(void);
extern int kbd_arch_get_host_mapping(void);

extern signed long kbd_arch_keyname_to_keynum(char *keyname);
extern const char *kbd_arch_keynum_to_keyname(signed long keynum);
extern void kbd_initialize_numpad_joykeys(int *joykeys);

#define KBD_PORT_PREFIX "sdl"

#define KBD_C64_SYM_US  "sdl_sym.vkm"
#define KBD_C64_SYM_DE  "sdl_sym.vkm"
#define KBD_C64_POS     "sdl_pos.vkm"
#define KBD_C128_SYM    "sdl_sym.vkm"
#define KBD_C128_POS    "sdl_pos.vkm"
#define KBD_VIC20_SYM   "sdl_sym.vkm"
#define KBD_VIC20_POS   "sdl_pos.vkm"
#define KBD_PET_SYM_UK  "sdl_buks.vkm"
#define KBD_PET_POS_UK  "sdl_bukp.vkm"
#define KBD_PET_SYM_DE  "sdl_bdes.vkm"
#define KBD_PET_POS_DE  "sdl_bdep.vkm"
#define KBD_PET_SYM_GR  "sdl_bgrs.vkm"
#define KBD_PET_POS_GR  "sdl_bgrp.vkm"
#define KBD_PLUS4_SYM   "sdl_sym.vkm"
#define KBD_PLUS4_POS   "sdl_pos.vkm"
#define KBD_CBM2_SYM_UK "sdl_buks.vkm"
#define KBD_CBM2_POS_UK "sdl_bukp.vkm"
#define KBD_CBM2_SYM_DE "sdl_bdes.vkm"
#define KBD_CBM2_POS_DE "sdl_bdep.vkm"
#define KBD_CBM2_SYM_GR "sdl_bgrs.vkm"
#define KBD_CBM2_POS_GR "sdl_bgrp.vkm"

#define KBD_INDEX_C64_DEFAULT   KBD_INDEX_C64_SYM
#define KBD_INDEX_C128_DEFAULT  KBD_INDEX_C128_SYM
#define KBD_INDEX_VIC20_DEFAULT KBD_INDEX_VIC20_SYM
#define KBD_INDEX_PET_DEFAULT   KBD_INDEX_PET_BUKS
#define KBD_INDEX_PLUS4_DEFAULT KBD_INDEX_PLUS4_SYM
#define KBD_INDEX_CBM2_DEFAULT  KBD_INDEX_CBM2_BUKS

//extern ui_menu_action_t sdlkbd_press(SDLKey key, SDLMod mod);
//extern ui_menu_action_t sdlkbd_release(SDLKey key, SDLMod mod);
//
//extern void sdlkbd_set_hotkey(SDLKey key, SDLMod mod, ui_menu_entry_t *value);
extern int sdlkbd_hotkeys_load(const char *filename);
extern int sdlkbd_hotkeys_load(const char *filename);
extern int sdlkbd_hotkeys_dump(const char *filename);

extern int sdlkbd_init_resources(void);
extern void sdlkbd_resources_shutdown(void);

extern int sdlkbd_init_cmdline(void);

extern void kbd_enter_leave(void);
extern void kbd_focus_change(void);

extern int sdl_ui_menukeys[];

#endif
