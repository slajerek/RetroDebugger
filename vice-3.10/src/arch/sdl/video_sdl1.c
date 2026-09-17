/** \brief  video.c
 * \brief   SDL video (probably needs a more descriptive 'brief')
 *
 * \author  Hannu Nuotio <hannu.nuotio@tut.fi>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
 *  Ettore Perazzoli
 *  Andre Fachat
 *  Oliver Schaertel
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
#include "vice_sdl.h"

#include "archdep.h"
#include "cmdline.h"
#include "fullscreen.h"
#include "fullscreenarch.h"
#include "icon.h"
#include "joy.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "lightpendrv.h"
#include "log.h"
#include "machine.h"
#include "mousedrv.h"
#include "palette.h"
#include "raster.h"
#include "resources.h"
#include "ui.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "util.h"
#include "videoarch.h"
#include "vkbd.h"
#include "vsidui_sdl.h"
#include "vsync.h"

#ifdef SDL_DEBUG
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

static log_t sdlvideo_log = LOG_DEFAULT;

static int sdl_bitdepth;

static int sdl_limit_mode;
static int sdl_ui_finalized;

/* window size, used for free scaling */
static int sdl_window_width = 0;
static int sdl_window_height = 0;

int sdl_active_canvas_num = 0;
static int sdl_num_screens = 0;
static video_canvas_t *sdl_canvaslist[MAX_CANVAS_NUM];
video_canvas_t *sdl_active_canvas = NULL;

#if defined(HAVE_HWSCALE)
static int sdl_gl_mode;
static GLint screen_texture;

static const float sdl_gl_vertex_coord[4][2] = {
    /* Lower Right Of Texture */
    { -1.0f, +1.0f },
    /* Upper Right Of Texture */
    { -1.0f, -1.0f },
    /* Upper Left Of Texture */
    { +1.0f, -1.0f },
    /* Lower Left Of Texture */
    { +1.0f, +1.0f },
};

static const int sdl_gl_vertex_pts[4 * 2][4] = {
    /* Normal */
    { 0, 1, 2, 3 }, /* Normal */
    { 3, 2, 1, 0 }, /* Flip X */
    { 1, 0, 3, 2 }, /* Flip Y */
    { 2, 3, 0, 1 }, /* Flip X&Y */
    /* rotated 90 degrees */
    { 3, 0, 1, 2 }, /* Normal */
    { 0, 3, 2, 1 }, /* Flip X */
    { 2, 1, 0, 3 }, /* Flip Y */
    { 1, 2, 3, 0 }, /* Flip X&Y */
};
#endif

struct sdl_lightpen_adjust_s {
    int offset_x, offset_y;
    int max_x, max_y;
    double scale_x, scale_y;
};
typedef struct sdl_lightpen_adjust_s sdl_lightpen_adjust_t;
static sdl_lightpen_adjust_t sdl_lightpen_adjust;

uint8_t *draw_buffer_vsid = NULL;
/* ------------------------------------------------------------------------- */
/* Video-related resources.  */

static int set_sdl_bitdepth(int d, void *param)
{
    switch (d) {
        case 0:
        case 8:
        case 15:
        case 16:
        case 24:
        case 32:
            break;
        default:
            return -1;
    }

    if (sdl_bitdepth == d) {
        return 0;
    }
    sdl_bitdepth = d;

    /* update */
    return 0;
}

static int set_sdl_limit_mode(int v, void *param)
{
    switch (v) {
        case SDL_LIMIT_MODE_OFF:
        case SDL_LIMIT_MODE_MAX:
        case SDL_LIMIT_MODE_FIXED:
            break;
        default:
            return -1;
    }

    if (sdl_limit_mode != v) {
        sdl_limit_mode = v;
        video_viewport_resize(sdl_active_canvas, 1);
    }
    return 0;
}

/* custom width for fullscreen */
int ui_set_fullscreen_custom_width(int w, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (cv->videoconfig->fullscreen_custom_width != w) {
        cv->videoconfig->fullscreen_custom_width = w;
        if (cv && cv->fullscreenconfig->enable
            && cv->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
            video_viewport_resize(cv, 1);
        }
    }
    return 0;
}

/* custom height for fullscreen */
int ui_set_fullscreen_custom_height(int h, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (cv->videoconfig->fullscreen_custom_height != h) {
        cv->videoconfig->fullscreen_custom_height = h;
        if (cv && cv->fullscreenconfig->enable
            && cv->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
            video_viewport_resize(cv, 1);
        }
    }
    return 0;
}

static int set_sdl_window_width(int w, void *param)
{
    if (w < 0) {
        return -1;
    }

    sdl_window_width = w;
    return 0;
}

static int set_sdl_window_height(int h, void *param)
{
    if (h < 0) {
        return -1;
    }

    sdl_window_height = h;
    return 0;
}

#if defined(HAVE_HWSCALE)

/* called when the <CHIP>AspectMode resource was set */
int ui_set_aspect_mode(int newmode, void *canvas)
{
    int oldmode;
    video_canvas_t *cv = canvas;

    oldmode = cv->videoconfig->aspect_mode;

    switch (newmode) {
        case VIDEO_ASPECT_MODE_NONE:
        case VIDEO_ASPECT_MODE_CUSTOM:
        case VIDEO_ASPECT_MODE_TRUE:
            break;
        default:
            return -1;
    }

    cv->videoconfig->aspect_mode = newmode;

    if (oldmode != newmode) {
        if (sdl_active_canvas) {
            video_viewport_resize(sdl_active_canvas, 1);
        }
    }

    return 0;
}

/* called when the <CHIP>AspectRatio resource was set */
int ui_set_aspect_ratio(double aspect_ratio, void *canvas)
{
    video_canvas_t *cv = canvas;
    double old_aspect = cv->videoconfig->aspect_ratio;

    if (old_aspect != aspect_ratio) {
        if (sdl_active_canvas) {
            video_viewport_resize(sdl_active_canvas, 1);
        }
    }
    return 0;
}

int ui_set_flipx(int val, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (val < 0) {
        val = 0;
    }
    if (val > 1) {
        val = 1;
    }
    cv->videoconfig->flipx = val;
    return 0;
}

int ui_set_flipy(int val, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (val < 0) {
        val = 0;
    }
    if (val > 1) {
        val = 1;
    }
    cv->videoconfig->flipy = val;
    return 0;
}

int ui_set_glfilter(int val, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (val == VIDEO_GLFILTER_NEAREST) {
        val = VIDEO_GLFILTER_NEAREST;
    } else {
        val = VIDEO_GLFILTER_BILINEAR;
    }

    cv->videoconfig->glfilter = val;
    return 0;
}

int ui_set_rotate(int val, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (val < 0) {
        val = 0;
    }
    if (val > 1) {
        val = 1;
    }
    cv->videoconfig->rotate = val;
    return 0;
}

int ui_set_vsync(int val, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (val < 0) {
        val = 0;
    }
    if (val > 1) {
        val = 1;
    }
    cv->videoconfig->vsync = val;
    return 0;
}

#endif /* HAVE_HWSCALE */

#define VICE_DEFAULT_BITDEPTH 0

#define SDLLIMITMODE_DEFAULT     SDL_LIMIT_MODE_OFF

/* FIXME: more resources should have the same name as their GTK counterparts,
          and the SDL prefix removed */
static const resource_int_t resources_int[] = {
    { "SDLBitdepth", VICE_DEFAULT_BITDEPTH, RES_EVENT_NO, NULL,
      &sdl_bitdepth, set_sdl_bitdepth, NULL },
    { "SDLLimitMode", SDLLIMITMODE_DEFAULT, RES_EVENT_NO, NULL,
      &sdl_limit_mode, set_sdl_limit_mode, NULL },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window0Width", 0, RES_EVENT_NO, NULL,
      &sdl_window_width, set_sdl_window_width, NULL },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window0Height", 0, RES_EVENT_NO, NULL,
      &sdl_window_height, set_sdl_window_height, NULL },
    RESOURCE_INT_LIST_END
};

int video_arch_resources_init(void)
{
    DBG(("%s", __func__));

    if (machine_class == VICE_MACHINE_VSID) {
        if (joy_sdl_resources_init() < 0) {
            return -1;
        }
    }
    return resources_register_int(resources_int);
}

void video_arch_resources_shutdown(void)
{
    DBG(("%s", __func__));

#if defined(HAVE_HWSCALE)
    /* FIXME: should loop over all canvas */
    /* lib_free(canvas->videoconfig->aspect_ratio_s); */
#endif
}

/* ------------------------------------------------------------------------- */
/* Video-related command-line options.  */

/* FIXME: more options should have the same name as their GTK counterparts,
          and the SDL prefix removed */
static const cmdline_option_t cmdline_options[] =
{
    { "-sdlbitdepth", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SDLBitdepth", NULL,
      "<bpp>", "Set bitdepth (0 = current, 8, 15, 16, 24, 32)" },
    { "-sdllimitmode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SDLLimitMode", NULL,
      "<mode>", "Set resolution limiting mode (0 = off, 1 = max, 2 = fixed)" },
    /* FIXME: this could be a generic (not SDL specific) option */
    { "-sdlinitialw", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window0Width", NULL,
      "<width>", "Set initial window width" },
    /* FIXME: this could be a generic (not SDL specific) option */
    { "-sdlinitialh", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window0Height", NULL,
      "<height>", "Set initial window height" },
    CMDLINE_LIST_END
};

int video_arch_cmdline_options_init(void)
{
    DBG(("%s", __func__));

    if (machine_class == VICE_MACHINE_VSID) {
        if (joystick_cmdline_options_init() < 0) {
            return -1;
        }
    }

    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

int video_init(void)
{
    sdlvideo_log = log_open("SDLVideo");
    return 0;
}

void video_shutdown(void)
{
    DBG(("%s", __func__));

    if (draw_buffer_vsid) {
        lib_free(draw_buffer_vsid);
    }

    sdl_active_canvas = NULL;
}

/* ------------------------------------------------------------------------- */
/* static helper functions */

static int sdl_video_canvas_limit(unsigned int limit_w, unsigned int limit_h, unsigned int *w, unsigned int *h, int mode)
{
    DBG(("%s", __func__));
    switch (mode & 3) {
        case SDL_LIMIT_MODE_MAX:
            if ((*w > limit_w) || (*h > limit_h)) {
                *w = MIN(*w, limit_w);
                *h = MIN(*h, limit_h);
                return 1;
            }
            break;
        case SDL_LIMIT_MODE_FIXED:
            if ((*w != limit_w) || (*h != limit_h)) {
                *w = limit_w;
                *h = limit_h;
                return 1;
            }
            break;
        case SDL_LIMIT_MODE_OFF:
        default:
            break;
    }
    return 0;
}

#if defined(HAVE_HWSCALE)
static void sdl_gl_set_viewport(unsigned int src_w, unsigned int src_h, unsigned int dest_w, unsigned int dest_h)
{
    int dest_x = 0, dest_y = 0;

    if (sdl_active_canvas->videoconfig->aspect_mode != VIDEO_ASPECT_MODE_NONE) {
        double aspect = sdl_active_canvas->videoconfig->aspect_ratio;

        /* Get "true" aspect ratio */
        if (sdl_active_canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
            aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
        }

        /* Keep aspect ratio of src image. */
        if (dest_w * src_h < src_w * aspect * dest_h) {
            dest_y = dest_h;
            dest_h = (unsigned int)(dest_w * src_h / (src_w * aspect) + 0.5);
            dest_y = (dest_y - dest_h) / 2;
        } else {
            dest_x = dest_w;
            dest_w = (unsigned int)(dest_h * src_w * aspect / src_h + 0.5);
            dest_x = (dest_x - dest_w) / 2;
        }
    }

    /* Update lightpen adjustment parameters */
    sdl_lightpen_adjust.offset_x = dest_x;
    sdl_lightpen_adjust.offset_y = dest_y;

    sdl_lightpen_adjust.max_x = dest_w;
    sdl_lightpen_adjust.max_y = dest_h;

    sdl_lightpen_adjust.scale_x = (double)(src_w) / (double)(dest_w);
    sdl_lightpen_adjust.scale_y = (double)(src_h) / (double)(dest_h);

    glViewport(dest_x, dest_y, dest_w, dest_h);
}
#endif

static video_canvas_t *sdl_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height)
{
    SDL_Surface *new_screen;
    unsigned int new_width, new_height;
    unsigned int actual_width, actual_height;
    int flags;
    int fullscreen = 0;
    int limit = sdl_limit_mode;
    unsigned int limit_w = (unsigned int)canvas->videoconfig->fullscreen_custom_width;
    unsigned int limit_h = (unsigned int)canvas->videoconfig->fullscreen_custom_height;
    int hwscale = 0;
    int lightpen_updated = 0;
#ifdef HAVE_HWSCALE
    int rbits = 0, gbits = 0, bbits = 0, abits = 0;
    const Uint32

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00, amask = 0x000000ff;
#else
    rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
#endif
#endif

    DBG(("%s: %i,%i (%i)", __func__, *width, *height, canvas->index));

    flags = SDL_SWSURFACE | SDL_RESIZABLE;

    new_width = *width;
    new_height = *height;

    new_width *= canvas->videoconfig->scalex;
    new_height *= canvas->videoconfig->scaley;

    sdl_ui_set_window_icon(NULL);

    if ((canvas == sdl_active_canvas) && (canvas->fullscreenconfig->enable)) {
        fullscreen = 1;
    }

#ifdef HAVE_HWSCALE
    if (canvas == sdl_active_canvas) {
        hwscale = 1;
    }
#endif

    if (fullscreen) {
        flags = SDL_FULLSCREEN | SDL_SWSURFACE;

        if (canvas->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
            limit = SDL_LIMIT_MODE_FIXED;
        }
    }

    if (!sdl_ui_finalized) { /* remember first size */
        double aspect = 1.0;
#ifdef HAVE_HWSCALE
        aspect = sdl_active_canvas->videoconfig->aspect_ratio;

        if (sdl_active_canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
            aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
        }
#endif
        sdl_active_canvas->real_width = (unsigned int)((double)new_width * aspect + 0.5);
        sdl_active_canvas->real_height = new_height;
        DBG(("first: %d:%d\n", sdl_active_canvas->real_width, sdl_active_canvas->real_height));
    }

#ifdef HAVE_HWSCALE
    if (hwscale) {
        flags |= SDL_OPENGL;

        if (fullscreen) {
            limit = SDL_LIMIT_MODE_OFF;
        } else {
            double aspect = sdl_active_canvas->videoconfig->aspect_ratio;

            if (sdl_active_canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
                aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
            }

            /* if no window geometry given then create one. */
            if (!sdl_window_width || !sdl_window_height) {
                limit_w = (unsigned int)((double)new_width * aspect + 0.5);
                limit_h = new_height;
            } else { /* full window size remembering when aspect ratio is not important */
                if (sdl_active_canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_NONE) {
                    limit_w = (unsigned int)sdl_window_width;
                    limit_h = (unsigned int)sdl_window_height;
                } else { /* only remember height, set width according to that and the aspect ratio */
                    limit_h = (unsigned int)sdl_window_height;
                    limit_w = (unsigned int)((double)new_width * (double)sdl_window_height * aspect / (double)new_height + 0.5);
                }
            }
            limit = SDL_LIMIT_MODE_FIXED;
        }

        switch (sdl_bitdepth) {
            case 0:
                log_warning(sdlvideo_log, "bitdepth not set for OpenGL, trying 32...");
                sdl_bitdepth = 32;
            /* fall through */
            case 32:
                rbits = gbits = bbits = abits = 8;
                sdl_gl_mode = GL_RGBA;
                break;
            case 24:
                rbits = gbits = bbits = 8;
                abits = 0;
                sdl_gl_mode = GL_RGB;
                break;
            default:
                log_error(sdlvideo_log, "%i bpp not supported in OpenGL.", sdl_bitdepth);
                hwscale = 0;
                flags = SDL_SWSURFACE;
                break;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, rbits);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, gbits);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bbits);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, abits);
    }
#endif

    actual_width = new_width;
    actual_height = new_height;

    if (canvas == sdl_active_canvas) {
#ifdef HAVE_HWSCALE
        if (hwscale) {
            double aspect = sdl_active_canvas->videoconfig->aspect_ratio;

            if (sdl_active_canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
                aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
            }

            actual_width = (unsigned int)((double)actual_width * aspect + 0.5);
        }
#endif
        if (sdl_video_canvas_limit(limit_w, limit_h, &actual_width, &actual_height, limit)) {
            if (!hwscale) {
                canvas->draw_buffer->canvas_physical_width = actual_width;
                canvas->draw_buffer->canvas_physical_height = actual_height;
                video_viewport_resize(sdl_active_canvas, 0);
                if (sdl_ui_finalized) {
                    return canvas; /* exit here as video_viewport_resize will recall */
                }
            }
        }
    }

    if (canvas == sdl_active_canvas) {
        SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
#ifndef HAVE_HWSCALE
        new_screen = SDL_SetVideoMode(actual_width, actual_height, sdl_bitdepth, flags);
        new_width = new_screen->w;
        new_height = new_screen->h;
#else
        if (hwscale) {
            /* To get fullscreen resolution, SetVideoMode must be called with the
               desired fullscreen resolution. If it is called with a smaller resolution,
               it will display the undesirable black borders around the emulator display. */
            if ((fullscreen) && (canvas->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM)) {
                new_screen = SDL_SetVideoMode(limit_w, limit_h, sdl_bitdepth, flags);
            } else {
                new_screen = SDL_SetVideoMode(actual_width, actual_height, sdl_bitdepth, flags);
            }
            if (!new_screen) { /* Did not work out quite well. Let's try without hwscale */
                return sdl_canvas_create(canvas, width, height);
            }
            actual_width = new_screen->w;
            actual_height = new_screen->h;

            /* free the old rendering surface when staying in hwscale mode */
            if ((canvas->hwscale_screen) && (canvas->screen)) {
                SDL_FreeSurface(canvas->screen);
            }

            canvas->hwscale_screen = new_screen;
            new_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, new_width, new_height, sdl_bitdepth, rmask, gmask, bmask, amask);
            sdl_gl_set_viewport(new_width, new_height, actual_width, actual_height);
            lightpen_updated = 1;
        } else {
            new_screen = SDL_SetVideoMode(actual_width, actual_height, sdl_bitdepth, flags);
            new_width = new_screen->w;
            new_height = new_screen->h;

            /* free the old rendering surface when leaving hwscale mode */
            if ((canvas->hwscale_screen) && (canvas->screen)) {
                SDL_FreeSurface(canvas->screen);
                SDL_FreeSurface(canvas->hwscale_screen);
                canvas->hwscale_screen = NULL;
            }
        }
#endif
        SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE);
    } else {
#ifdef HAVE_HWSCALE
        /* free the old hwscale screen when hwscaled screen is switched away */
        if (canvas->hwscale_screen) {
            SDL_FreeSurface(canvas->hwscale_screen);
            canvas->hwscale_screen = NULL;
        }
        if (!hwscale) {
            new_width = actual_width;
            new_height = actual_height;
        }
#else
        new_width = actual_width;
        new_height = actual_height;
#endif
        if (canvas->screen) {
            SDL_FreeSurface(canvas->screen);
        }
        new_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, new_width, new_height, sdl_bitdepth, 0, 0, 0, 0);
    }

    if (!new_screen) {
        log_error(sdlvideo_log, "SDL_SetVideoMode failed!");
        return NULL;
    }
    sdl_bitdepth = new_screen->format->BitsPerPixel;

    canvas->depth = sdl_bitdepth;
    canvas->width = new_width;
    canvas->height = new_height;
    canvas->screen = new_screen;
    canvas->actual_width = actual_width;
    canvas->actual_height = actual_height;

    if (canvas == sdl_active_canvas) {
        if (!fullscreen) {
            resources_set_int("Window0Width", actual_width);
            resources_set_int("Window0Height", actual_height);
        }
    }

    log_message(sdlvideo_log, "%s (%s) %ux%u %ibpp %s%s",
        canvas->videoconfig->chip_name,
        (canvas == sdl_active_canvas) ? "active" : "inactive",
        actual_width,
        actual_height,
        sdl_bitdepth,
        hwscale ? "OpenGL " : "",
        (canvas->fullscreenconfig->enable) ? "(fullscreen)" : "");
#ifdef SDL_DEBUG
    log_message(sdlvideo_log, "Canvas %ix%i, real %ix%i", new_width, new_height, canvas->real_width, canvas->real_height);
#endif

    /* Update lightpen adjustment parameters */
    if (canvas == sdl_active_canvas && !lightpen_updated) {
        sdl_lightpen_adjust.max_x = actual_width;
        sdl_lightpen_adjust.max_y = actual_height;

        sdl_lightpen_adjust.scale_x = (double)*width / (double)actual_width;
        sdl_lightpen_adjust.scale_y = (double)*height / (double)actual_height;
    }

    video_canvas_set_palette(canvas, canvas->palette);

    return canvas;
}

/* ------------------------------------------------------------------------- */
/* Main API */

/* called from raster/raster.c:realize_canvas */
video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
    /* nothing to do here, the real work is done in sdl_ui_init_finalize */
    return canvas;
}

void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
    uint8_t *backup;

    if ((canvas == NULL) || (canvas->screen == NULL) || (canvas != sdl_active_canvas)) {
        return;
    }

    if (sdl_vsid_state & SDL_VSID_ACTIVE) {
        sdl_vsid_draw();
    }

    if (sdl_vkbd_state & SDL_VKBD_ACTIVE) {
        sdl_vkbd_draw();
    }

    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
        uistatusbar_draw();
    }

    xi *= canvas->videoconfig->scalex;
    w *= canvas->videoconfig->scalex;

    yi *= canvas->videoconfig->scaley;
    h *= canvas->videoconfig->scaley;

    w = MIN(w, canvas->width);
    h = MIN(h, canvas->height);

    /* FIXME attempt to draw outside canvas */
    if ((xi + w > canvas->width) || (yi + h > canvas->height)) {
        return;
    }

    if (SDL_MUSTLOCK(canvas->screen)) {
        canvas->videoconfig->readable = 0;
        if (SDL_LockSurface(canvas->screen) < 0) {
            return;
        }
    } else { /* no direct rendering, safe to read */
        canvas->videoconfig->readable = !(canvas->screen->flags & SDL_HWSURFACE);
    }

    if (machine_class == VICE_MACHINE_VSID) {
        canvas->draw_buffer_vsid->draw_buffer_width = canvas->draw_buffer->draw_buffer_width;
        canvas->draw_buffer_vsid->draw_buffer_height = canvas->draw_buffer->draw_buffer_height;
        canvas->draw_buffer_vsid->draw_buffer_pitch = canvas->draw_buffer->draw_buffer_pitch;
        canvas->draw_buffer_vsid->canvas_physical_width = canvas->draw_buffer->canvas_physical_width;
        canvas->draw_buffer_vsid->canvas_physical_height = canvas->draw_buffer->canvas_physical_height;
        canvas->draw_buffer_vsid->canvas_width = canvas->draw_buffer->canvas_width;
        canvas->draw_buffer_vsid->canvas_height = canvas->draw_buffer->canvas_height;
        canvas->draw_buffer_vsid->visible_width = canvas->draw_buffer->visible_width;
        canvas->draw_buffer_vsid->visible_height = canvas->draw_buffer->visible_height;

        backup = canvas->draw_buffer->draw_buffer;
        canvas->draw_buffer->draw_buffer = canvas->draw_buffer_vsid->draw_buffer;
        video_canvas_render(canvas, (uint8_t *)canvas->screen->pixels, w, h, xs, ys, xi, yi, canvas->screen->pitch);
        canvas->draw_buffer->draw_buffer = backup;
    } else {
        video_canvas_render(canvas, (uint8_t *)canvas->screen->pixels, w, h, xs, ys, xi, yi, canvas->screen->pitch);
    }

    if (SDL_MUSTLOCK(canvas->screen)) {
        SDL_UnlockSurface(canvas->screen);
    }

#if defined(HAVE_HWSCALE)
    {
        int sdl_gl_vertex_base = (canvas->videoconfig->flipx << 0) |
                                 (canvas->videoconfig->flipy << 1) |
                                 (canvas->videoconfig->rotate << 2);
        int sdl_gl_filter = (canvas->videoconfig->glfilter == VIDEO_GLFILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;

        if (canvas != sdl_active_canvas) {
            DBG(("%s: not active SDL canvas, ignoring", __func__));
            return;
        }

        if (!(canvas->hwscale_screen)) {
            DBG(("%s: hwscale refresh without hwscale screen, ignoring", __func__));
            return;
        }

/* XXX make use of glXBindTexImageEXT aka texture from pixmap extension */

        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

/* GL_TEXTURE_RECTANGLE is standardised as _EXT in OpenGL 1.4. Here's some
 * aliases in the meantime. */
#ifndef GL_TEXTURE_RECTANGLE_EXT
    #if defined(GL_TEXTURE_RECTANGLE_NV)
        #define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_NV
    #elif defined(GL_TEXTURE_RECTANGLE_ARB)
        #define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_ARB
    #else
        #error "Your headers do not supply GL_TEXTURE_RECTANGLE. Disable HWSCALE and try again."
    #endif
#endif

        glEnable(GL_TEXTURE_RECTANGLE_EXT);
        glBindTexture(GL_TEXTURE_RECTANGLE_EXT, screen_texture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, sdl_gl_filter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, sdl_gl_filter);
        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, sdl_gl_mode, canvas->width, canvas->height, 0, sdl_gl_mode, GL_UNSIGNED_BYTE, canvas->screen->pixels);

        glBegin(GL_QUADS);

        /* Lower Right Of Texture */
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][0]][0],
                    sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][0]][1]);

        /* Upper Right Of Texture */
        glTexCoord2f(0.0f, (float)(canvas->height));
        glVertex2f(sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][1]][0],
                    sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][1]][1]);

        /* Upper Left Of Texture */
        glTexCoord2f((float)(canvas->width), (float)(canvas->height));
        glVertex2f(sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][2]][0],
                    sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][2]][1]);

        /* Lower Left Of Texture */
        glTexCoord2f((float)(canvas->width), 0.0f);
        glVertex2f(sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][3]][0],
                    sdl_gl_vertex_coord[sdl_gl_vertex_pts[sdl_gl_vertex_base][3]][1]);

        glEnd();

        SDL_GL_SwapBuffers();
    }
#else
    SDL_UpdateRect(canvas->screen, xi, yi, w, h);
#endif
    ui_autohide_mouse_cursor();
}

int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette)
{
    unsigned int i, col = 0;
    video_render_color_tables_t *color_tables = &canvas->videoconfig->color_tables;
    SDL_PixelFormat *fmt;
    SDL_Color colors[256];

    DBG(("video_canvas_set_palette canvas: %p", canvas));

    if (palette == NULL) {
        return 0; /* no palette, nothing to do */
    }

    canvas->palette = palette;

    /* Fixme: needs further investigation how it can reach here without being fully initialized */
    if (canvas != sdl_active_canvas || canvas->width != canvas->screen->w) {
        DBG(("video_canvas_set_palette not active canvas or window not created, don't update hw palette"));
        return 0;
    }

    fmt = canvas->screen->format;

    for (i = 0; i < palette->num_entries; i++) {
        if (canvas->depth == 8) {
            colors[i].r = palette->entries[i].red;
            colors[i].b = palette->entries[i].blue;
            colors[i].g = palette->entries[i].green;
            col = i;
        } else {
            col = SDL_MapRGB(fmt, palette->entries[i].red, palette->entries[i].green, palette->entries[i].blue);
        }
        video_render_setphysicalcolor(canvas->videoconfig, i, col, canvas->depth);
    }

    if (canvas->depth == 8) {
        SDL_SetColors(canvas->screen, colors, 0, palette->num_entries);
    } else {
        for (i = 0; i < 256; i++) {
            video_render_setrawrgb(color_tables, i, SDL_MapRGB(fmt, (Uint8)i, 0, 0), SDL_MapRGB(fmt, 0, (Uint8)i, 0), SDL_MapRGB(fmt, 0, 0, (Uint8)i));
        }
        video_render_initraw(canvas->videoconfig);
    }

    return 0;
}

/* called from video_viewport_resize */
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
    unsigned int width = canvas->draw_buffer->canvas_width;
    unsigned int height = canvas->draw_buffer->canvas_height;
    DBG(("%s: %ix%i (%i)", __func__, width, height, canvas->index));
    /* Check if canvas needs to be resized to real size first */
    if (sdl_ui_finalized) {
        /* NOTE: setting the resources to zero like this here would actually
                 not only force a recalculation of the resources, but also
                 result in the window size being recalculated from the default
                 dimensions instead of the (saved and supposed to be persistant)
                 values in the resources. what goes wrong when this is done can
                 be observed when x128 starts up.
            FIXME: remove this note and code below after some testing. hopefully
                   nothing else relies on the broken behavior...
         */
#if 0
        sdl_window_width = 0; /* force recalculate */
        sdl_window_height = 0;
#endif
        sdl_canvas_create(canvas, &width, &height); /* set the real canvas size */

        if (resize_canvas) {
            DBG(("%s: set and resize to real size (%ix%i)", __func__, width, height));
            canvas->real_width = canvas->actual_width;
            canvas->real_height = canvas->actual_height;
        }
        /* Recreating the video like this sometimes makes us lose the
        fact that keys were released or pressed. Reset the keyboard
        state. */
        keyboard_key_clear();
    }
}

/* Resize window to w/h. */
static void sdl_video_resize(unsigned int w, unsigned int h)
{
    DBG(("%s: %ix%i", __func__, w, h));

    if ((w == 0) || (h == 0)) {
        DBG(("%s: ERROR, ignored!", __func__));
        return;
    }

    vsync_suspend_speed_eval();

#if defined(HAVE_HWSCALE)
    if (sdl_active_canvas->hwscale_screen) {
        int flags;

        if (sdl_active_canvas->fullscreenconfig->enable) {
            flags = SDL_OPENGL | SDL_SWSURFACE | SDL_FULLSCREEN;
        } else {
            flags = SDL_OPENGL | SDL_SWSURFACE | SDL_RESIZABLE;
        }

        SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
        sdl_active_canvas->hwscale_screen = SDL_SetVideoMode((int)w, (int)h, sdl_bitdepth, flags);
        SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE);

#ifdef SDL_DEBUG
        if (!sdl_active_canvas->hwscale_screen) {
            DBG(("%s: setting video mode failed", __func__));
        }
#endif
        sdl_gl_set_viewport(sdl_active_canvas->width, sdl_active_canvas->height, w, h);
        sdl_active_canvas->actual_width = w;
        sdl_active_canvas->actual_height = h;
    } else
#endif /*  HAVE_HWSCALE */
    {
        sdl_active_canvas->draw_buffer->canvas_physical_width = w;
        sdl_active_canvas->draw_buffer->canvas_physical_height = h;
        video_viewport_resize(sdl_active_canvas, 0);
    }
}

/* Resize window to stored real size */
void sdl_video_restore_size(void)
{
    unsigned int w, h;

    w = sdl_active_canvas->real_width;
    h = sdl_active_canvas->real_height;

    DBG(("%s: %ix%i->%ix%i", __func__, sdl_active_canvas->real_width, sdl_active_canvas->real_height, w, h));
    sdl_video_resize(w, h);
}

/* special case handling for the SDL window resize event */
void sdl_video_resize_event(unsigned int w, unsigned int h)
{
#if defined(HAVE_HWSCALE)

    DBG(("%s: %ix%i", __func__, w, h));
    if ((w == 0) || (h == 0)) {
        DBG(("%s: ERROR, ignored!", __func__));
        return;
    }
    sdl_video_resize(w, h);
    if (!sdl_active_canvas->fullscreenconfig->enable) {
        resources_set_int("Window0Width", sdl_active_canvas->actual_width);
        resources_set_int("Window0Height", sdl_active_canvas->actual_height);
    }

#endif /*  HAVE_HWSCALE */
}

void sdl_video_canvas_switch(int index)
{
    struct video_canvas_s *canvas;

    DBG(("%s: %i->%i", __func__, sdl_active_canvas_num, index));

    if (sdl_active_canvas_num == index) {
        return;
    }

    if (index >= sdl_num_screens) {
        return;
    }

    if (sdl_canvaslist[index]->screen != NULL) {
        SDL_FreeSurface(sdl_canvaslist[index]->screen);
        sdl_canvaslist[index]->screen = NULL;
    }

    sdl_active_canvas_num = index;

    canvas = sdl_canvaslist[sdl_active_canvas_num];
    sdl_active_canvas = canvas;

    video_viewport_resize(canvas, 1);
}

int video_arch_get_active_chip(void)
{
    if (sdl_active_canvas_num == VIDEO_CANVAS_IDX_VDC) {
        return VIDEO_CHIP_VDC;
    } else {
        return VIDEO_CHIP_VICII;
    }
}

void video_arch_canvas_init(struct video_canvas_s *canvas)
{
    DBG(("%s: (%p, %i)", __func__, canvas, sdl_num_screens));

    if (sdl_num_screens == MAX_CANVAS_NUM) {
        log_error(sdlvideo_log, "Too many canvases!");
        archdep_vice_exit(-1);
    }

    canvas->fullscreenconfig = lib_calloc(1, sizeof(fullscreenconfig_t));

    if (sdl_active_canvas_num == sdl_num_screens) {
        sdl_active_canvas = canvas;
    }

    canvas->index = sdl_num_screens;

    sdl_canvaslist[sdl_num_screens++] = canvas;

    canvas->screen = NULL;
#if defined(HAVE_HWSCALE)
    canvas->hwscale_screen = NULL;
#endif
    canvas->real_width = 0;
    canvas->real_height = 0;
}

void video_canvas_destroy(struct video_canvas_s *canvas)
{
    int i;

    DBG(("%s: (%p, %i)", __func__, canvas, canvas->index));

    for (i = 0; i < sdl_num_screens; ++i) {
        if ((sdl_canvaslist[i] == canvas) && (canvas == sdl_active_canvas)) {
            SDL_FreeSurface(sdl_canvaslist[i]->screen);
            sdl_canvaslist[i]->screen = NULL;
        }
    }

    lib_free(canvas->fullscreenconfig);
}

char video_canvas_can_resize(video_canvas_t *canvas)
{
    return 1;
}

void sdl_ui_init_finalize(void)
{
    unsigned int width = sdl_active_canvas->draw_buffer->canvas_width;
    unsigned int height = sdl_active_canvas->draw_buffer->canvas_height;
    int minimized = 0;

    /* unfortunately we cant create the window minimized in SDL1 */
    resources_get_int("StartMinimized", &minimized);

    sdl_canvas_create(sdl_active_canvas, &width, &height); /* set the real canvas size */
    /* minimize window after it was created */
    if (minimized) {
        SDL_WM_IconifyWindow();
    }

    sdl_ui_finalized = 1;

    mousedrv_mouse_changed();
}

int sdl_ui_get_mouse_state(int *px, int *py, unsigned int *pbuttons)
{
    int local_x, local_y, local_buttons;
    if (!(SDL_GetAppState() & SDL_APPMOUSEFOCUS)) {
        /* We don't have mouse focus */
        return 0;
    }

    local_buttons = SDL_GetMouseState(&local_x, &local_y);

#ifdef SDL_DEBUG
    fprintf(stderr, "%s pre : x = %i, y = %i, buttons = %02x, on_screen = %i\n", __func__, px, py, buttons, on_screen);
#endif

    local_x -= sdl_lightpen_adjust.offset_x;
    local_y -= sdl_lightpen_adjust.offset_y;

    if ((local_x < 0) || (local_y < 0) || (local_x >= sdl_lightpen_adjust.max_x) || (local_y >= sdl_lightpen_adjust.max_y)) {
        return 0;
    } else {
        local_x = (int)(local_x * sdl_lightpen_adjust.scale_x);
        local_y = (int)(local_y * sdl_lightpen_adjust.scale_y);
    }

#ifdef SDL_DEBUG
    fprintf(stderr, "%s post: x = %i, y = %i\n", __func__, x, y);
#endif
    if (px) {
        *px = local_x;
    }
    if (py) {
        *py = local_y;
    }
    if (pbuttons) {
        *pbuttons = local_buttons;
    }
    return 1;
}

void sdl_ui_consume_mouse_event(SDL_Event *event)
{
    /* This is a no-op on SDL1 */
    ui_autohide_mouse_cursor();
}

void sdl_ui_set_window_title(char *title)
{
    char *dummy = NULL;
    char *icon = NULL;

    if (sdl_ui_finalized) {
        SDL_WM_GetCaption(&dummy, &icon);
        SDL_WM_SetCaption(title, icon);
    }
}
