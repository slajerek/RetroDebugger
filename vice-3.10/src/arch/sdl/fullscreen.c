/*
 * fullscreen.c
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *  pottendo
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

/* #define SDL_DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "fullscreen.h"
#include "fullscreenarch.h"
#include "kbd.h"
#include "log.h"
#include "ui.h"
#include "video.h"
#include "videoarch.h"

#ifdef SDL_DEBUG
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

static int fullscreen_enable(struct video_canvas_s *canvas, int enable)
{
    SDL_Event e;
    int count;

    DBG(("%s: %i", __func__, enable));

    canvas->fullscreenconfig->enable = enable;

    ui_check_mouse_cursor();

    if (canvas->initialized) {
        /* resize window back to normal when leaving fullscreen */
        video_viewport_resize(canvas, 1);

        /* HACK: when switching from/to fullscreen using hotkey (alt+d), some
                 spurious keyup/keydown events fire for the keys being held
                 down while switching modes. the following tries to get rid
                 of these events, so "alt-d" doesnt end up in the emulated machine.

        */
        count = 10; while (count--) {
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                        sdlkbd_release(SDL2x_to_SDL1x_Keys(e.key.keysym.sym), e.key.keysym.mod);
                        break;
                }
            }
            SDL_Delay(20);
        }
    }
    return 0;
}

static int fullscreen_mode_sdl(struct video_canvas_s *canvas, int mode)
{
    DBG(("%s: %i", __func__, mode));

    canvas->fullscreenconfig->mode = mode;
    return 0;
}

void fullscreen_capability(cap_fullscreen_t *cap_fullscreen)
{
    DBG(("%s", __func__));

    cap_fullscreen->enable = fullscreen_enable;
    cap_fullscreen->mode[0] = fullscreen_mode_sdl;
}
