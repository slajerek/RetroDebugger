/*
 * mousedrv.c - Mouse handling for SDL.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Oliver Schaertel <orschaer@forwiss.uni-erlangen.de>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "vice.h"

#include "vice_sdl.h"

#include "mouse.h"
#include "mousedrv.h"
#include "ui.h"
#include "vsyncapi.h"

static mouse_func_t mouse_funcs;

void mousedrv_mouse_changed(void)
{
    ui_set_mouse_grab_window_title(_mouse_enabled);
    ui_check_mouse_cursor();
}

int mousedrv_resources_init(const mouse_func_t *funcs)
{
    /* Copy entire 'mouse_func_t' structure. */
    mouse_funcs = *funcs;
    return 0;
}

/* ------------------------------------------------------------------------- */

int mousedrv_cmdline_options_init(void)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

void mousedrv_init(void)
{
}

/* ------------------------------------------------------------------------- */

void mouse_button(int bnumber, int state)
{
    switch (bnumber) {
        case SDL_BUTTON_LEFT:
            mouse_funcs.mbl(state);
            break;
        case SDL_BUTTON_MIDDLE:
            mouse_funcs.mbm(state);
            break;
        case SDL_BUTTON_RIGHT:
            mouse_funcs.mbr(state);
            break;
/* FIXME: fix for SDL2 */
#ifndef USE_SDL2UI
        case SDL_BUTTON_WHEELUP:
            mouse_funcs.mbu(state);
            break;
        case SDL_BUTTON_WHEELDOWN:
            mouse_funcs.mbd(state);
            break;
#endif
        default:
            break;
    }
}

void mousedrv_button_left(int pressed)
{
    mouse_funcs.mbl(pressed);
}

void mousedrv_button_right(int pressed)
{
    mouse_funcs.mbr(pressed);
}

void mousedrv_button_middle(int pressed)
{
    mouse_funcs.mbm(pressed);
}
