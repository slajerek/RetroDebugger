/*
 * videoarch.h - SDL graphics routines.
 *
 * Written by:
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * based on the X11 version written by
 *  Ettore Perazzoli
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_VIDEOARCH_H
#define VICE_VIDEOARCH_H

#include "vice.h"

#include "vice_sdl.h"

#include "archdep.h"
#include "viewport.h"
#include "video.h"

#define VIDEO_CANVAS_IDX_VDC   1
#define VIDEO_CANVAS_IDX_VICII 0
#define MAX_CANVAS_NUM 2

#define VIDEO_SDL2_CANVAS_INDEX_KEY "canvas_index"

typedef void (*video_refresh_func_t)(struct video_canvas_s *, int, int, int, int, unsigned int, unsigned int);

#ifdef USE_SDL2UI
/** \brief Everything needed to render textures to the screen in a window */
struct video_container_s {
    /** \brief The SDL window associated with this renderer and texture. */
    SDL_Window* window;

    /** \brief The renderer associated with this SDL_Window. */
    SDL_Renderer* renderer;

    /** \brief State variable for making sure that the OS let us leave fullscreen sanely. */
    int leaving_fullscreen;

    /** \brief Recorded width, for dealing with windowing systems that forget
     * how big the window was when leaving fullscreen. */
    int last_width;

    /** \brief Recorded height, for dealing with windowing systems that forget
     * how big the window was when leaving fullscreen. */
    int last_height;
};
typedef struct video_container_s video_container_t;
#endif

struct video_canvas_s {
    /** \brief Nonzero if it is safe to access other members of the
     *         structure. */
    unsigned int initialized;

    /** \brief Nonzero if the structure has been fully realized. */
    unsigned int created;

    /** \brief Index of the canvas, needed for x128 and xcbm2 */
    int index;

    /** \brief Depth of the canvas in bpp */
    unsigned int depth;

    /** \brief Size of the drawable canvas area, including the black borders */
    unsigned int width, height;

    /** \brief Size of the canvas as requested by the emulator itself */
    unsigned int real_width, real_height;

    /** \brief  Actual size of the window; in most cases the same as width/height */
    unsigned int actual_width, actual_height;

    /** \brief Drawable surface. Main output for SDL1, SDL2 uses other members. */
    SDL_Surface* screen;

#ifdef USE_SDL2UI
    /** \brief The texture that can be rendered to for this window and renderer. */
    SDL_Texture* texture;

    /** \brief Last frame's texture, used for interlaced modes. */
    SDL_Texture* previous_frame_texture;

    /** \brief The SDL2 objects that this canvas can output to. */
    video_container_t* container;
#endif

    struct video_render_config_s *videoconfig;
    int crt_type;
    struct draw_buffer_s *draw_buffer;
    struct draw_buffer_s *draw_buffer_vsid;
    struct viewport_s *viewport;
    struct geometry_s *geometry;
    struct palette_s *palette;
    struct raster_s *parent_raster;

    struct fullscreenconfig_s *fullscreenconfig;
    video_refresh_func_t video_fullscreen_refresh_func;

#if defined(HAVE_HWSCALE) && !defined(USE_SDL2UI)
    /* OpenGL context */
    SDL_Surface *hwscale_screen;
#endif

    /** \brief Used to limit frame rate under warp. */
    tick_t warp_next_render_tick;
};
typedef struct video_canvas_s video_canvas_t;

extern video_canvas_t *sdl_active_canvas;

/* Resize window to stored real size */
void sdl_video_restore_size(void);

#ifdef USE_SDL2UI
/* special case handling for the SDL window resize event */
void sdl2_video_resize_event(int canvas_id, unsigned int w, unsigned int h);
#else
/* special case handling for the SDL window resize event */
void sdl_video_resize_event(unsigned int w, unsigned int h);
#endif

/* Switch to canvas with given index; used by x128 and xcbm2 */
void sdl_video_canvas_switch(int index);
extern int sdl_active_canvas_num;

void sdl_ui_init_finalize(void);

int sdl_ui_get_mouse_state(int *px, int *py, unsigned int *pbuttons);
void sdl_ui_consume_mouse_event(SDL_Event *event);

extern uint8_t *draw_buffer_vsid;

/* Modes of resolution limitation */
#define SDL_LIMIT_MODE_OFF   0
#define SDL_LIMIT_MODE_MAX   1
#define SDL_LIMIT_MODE_FIXED 2

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)

/* FIXME: remove and make global */
/* Filtering modes */
#define SDL_FILTER_NEAREST     0
#define SDL_FILTER_LINEAR      1
#endif

void sdl_ui_set_window_title(char *title);

#ifdef USE_SDL2UI
void sdl2_show_second_window(void);
void sdl2_hide_second_window(void);
video_canvas_t *sdl2_get_canvas_from_index(int index);
#endif

#endif
