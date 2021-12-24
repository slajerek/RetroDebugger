/*
 * video.c - SDL video
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Ettore Perazzoli
 *  Andre Fachat
 *  Oliver Schaertel
 *  Martin Pottendorfer
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

#include "cmdline.h"
#include "fullscreen.h"
#include "fullscreenarch.h"
#include "lib.h"
#include "lightpendrv.h"
#include "log.h"
#include "palette.h"
#include "raster.h"
#include "resources.h"
#include "translate.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "util.h"
#include "videoarch.h"
#include "vkbd.h"
#include "vsidui_sdl.h"
#include "vsync.h"

#include "ViceWrapper.h"

#ifdef SDL_DEBUG
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

static log_t sdlvideo_log = LOG_ERR;

static int sdl_bitdepth;

static int sdl_limit_mode;
static int sdl_ui_finalized;

/* Custom w/h, used for fullscreen and limiting*/
static int sdl_custom_width = 0;
static int sdl_custom_height = 0;

/* window size, used for free scaling */
static int sdl_window_width = 0;
static int sdl_window_height = 0;

int sdl_active_canvas_num = 0;
static int sdl_num_screens = 0;
static video_canvas_t *sdl_canvaslist[MAX_CANVAS_NUM];
video_canvas_t *sdl_active_canvas = NULL;

#ifdef HAVE_HWSCALE
static int sdl_gl_mode;
//static GLint screen_texture;

static int sdl_gl_aspect_mode;
static char *aspect_ratio_s = NULL;
static double aspect_ratio;

static int sdl_gl_flipx;
static int sdl_gl_flipy;

static int sdl_gl_vertex_base = 0;

static const float sdl_gl_vertex_coord[4*4] = {
    /* Normal */
    -1.0f, +1.0f, -1.0f, +1.0f,
    /* Flip X */
    +1.0f, +1.0f, -1.0f, -1.0f,
    /* Flip Y */
    -1.0f, -1.0f, +1.0f, +1.0f,
    /* Flip X&Y */
    +1.0f, -1.0f, +1.0f, -1.0f
};
#endif

/* ------------------------------------------------------------------------- */
/* Video-related resources.  */

static int set_sdl_bitdepth(int d, void *param)
{
    if ((d == 0) || (d == 8) || (d == 15) || (d == 16) || (d == 24) || (d == 32)) {
        if (sdl_bitdepth == d) {
            return 0;
        }
        sdl_bitdepth = d;
#ifdef HAVE_HWSCALE
        if (!((d == 0) || (d == 24) || (d == 32))) {
            resources_set_int("HwScalePossible", 0);
        }
#endif
        /* update */
        return 0;
    }
    return -1;
}

static int set_sdl_limit_mode(int v, void *param)
{
    if ((v < SDL_LIMIT_MODE_OFF) || (v > SDL_LIMIT_MODE_FIXED)) {
        return -1;
    }

    if (sdl_limit_mode != v) {
        sdl_limit_mode = v;
        video_viewport_resize(sdl_active_canvas, 1);
    }
    return 0;
}

static int set_sdl_custom_width(int w, void *param)
{
    if (w <= 0) {
        return -1;
    }

    if (sdl_custom_width != w) {
        sdl_custom_width = w;
        if (sdl_active_canvas && sdl_active_canvas->fullscreenconfig->enable
                && sdl_active_canvas->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
            video_viewport_resize(sdl_active_canvas, 1);
        }
    }
    return 0;
}

static int set_sdl_custom_height(int h, void *param)
{
    if (h <= 0) {
        return -1;
    }

    if (sdl_custom_height != h) {
        sdl_custom_height = h;
        if (sdl_active_canvas && sdl_active_canvas->fullscreenconfig->enable
                && sdl_active_canvas->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
            video_viewport_resize(sdl_active_canvas, 1);
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

#ifdef HAVE_HWSCALE
static int set_sdl_gl_aspect_mode(int v, void *param)
{
    int old_v = sdl_gl_aspect_mode;

    if ((v < SDL_ASPECT_MODE_OFF) || (v > SDL_ASPECT_MODE_TRUE)) {
        return -1;
    }

    sdl_gl_aspect_mode = v;

    if (old_v != v) {
        if (sdl_active_canvas && sdl_active_canvas->videoconfig->hwscale) {
            video_viewport_resize(sdl_active_canvas, 1);
        }
    }

    return 0;
}

static int set_aspect_ratio(const char *val, void *param)
{	
    double old_aspect = aspect_ratio;
    char buf[20];

	LOGD("set_aspect_ratio");

    if (val) {
        char *endptr;

        util_string_set(&aspect_ratio_s, val);

        aspect_ratio = strtod(val, &endptr);
        if (val == endptr) {
            aspect_ratio = 1.0;
        } else if (aspect_ratio < 0.5) {
            aspect_ratio = 0.5;
        } else if (aspect_ratio > 2.0) {
            aspect_ratio = 2.0;
        }
    } else {
        aspect_ratio = 1.0;
    }

    sprintf(buf, "%f", aspect_ratio);
    util_string_set(&aspect_ratio_s, buf);

    if (old_aspect != aspect_ratio) {
        if (sdl_active_canvas && sdl_active_canvas->videoconfig->hwscale) {
            video_viewport_resize(sdl_active_canvas, 1);
        }
    }

    return 0;
}

static void update_vertex_base(void)
{
    sdl_gl_vertex_base = (sdl_gl_flipx << 2) | (sdl_gl_flipy << 3);
}

static int set_sdl_gl_flipx(int v, void *param)
{
    sdl_gl_flipx = (v != 0);
    update_vertex_base();

    return 0;
}

static int set_sdl_gl_flipy(int v, void *param)
{
    sdl_gl_flipy = (v != 0);
    update_vertex_base();

    return 0;
}

#endif /* HAVE_HWSCALE */

static const resource_string_t resources_string[] = {
#ifdef HAVE_HWSCALE
    { "AspectRatio", "1.0", RES_EVENT_NO, NULL,
      &aspect_ratio_s, set_aspect_ratio, NULL },
#endif
    RESOURCE_STRING_LIST_END
};

#ifdef WATCOM_COMPILE
#define VICE_DEFAULT_BITDEPTH 32
#else
#define VICE_DEFAULT_BITDEPTH 0
#endif

static const resource_int_t resources_int[] = {
    { "SDLBitdepth", VICE_DEFAULT_BITDEPTH, RES_EVENT_NO, NULL,
      &sdl_bitdepth, set_sdl_bitdepth, NULL },
    { "SDLLimitMode", SDL_LIMIT_MODE_OFF, RES_EVENT_NO, NULL,
      &sdl_limit_mode, set_sdl_limit_mode, NULL },
    { "SDLCustomWidth", 800, RES_EVENT_NO, NULL,
      &sdl_custom_width, set_sdl_custom_width, NULL },
    { "SDLCustomHeight", 600, RES_EVENT_NO, NULL,
      &sdl_custom_height, set_sdl_custom_height, NULL },
    { "SDLWindowWidth", 0, RES_EVENT_NO, NULL,
      &sdl_window_width, set_sdl_window_width, NULL },
    { "SDLWindowHeight", 0, RES_EVENT_NO, NULL,
      &sdl_window_height, set_sdl_window_height, NULL },
#ifdef HAVE_HWSCALE
    { "SDLGLAspectMode", SDL_ASPECT_MODE_TRUE, RES_EVENT_NO, NULL,
      &sdl_gl_aspect_mode, set_sdl_gl_aspect_mode, NULL },
    { "SDLGLFlipX", 0, RES_EVENT_NO, NULL,
      &sdl_gl_flipx, set_sdl_gl_flipx, NULL },
    { "SDLGLFlipY", 0, RES_EVENT_NO, NULL,
      &sdl_gl_flipy, set_sdl_gl_flipy, NULL },
#endif
    RESOURCE_INT_LIST_END
};

int video_arch_resources_init(void)
{
    DBG(("%s", __func__));
    if (resources_register_string(resources_string) < 0)
        return -1;

    return resources_register_int(resources_int);
}

void video_arch_resources_shutdown(void)
{
    DBG(("%s", __func__));
#ifdef HAVE_HWSCALE
    lib_free(aspect_ratio_s);
#endif
}

/* ------------------------------------------------------------------------- */
/* Video-related command-line options.  */

static const cmdline_option_t cmdline_options[] = {
    { "-sdlbitdepth", SET_RESOURCE, 1, NULL, NULL, "SDLBitdepth", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      "<bpp>", "Set bitdepth (0 = current, 8, 15, 16, 24, 32)" },
    { "-sdllimitmode", SET_RESOURCE, 1, NULL, NULL, "SDLLimitMode", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      "<mode>", "Set resolution limiting mode (0 = off, 1 = max, 2 = fixed)" },
    { "-sdlcustomw", SET_RESOURCE, 1, NULL, NULL, "SDLCustomWidth", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      "<width>", "Set custom resolution width" },
    { "-sdlcustomh", SET_RESOURCE, 1, NULL, NULL, "SDLCustomHeight", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      "<height>", "Set custom resolution height" },
#ifdef HAVE_HWSCALE
    { "-sdlaspectmode", SET_RESOURCE, 1, NULL, NULL, "SDLGLAspectMode", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      "<mode>", "Set aspect ratio mode (0 = off, 1 = custom, 2 = true)" },
    { "-aspect", SET_RESOURCE, 1, NULL, NULL, "AspectRatio", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      "<aspect ratio>", "Set custom aspect ratio (0.5 - 2.0)" },
    { "-sdlflipx", SET_RESOURCE, 0, NULL, NULL, "SDLGLFlipX", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      NULL, "Enable X flip" },
    { "+sdlflipx", SET_RESOURCE, 0, NULL, NULL, "SDLGLFlipX", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      NULL, "Disable X flip" },
    { "-sdlflipy", SET_RESOURCE, 0, NULL, NULL, "SDLGLFlipY", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      NULL, "Enable Y flip" },
    { "+sdlflipy", SET_RESOURCE, 0, NULL, NULL, "SDLGLFlipY", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_STRING, IDCLS_UNUSED, IDCLS_UNUSED,
      NULL, "Disable Y flip" },
#endif
    CMDLINE_LIST_END
};

int video_init_cmdline_options(void)
{
    DBG(("%s", __func__));
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

int video_init(void)
{
	LOGD("video_init");
	
    sdlvideo_log = log_open("SDLVideo");
    return 0;
}

void video_shutdown(void)
{
    DBG(("%s", __func__));
    sdl_active_canvas = NULL;
}

/* ------------------------------------------------------------------------- */
/* static helper functions */

static int sdl_video_canvas_limit(unsigned int limit_w, unsigned int limit_h, unsigned int *w, unsigned int *h, int mode)
{
    DBG(("%s",__func__));
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

#ifdef HAVE_HWSCALE
static void sdl_gl_set_viewport(unsigned int src_w, unsigned int src_h, unsigned int dest_w, unsigned int dest_h)
{
    int dest_x = 0, dest_y = 0;

    if (sdl_gl_aspect_mode != SDL_ASPECT_MODE_OFF) {
        double aspect = aspect_ratio;

        /* Get "true" aspect ratio */
        if (sdl_gl_aspect_mode == SDL_ASPECT_MODE_TRUE) {
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

    //glViewport(dest_x, dest_y, dest_w, dest_h);
}
#endif

static video_canvas_t *sdl_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height)
{
	LOGD("sdl_canvas_create");
	
//    SDL_Surface *new_screen;
//    unsigned int new_width, new_height;
//    unsigned int actual_width, actual_height;
//    int flags;
//    int fullscreen = 0;
//    int limit = sdl_limit_mode;
//    unsigned int limit_w = (unsigned int)sdl_custom_width;
//    unsigned int limit_h = (unsigned int)sdl_custom_height;
//    int hwscale = 0;
//    int lightpen_updated = 0;
//#ifdef HAVE_HWSCALE
//    int rbits, gbits, bbits;
//    const Uint32
//
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//    rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00, amask = 0x000000ff;
//#else
//    rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
//#endif
//#endif
//
//    DBG(("%s: %i,%i (%i)", __func__, *width, *height, canvas->index));
//
//    flags = SDL_SWSURFACE | SDL_RESIZABLE;
//
//    new_width = *width;
//    new_height = *height;
//
//    if (canvas->videoconfig->doublesizex) {
//        new_width *= (canvas->videoconfig->doublesizex + 1);
//    }
//
//    if (canvas->videoconfig->doublesizey) {
//        new_height *= (canvas->videoconfig->doublesizey + 1);
//    }
//
//    if ((canvas == sdl_active_canvas) && (canvas->fullscreenconfig->enable)) {
//        fullscreen = 1;
//    }
//
//#ifdef HAVE_HWSCALE
//    if ((canvas == sdl_active_canvas) && (canvas->videoconfig->hwscale)) {
//        hwscale = 1;
//    }
//#endif
//
//    if (fullscreen) {
//        flags = SDL_FULLSCREEN | SDL_SWSURFACE;
//
//        if (canvas->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
//            limit = SDL_LIMIT_MODE_FIXED;
//        }
//    }
//
//    if (!sdl_ui_finalized) { /* remember first size */
//        double aspect = 1.0;
//#ifdef HAVE_HWSCALE
//        aspect = aspect_ratio;
//
//        if (sdl_gl_aspect_mode == SDL_ASPECT_MODE_TRUE) {
//            aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
//        }
//#endif
//        sdl_active_canvas->real_width = (unsigned int)((double)new_width * aspect + 0.5);
//        sdl_active_canvas->real_height = new_height;
//    }
//
//#ifdef HAVE_HWSCALE
//    if (hwscale) {
//        flags |= SDL_OPENGL;
//
//        if (fullscreen) {
//            limit = SDL_LIMIT_MODE_OFF;
//        } else {
//            double aspect = aspect_ratio;
//
//            if (sdl_gl_aspect_mode == SDL_ASPECT_MODE_TRUE) {
//                aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
//            }
//
//            /* if no window geometry given then create one. */
//            if (!sdl_window_width || !sdl_window_height) {
//                limit_w = (unsigned int)((double)new_width * aspect + 0.5);
//                limit_h = new_height;
//            } else { /* full window size remembering when aspect ratio is not important */
//                if (sdl_gl_aspect_mode == SDL_ASPECT_MODE_OFF) {
//                    limit_w = (unsigned int)sdl_window_width;
//                    limit_h = (unsigned int)sdl_window_height;
//                } else { /* only remember height, set width according to that and the aspect ratio */
//                    limit_h = (unsigned int)sdl_window_height;
//                    limit_w = (unsigned int)((double)new_width * (double)sdl_window_height * aspect / (double)new_height + 0.5);
//                }
//            }
//            limit = SDL_LIMIT_MODE_FIXED;
//        }
//
//        switch (sdl_bitdepth) {
//            case 0:
//                log_warning(sdlvideo_log, "bitdepth not set for OpenGL, trying 32...");
//                sdl_bitdepth = 32;
//                /* fall through */
//            case 32:
//                rbits = gbits = bbits = 8;
//                sdl_gl_mode = GL_RGBA;
//                break;
//            case 24:
//                rbits = gbits = bbits = 8;
//                sdl_gl_mode = GL_RGB;
//                break;
//            default:
//                log_error(sdlvideo_log, "%i bpp not supported in OpenGL.", sdl_bitdepth);
//                resources_set_int("HwScalePossible", 0);
//                hwscale = 0;
//                canvas->videoconfig->hwscale = 0;
//                flags = SDL_SWSURFACE;
//                break;
//        }
//
//        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, rbits);
//        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, gbits);
//        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bbits);
//    }
//#endif
//
//    actual_width = new_width;
//    actual_height = new_height;
//
//    if (canvas == sdl_active_canvas) {
//#ifdef HAVE_HWSCALE
//        if (hwscale) {
//            double aspect = aspect_ratio;
//
//            if (sdl_gl_aspect_mode == SDL_ASPECT_MODE_TRUE) {
//                aspect = sdl_active_canvas->geometry->pixel_aspect_ratio;
//            }
//
//            actual_width = (unsigned int)((double)actual_width * aspect + 0.5);
//        }
//#endif
//        if (sdl_video_canvas_limit(limit_w, limit_h, &actual_width, &actual_height, limit)) {
//            if (!hwscale) {
//                canvas->draw_buffer->canvas_physical_width = actual_width;
//                canvas->draw_buffer->canvas_physical_height = actual_height;
//                video_viewport_resize(sdl_active_canvas, 0);
//                if (sdl_ui_finalized) {
//                    return canvas; /* exit here as video_viewport_resize will recall */
//                }
//            }
//        }
//    }
//
//    if (canvas == sdl_active_canvas) {
//        SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
//#ifndef HAVE_HWSCALE
//        new_screen = SDL_SetVideoMode(actual_width, actual_height, sdl_bitdepth, flags);
//        new_width = new_screen->w;
//        new_height = new_screen->h;
//#else
//        if (hwscale) {
//            new_screen = SDL_SetVideoMode(actual_width, actual_height, sdl_bitdepth, flags);
//            actual_width = new_screen->w;
//            actual_height = new_screen->h;
//
//            /* free the old rendering surface when staying in hwscale mode */
//            if ((canvas->hwscale_screen)&&(canvas->screen)) {
//                SDL_FreeSurface(canvas->screen);
//            }
//
//            canvas->hwscale_screen = new_screen;
//            new_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, new_width, new_height, sdl_bitdepth, rmask, gmask, bmask, amask);
//            sdl_gl_set_viewport(new_width, new_height, actual_width, actual_height);
//            lightpen_updated = 1;
//        } else {
//            new_screen = SDL_SetVideoMode(actual_width, actual_height, sdl_bitdepth, flags);
//            new_width = new_screen->w;
//            new_height = new_screen->h;
//
//            /* free the old rendering surface when leaving hwscale mode */
//            if ((canvas->hwscale_screen) && (canvas->screen)) {
//                SDL_FreeSurface(canvas->screen);
//                SDL_FreeSurface(canvas->hwscale_screen);
//                canvas->hwscale_screen = NULL;
//            }
//        }
//#endif
//        SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE);
//    } else {
//#ifdef HAVE_HWSCALE
//        /* free the old hwscale screen when hwscaled screen is switched away */
//        if (canvas->hwscale_screen) {
//            SDL_FreeSurface(canvas->hwscale_screen);
//            canvas->hwscale_screen = NULL;
//        }
//        if (!hwscale) {
//            new_width = actual_width;
//            new_height = actual_height;
//        }
//#else
//        new_width = actual_width;
//        new_height = actual_height;
//#endif
//        if (canvas->screen) {
//            SDL_FreeSurface(canvas->screen);
//        }
//        new_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, new_width, new_height, sdl_bitdepth, 0, 0, 0, 0);
//    }
//
//    if (!new_screen) {
//        log_error(sdlvideo_log, "SDL_SetVideoMode failed!");
//        return NULL;
//    }
//    sdl_bitdepth = new_screen->format->BitsPerPixel;
//
//    canvas->depth = sdl_bitdepth;
//    canvas->width = new_width;
//    canvas->height = new_height;
//    canvas->screen = new_screen;
//    canvas->actual_width = actual_width;
//    canvas->actual_height = actual_height;
//
//    if (!fullscreen) {
//        resources_set_int("SDLWindowWidth", actual_width);
//        resources_set_int("SDLWindowHeight", actual_height);
//    }
//
//    log_message(sdlvideo_log, "%ix%i %ibpp %s%s", actual_width, actual_height, sdl_bitdepth, hwscale?"OpenGL ":"", (canvas->fullscreenconfig->enable)?"(fullscreen)":"");
//#ifdef SDL_DEBUG
//    log_message(sdlvideo_log, "Canvas %ix%i, real %ix%i", new_width, new_height, canvas->real_width, canvas->real_height);
//#endif
//
//    /* Update lightpen adjustment parameters */
//    if (canvas == sdl_active_canvas && !lightpen_updated) {
//        sdl_lightpen_adjust.max_x = actual_width;
//        sdl_lightpen_adjust.max_y = actual_height;
//
//        sdl_lightpen_adjust.scale_x = (double)*width / (double)actual_width;
//        sdl_lightpen_adjust.scale_y = (double)*height / (double)actual_height;
//    }
//
//    if (canvas == sdl_active_canvas) {
//        video_canvas_set_palette(canvas, canvas->palette);
//    }
//
//    return canvas;
	
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* Main API */

/* called from raster/raster.c:realize_canvas */
video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
	LOGD("video_canvas_create");
	
    /* nothing to do here, the real work is done in sdl_ui_init_finalize */
    return canvas;
}

void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
	//LOGD("*********** video_canvas_refresh: xs=%d ys=%d xi=%d yi=%d w=%d h=%d", xs, ys, xi, yi, w, h);
	
	//    video_canvas_render(canvas, (BYTE *)canvas->screen->pixels, w, h, xs, ys, xi, yi, canvas->screen->pitch, canvas->screen->format->BitsPerPixel);

	// draw directly from c64 screen
	
	// from: canvas->draw_buffer->draw_buffer
	c64d_refresh_screen();
	
	
//    if ((canvas == NULL) || (canvas->screen == NULL)) {
//        return;
//    }
//
//    if (sdl_vsid_state & SDL_VSID_ACTIVE) {
//        sdl_vsid_draw();
//    }
//
//    if (sdl_vkbd_state & SDL_VKBD_ACTIVE) {
//        sdl_vkbd_draw();
//    }
//
//    if (uistatusbar_state & UISTATUSBAR_ACTIVE) {
//        uistatusbar_draw();
//    }
//
//    if (canvas->videoconfig->doublesizex) {
//        xi *= (canvas->videoconfig->doublesizex + 1);
//        w *= (canvas->videoconfig->doublesizex + 1);
//    }
//
//    if (canvas->videoconfig->doublesizey) {
//        yi *= (canvas->videoconfig->doublesizey + 1);
//        h *= (canvas->videoconfig->doublesizey + 1);
//    }
//
//    w = MIN(w, canvas->width);
//    h = MIN(h, canvas->height);
//
//    /* FIXME attempt to draw outside canvas */
//    if ((xi + w > canvas->width)||(yi + h > canvas->height)) {
//        return;
//    }
//
//    if (SDL_MUSTLOCK(canvas->screen)) {
//        canvas->videoconfig->readable = 0;
//        if (SDL_LockSurface(canvas->screen) < 0) {
//            return;
//        }
//    } else { /* no direct rendering, safe to read */
//        canvas->videoconfig->readable = !(canvas->screen->flags & SDL_HWSURFACE);
//    }
//
//    video_canvas_render(canvas, (BYTE *)canvas->screen->pixels, w, h, xs, ys, xi, yi, canvas->screen->pitch, canvas->screen->format->BitsPerPixel);
//
//    if (SDL_MUSTLOCK(canvas->screen)) {
//        SDL_UnlockSurface(canvas->screen);
//    }
//
//#ifdef HAVE_HWSCALE
//    if (canvas->videoconfig->hwscale) {
//        const float *v = &(sdl_gl_vertex_coord[sdl_gl_vertex_base]);
//
//        if (canvas != sdl_active_canvas) {
//            DBG(("%s: not active SDL canvas, ignoring", __func__));
//            return;
//        }
//
//        if (!(canvas->hwscale_screen)) {
//            DBG(("%s: hwscale refresh without hwscale screen, ignoring", __func__));
//            return;
//        }
//
///* XXX make use of glXBindTexImageEXT aka texture from pixmap extension */
//
//        glClear(GL_COLOR_BUFFER_BIT);
//        glDisable(GL_DEPTH_TEST);
//
///* GL_TEXTURE_RECTANGLE is standardised as _EXT in OpenGL 1.4. Here's some
// * aliases in the meantime. */
//#ifndef GL_TEXTURE_RECTANGLE_EXT
//    #if defined(GL_TEXTURE_RECTANGLE_NV)
//        #define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_NV
//    #elif defined(GL_TEXTURE_RECTANGLE_ARB)
//        #define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_ARB
//    #else
//        #error "Your headers do not supply GL_TEXTURE_RECTANGLE. Disable HWSCALE and try again."
//    #endif
//#endif
//
//        glEnable(GL_TEXTURE_RECTANGLE_EXT);
//        glBindTexture(GL_TEXTURE_RECTANGLE_EXT, screen_texture);
//        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, sdl_gl_mode, canvas->width, canvas->height, 0, sdl_gl_mode, GL_UNSIGNED_BYTE, canvas->screen->pixels);
//
//        glBegin(GL_QUADS);
//
//        /* Lower Right Of Texture */
//        glTexCoord2f(0.0f, 0.0f);
//        glVertex2f(v[0], v[1]);
//
//        /* Upper Right Of Texture */
//        glTexCoord2f(0.0f, (float)(canvas->height));
//        glVertex2f(v[0], v[2]);
//
//        /* Upper Left Of Texture */
//        glTexCoord2f((float)(canvas->width), (float)(canvas->height));
//        glVertex2f(v[3], v[2]);
//
//        /* Lower Left Of Texture */
//        glTexCoord2f((float)(canvas->width), 0.0f);
//        glVertex2f(v[3], v[1]);
//
//        glEnd();
//
//        SDL_GL_SwapBuffers();
//    } else
//#endif
//    SDL_UpdateRect(canvas->screen, xi, yi, w, h);
}

int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette)
{
//    unsigned int i, col;
//    SDL_PixelFormat *fmt;
//    SDL_Color colors[256];
//
//    if (palette == NULL) {
//        return 0; /* no palette, nothing to do */
//    }
//
//    canvas->palette = palette;
//
//    fmt = canvas->screen->format;
//
//    for (i = 0; i < palette->num_entries; i++) {
//        if (canvas->depth == 8) {
//            colors[i].r = palette->entries[i].red;
//            colors[i].b = palette->entries[i].blue;
//            colors[i].g = palette->entries[i].green;
//            col = i;
//        } else {
//            col = SDL_MapRGB(fmt, palette->entries[i].red, palette->entries[i].green, palette->entries[i].blue);
//        }
//        video_render_setphysicalcolor(canvas->videoconfig, i, col, canvas->depth);
//    }
//
//    if (canvas->depth == 8) {
//        SDL_SetColors(canvas->screen, colors, 0, palette->num_entries);
//    } else {
//        for (i = 0; i < 256; i++) {
//            video_render_setrawrgb(i, SDL_MapRGB(fmt, (Uint8)i, 0, 0), SDL_MapRGB(fmt, 0, (Uint8)i, 0), SDL_MapRGB(fmt, 0, 0, (Uint8)i));
//        }
//        video_render_initraw(canvas->videoconfig);
//    }
//
    return 0;
}

/* called from video_viewport_resize */
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
	LOGD("video_canvas_resize");
	
//    unsigned int width = canvas->draw_buffer->canvas_width;
//    unsigned int height = canvas->draw_buffer->canvas_height;
//    DBG(("%s: %ix%i (%i)", __func__, width, height, canvas->index));
//    /* Check if canvas needs to be resized to real size first */
//    if (sdl_ui_finalized) {
//        sdl_window_width = 0; /* force recalculate */
//        sdl_window_height = 0;
//        sdl_canvas_create(canvas, &width, &height);/* set the real canvas size */
//
//        if (resize_canvas) {
//            DBG(("%s: set and resize to real size (%ix%i)", __func__, width, height));
//            canvas->real_width = canvas->actual_width;
//            canvas->real_height = canvas->actual_height;
//        }
//    }
}

/* Resize window to w/h. */
static void sdl_video_resize(unsigned int w, unsigned int h)
{
    DBG(("%s: %ix%i", __func__, w, h));

    if ((w == 0) || (h == 0)) {
        DBG(("%s: ERROR, ignored!", __func__));
        return;
    }

//    vsync_suspend_speed_eval();
//
//#ifdef HAVE_HWSCALE
//    if (sdl_active_canvas->videoconfig->hwscale && sdl_active_canvas->hwscale_screen) {
//        int flags;
//
//        if (sdl_active_canvas->fullscreenconfig->enable) {
//            flags = SDL_OPENGL | SDL_SWSURFACE | SDL_FULLSCREEN;
//        } else {
//            flags = SDL_OPENGL | SDL_SWSURFACE | SDL_RESIZABLE;
//        }
//
//        SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
//        sdl_active_canvas->hwscale_screen = SDL_SetVideoMode((int)w, (int)h, sdl_bitdepth, flags);
//        SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE);
//
//#ifdef SDL_DEBUG
//        if (!sdl_active_canvas->hwscale_screen) {
//            DBG(("%s: setting video mode failed", __func__));
//        }
//#endif
//        sdl_gl_set_viewport(sdl_active_canvas->width, sdl_active_canvas->height, w, h);
//        sdl_active_canvas->actual_width = w;
//        sdl_active_canvas->actual_height = h;
//    } else
//#endif /*  HAVE_HWSCALE */
//    {
//        sdl_active_canvas->draw_buffer->canvas_physical_width = w;
//        sdl_active_canvas->draw_buffer->canvas_physical_height = h;
//        video_viewport_resize(sdl_active_canvas, 0);
//    }
//}
//
///* Resize window to stored real size */
//void sdl_video_restore_size(void)
//{
//    unsigned int w, h;
//
//    w = sdl_active_canvas->real_width;
//    h = sdl_active_canvas->real_height;
//
//    DBG(("%s: %ix%i->%ix%i", __func__, sdl_active_canvas->real_width, sdl_active_canvas->real_height, w, h));
//    sdl_video_resize(w, h);
}

/* special case handling for the SDL window resize event */
void sdl_video_resize_event(unsigned int w, unsigned int h)
{
//#ifdef HAVE_HWSCALE
//
//    DBG(("%s: %ix%i", __func__, w, h));
//    if ((w == 0) || (h == 0)) {
//        DBG(("%s: ERROR, ignored!", __func__));
//        return;
//    }
//    sdl_video_resize(w, h);
//    resources_set_int("SDLWindowWidth", sdl_active_canvas->actual_width);
//    resources_set_int("SDLWindowHeight", sdl_active_canvas->actual_height);
//
//#endif /*  HAVE_HWSCALE */
//}
//
//void sdl_video_canvas_switch(int index)
//{
//    struct video_canvas_s *canvas;
//
//    DBG(("%s: %i->%i", __func__, sdl_active_canvas_num, index));
//
//    if (sdl_active_canvas_num == index) {
//        return;
//    }
//
//    if (index >= sdl_num_screens) {
//        return;
//    }
//
//    if (sdl_canvaslist[index]->screen != NULL) {
//        SDL_FreeSurface(sdl_canvaslist[index]->screen);
//        sdl_canvaslist[index]->screen = NULL;
//    }
//
//    sdl_active_canvas_num = index;
//
//    canvas = sdl_canvaslist[sdl_active_canvas_num];
//    sdl_active_canvas = canvas;
//    
//    video_viewport_resize(canvas, 1);
}

void video_arch_canvas_init(struct video_canvas_s *canvas)
{
	DBG(("%s: (%p, %i)", __func__, canvas, sdl_num_screens));

//    DBG(("%s: (%p, %i)", __func__, canvas, sdl_num_screens));
//
//    if (sdl_num_screens == MAX_CANVAS_NUM) {
//        log_error(sdlvideo_log,"Too many canvases!");
//        exit(-1);
//    }
//
//    canvas->video_draw_buffer_callback = NULL;
//
//    canvas->fullscreenconfig = lib_calloc(1, sizeof(fullscreenconfig_t));
//
//    if (sdl_active_canvas_num == sdl_num_screens) {
//        sdl_active_canvas = canvas;
//    }
//
//    canvas->index = sdl_num_screens;
//
//    sdl_canvaslist[sdl_num_screens++] = canvas;
//
//    canvas->screen = NULL;
//#ifdef HAVE_HWSCALE
//    canvas->hwscale_screen = NULL;
//#endif
//    canvas->real_width = 0;
//    canvas->real_height = 0;
}

void video_canvas_destroy(struct video_canvas_s *canvas)
{
//    int i;
//
//    DBG(("%s: (%p, %i)", __func__, canvas, canvas->index));
//
//    for (i = 0; i < sdl_num_screens; ++i) {
//        if ((sdl_canvaslist[i] == canvas) && (i != sdl_active_canvas_num)) {
//            SDL_FreeSurface(sdl_canvaslist[i]->screen);
//            sdl_canvaslist[i]->screen = NULL;
//        }
//    }
//
//    lib_free(canvas->fullscreenconfig);
}

void video_add_handlers(void)
{
    DBG(("%s", __func__));
}

char video_canvas_can_resize(video_canvas_t *canvas)
{
    return 1;
}

void sdl_ui_init_finalize(void)
{
//    unsigned int width = sdl_active_canvas->draw_buffer->canvas_width;
//    unsigned int height = sdl_active_canvas->draw_buffer->canvas_height;
//
//    sdl_canvas_create(sdl_active_canvas, &width, &height);/* set the real canvas size */
//    sdl_ui_finalized = 1;
}

int video_arch_cmdline_options_init(void)
{
	DBG(("%s", __func__));
 
//	if (machine_class == VICE_MACHINE_VSID) {
//		if (joystick_cmdline_options_init() < 0) {
//			return -1;
//		}
//	}
// 
	return cmdline_register_options(cmdline_options);
}
