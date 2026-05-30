/*
 * fullscreen.c
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *  Martin Pottendorfer <pottendo@utanet.at>
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

//#define SDL_DEBUG

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "fullscreen.h"
#include "fullscreenarch.h"
#include "log.h"
#include "ui.h"
#include "video.h"
#include "videoarch.h"

#ifdef SDL_DEBUG
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

void fullscreen_resume(void)
{
    DBG(("%s", __func__));
}

static int fullscreen_enable(struct video_canvas_s *canvas, int enable)
{
    DBG(("%s: %i", __func__, enable));

//    if (!canvas->fullscreenconfig->device_set) {
//        return 0;
//    }
//
//    canvas->fullscreenconfig->enable = enable;
//
//    ui_check_mouse_cursor();
//
//    if (canvas->initialized) {
//        /* resize window back to normal when leaving fullscreen */
//        video_viewport_resize(canvas, 1);
//    }
    return 0;
}

/* VICE 3.10 gutted cap_fullscreen_s to {enable, mode[]}; the statusbar/
   double_size/double_scan/device capability hooks were removed. */

static int fullscreen_mode_sdl(struct video_canvas_s *canvas, int mode)
{
    DBG(("%s: %i", __func__, mode));

//    canvas->fullscreenconfig->mode = mode;
    return 0;
}

void fullscreen_capability(cap_fullscreen_t *cap_fullscreen)
{
    DBG(("%s", __func__));

    cap_fullscreen->enable = fullscreen_enable;
    cap_fullscreen->mode[0] = fullscreen_mode_sdl;
}
