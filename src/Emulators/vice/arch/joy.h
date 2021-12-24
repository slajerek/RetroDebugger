/*
 * joy.h - SDL joystick support.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Bernhard Kuhn <kuhn@eikon.e-technik.tu-muenchen.de>
 *  Ulmer Lionel <ulmer@poly.polytechnique.fr>
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

#ifndef VICE_JOY_H
#define VICE_JOY_H

#include "types.h"

#include "vice_sdl.h"

#include "uimenu.h"

extern int joy_arch_init(void);
extern void joystick_close(void);
extern int joystick_arch_init_resources(void);
extern void joystick_arch_resources_shutdown(void);
extern int joystick_init_cmdline_options(void);

#ifdef HAVE_SDL_NUMJOYSTICKS
extern void joy_arch_init_default_mapping(int joynum);
extern int joy_arch_mapping_load(const char *filename);
extern int joy_arch_mapping_dump(const char *filename);

//extern ui_menu_action_t sdljoy_axis_event(Uint8 joynum, Uint8 axis, Sint16 value);
//extern ui_menu_action_t sdljoy_button_event(Uint8 joynum, Uint8 button, Uint8 value);
//extern ui_menu_action_t sdljoy_hat_event(Uint8 joynum, Uint8 hat, Uint8 value);
//extern ui_menu_action_t sdljoy_autorepeat(void);
//extern BYTE sdljoy_check_axis_movement(SDL_Event e);
//extern BYTE sdljoy_check_hat_movement(SDL_Event e);
//extern void sdljoy_set_joystick(SDL_Event e, int port, int bits);
//extern void sdljoy_set_hotkey(SDL_Event e, ui_menu_entry_t *value);
//extern void sdljoy_set_keypress(SDL_Event e, int row, int col);
//extern void sdljoy_set_extra(SDL_Event e, int type);
//extern void sdljoy_unset(SDL_Event e);

#endif

extern void sdljoy_swap_ports(void);

#define JOYDEV_NONE     0
#define JOYDEV_NUMPAD   1
#define JOYDEV_KEYSET1  2
#define JOYDEV_KEYSET2  3
#define JOYDEV_JOYSTICK 4

#endif
