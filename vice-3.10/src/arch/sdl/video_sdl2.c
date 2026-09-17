/*
 * video_sdl2.c - SDL2 video
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Michael C. Martin <mcmartin@gmail.com>
 *  June Tate-Gans <june@theonelab.com>
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

/* This file is a work in progress, and it is being refactored,
 * rewritten, and extended to take the maximum advantage of SDL2's
 * capabilities while still properly providing the special
 * capabilities that SDL VICE would like to generally support. Current
 * gaps:
 *
 * - The menu display sometimes ends up having a bad display until the
 *   user moves the cursor or resizes the window.
 * - The user's selected window size is not preserved across runs.
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
#include "lib.h"
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

/* Initial x/y for windowed display */
static int sdl_initial_xpos[2] = { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED };
static int sdl_initial_ypos[2] = { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED };

/* Initial w/h for windowed display */
static int sdl_initial_width[2] = { 0, 0 };
static int sdl_initial_height[2] = { 0, 0 };

int sdl_active_canvas_num = 0;
static int sdl_num_screens = 0;
static video_canvas_t *sdl_canvaslist[MAX_CANVAS_NUM];
video_canvas_t *sdl_active_canvas = NULL;

static int sdl2_dual_window;

static char *sdl2_renderer_name = NULL;

static Uint32 rmask = 0, gmask = 0, bmask = 0, amask = 0;
static int texformat = 0;
static int recreate_textures = 0;

uint8_t *draw_buffer_vsid = NULL;
/* ------------------------------------------------------------------------- */
/* Video-related resources.  */

static void sdl_correct_logical_size(void);
static void sdl_correct_logical_and_minimum_size(void);

static int set_sdl_bitdepth(int d, void *param)
{
    switch (d) {
        case 32:
            texformat = SDL_PIXELFORMAT_ARGB8888;
            rmask = 0x00ff0000, gmask = 0x0000ff00, bmask = 0x000000ff, amask = 0xff000000;
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

static int set_sdl_initial_xpos(int x, void *param)
{
    int idx = vice_ptr_to_int(param);
    sdl_initial_xpos[idx] = x;
    return 0;
}

static int set_sdl_initial_ypos(int y, void *param)
{
    int idx = vice_ptr_to_int(param);
    sdl_initial_ypos[idx] = y;
    return 0;
}

static int set_sdl_initial_width(int w, void *param)
{
    int idx = vice_ptr_to_int(param);
    if (w < 0) {
        return -1;
    }

    sdl_initial_width[idx] = w;
    return 0;
}

static int set_sdl_initial_height(int h, void *param)
{
    int idx = vice_ptr_to_int(param);
    if (h < 0) {
        return -1;
    }

    sdl_initial_height[idx] = h;
    return 0;
}

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
        sdl_correct_logical_and_minimum_size();
    }

    return 0;
}

/* called when the <CHIP>AspectRatio resource was set */
int ui_set_aspect_ratio(double aspect_ratio, void *canvas)
{
    video_canvas_t *cv = canvas;
    double old_aspect = cv->videoconfig->aspect_ratio;

    if (old_aspect != aspect_ratio) {
        if (canvas) {
            video_viewport_resize(canvas, 1);
            sdl_correct_logical_and_minimum_size();
        }
    }
    return 0;
}

/* called when the <CHIP>FlipX resource was set */
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

/* called when the <CHIP>FlipY resource was set */
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

/* called when the <CHIP>Rotate resource was set */
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

static void recreate_canvas_textures(video_canvas_t *canvas)
{
    SDL_Surface *surface;
    int width, height;

    if (!canvas || !canvas->container) {
        return;
    }

    surface = canvas->screen;
    if (!surface) {
        return;
    }

    width = surface->w;
    height = surface->h;

    /* This hint controls the scaling mode of textures created afterwards */
    if (canvas->videoconfig->glfilter == VIDEO_GLFILTER_BILINEAR) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    } else {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    }

    if (canvas->texture) {
        SDL_DestroyTexture(canvas->texture);
    }
    canvas->texture = SDL_CreateTexture(canvas->container->renderer, texformat, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!canvas->texture) {
        log_error(sdlvideo_log, "SDL_CreateTexture() failed on recreation: %s\n", SDL_GetError());
        return;
    }

    if (canvas->previous_frame_texture) {
        SDL_DestroyTexture(canvas->previous_frame_texture);
    }
    canvas->previous_frame_texture = SDL_CreateTexture(canvas->container->renderer, texformat, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!canvas->previous_frame_texture) {
        log_error(sdlvideo_log, "SDL_CreateTexture() failed on recreation: %s\n", SDL_GetError());
        return;
    }
}


static void recreate_all_textures(void)
{
    int i;

    for (i = 0; i < sdl_num_screens; ++i) {
        recreate_canvas_textures(sdl_canvaslist[i]);
    }
}

int ui_set_glfilter(int val, void *canvas)
{
    video_canvas_t *cv = canvas;
    if (val < 0) {
        val = 0;
    }
    if (val > 1) {
        val = 1;
    }
    cv->videoconfig->glfilter = val;
    recreate_textures = 1;
    return 0;
}

static int set_sdl2_renderer_name(const char *val, void *param)
{
    if (!val || val[0] == '\0') {
        util_string_set(&sdl2_renderer_name, "");
    } else {
        util_string_set(&sdl2_renderer_name, val);
    }
    return 0;
}

static int set_sdl2_dual_window(int v, void *param)
{
    sdl2_dual_window = v ? 1 : 0;

    return 0;
}

/* called when <CHIP>VSync was set */
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

static resource_string_t resources_string[] = {
    { "SDL2Backend", "", RES_EVENT_NO, NULL,
      &sdl2_renderer_name, set_sdl2_renderer_name, NULL },
    RESOURCE_STRING_LIST_END
};

#define VICE_DEFAULT_BITDEPTH 32

/* FIXME: more resources should have the same name as their GTK counterparts,
          and the SDL prefix removed */
static const resource_int_t resources_int[] = {
    { "SDLBitdepth", VICE_DEFAULT_BITDEPTH, RES_EVENT_NO, NULL,
      &sdl_bitdepth, set_sdl_bitdepth, NULL },
    { "DualWindow", 0, RES_EVENT_NO, NULL,
      &sdl2_dual_window, set_sdl2_dual_window, NULL },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window0Width", 0, RES_EVENT_NO, NULL,
      &sdl_initial_width[0], set_sdl_initial_width, (void*)0 },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window0Height", 0, RES_EVENT_NO, NULL,
      &sdl_initial_height[0], set_sdl_initial_height, (void*)0 },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window0Xpos", SDL_WINDOWPOS_CENTERED, RES_EVENT_NO, NULL,
      &sdl_initial_xpos[0], set_sdl_initial_xpos, (void*)0 },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window0Ypos", SDL_WINDOWPOS_CENTERED, RES_EVENT_NO, NULL,
      &sdl_initial_ypos[0], set_sdl_initial_ypos, (void*)0 },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_c128[] = {
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window1Width", 0, RES_EVENT_NO, NULL,
      &sdl_initial_width[1], set_sdl_initial_width, (void*)1 },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window1Height", 0, RES_EVENT_NO, NULL,
      &sdl_initial_height[1], set_sdl_initial_height, (void*)1 },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window1Xpos", SDL_WINDOWPOS_CENTERED, RES_EVENT_NO, NULL,
      &sdl_initial_xpos[1], set_sdl_initial_xpos, (void*)1 },
    /* FIXME: this is a generic (not SDL specific) resource */
    { "Window1Ypos", SDL_WINDOWPOS_CENTERED, RES_EVENT_NO, NULL,
      &sdl_initial_ypos[1], set_sdl_initial_ypos, (void*)1 },
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

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_C128) {
        if (resources_register_int(resources_int_c128) < 0) {
            return -1;
        }
    }

    return resources_register_int(resources_int);
}

void video_arch_resources_shutdown(void)
{
    DBG(("%s", __func__));

/* FIXME: loop over all canvas and free some stuff that is per videoconfig */
#if 0
    lib_free(canvas->videoconfig->aspect_ratio_s);
    lib_free(canvas->videoconfig->aspect_ratio_factory_value_s);
#endif
    lib_free(sdl2_renderer_name);
}

/* ------------------------------------------------------------------------- */
/* Video-related command-line options.  */

static const cmdline_option_t cmdline_options[] =
{
    { "-sdlbitdepth", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SDLBitdepth", NULL,
      "<bpp>", "Set bitdepth (0 = current, 8, 15, 16, 24, 32)" },
    { "-sdl2backend", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SDL2Backend", NULL,
      "<backend name>", "Set the preferred SDL2 backend" },
    { "-dualwindow", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DualWindow", (void *)1,
      NULL, "Enable dual window rendering"},
    { "+dualwindow", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DualWindow", (void *)0,
      NULL, "Disable dual window rendering"},
    /* Note: the following options are common/the same in GTK port */
    { "-windowwidth", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window0Width", NULL,
      "<width>", "Set initial window width" },
    { "-windowheight", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window0Height", NULL,
      "<height>", "Set initial window height" },
    { "-windowxpos", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window0Xpos", NULL,
      "<xpos>", "Set initial horizontal window position" },
    { "-windowypos", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window0Ypos", NULL,
      "<ypos>", "Set initial vertical window position" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_c128[] =
{
    /* Note: the following options are common/the same in GTK port */
    { "-windowwidth1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window1Width", NULL,
      "<width>", "Set initial window width" },
    { "-windowheight1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window1Height", NULL,
      "<height>", "Set initial window height" },
    { "-windowxpos1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window1Xpos", NULL,
      "<xpos>", "Set initial horizontal window position" },
    { "-windowypos1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Window1Ypos", NULL,
      "<ypos>", "Set initial vertical window position" },
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

    if (machine_class == VICE_MACHINE_C128) {
        if (cmdline_register_options(cmdline_options_c128) < 0) {
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

/** \brief  Given a canvas, generate a set of SDL_WindowFlags
 *
 * \return  A set of SDL_WindowFlags appropriate to the window resources.
 */
static SDL_WindowFlags sdl2_ui_generate_flags_for_canvas(const video_canvas_t* canvas)
{
    SDL_WindowFlags flags = 0;
    int minimized = 0;
    int maximized = 0;
    int hide_vdc = 0;

    if (machine_class == VICE_MACHINE_C128) {
        resources_get_int("C128HideVDC", &hide_vdc);
    }
    resources_get_int("StartMinimized", &minimized);
    resources_get_int("StartMaximized", &maximized);

    if (minimized && maximized) {
        log_warning(LOG_DEFAULT,
                    "both StartMinized and StartMaximized are true, ignoring.");
    } else if (minimized) {
        flags |= SDL_WINDOW_MINIMIZED;
    } else if (maximized) {
        flags |= SDL_WINDOW_MAXIMIZED;
    }

    if (hide_vdc && (canvas->index == VIDEO_CANVAS_IDX_VDC)) {
        flags |= SDL_WINDOW_HIDDEN;
    }

    if (canvas->fullscreenconfig->enable) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    } else {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    return flags;
}

/** \brief Destroys an sdl_container_t that was previously created using
 *         sdl_container_create.
 */
static void sdl_container_destroy(video_container_t* container)
{
    if (!container) {
        return;
    }

    if (container->renderer) {
        SDL_DestroyRenderer(container->renderer);
        container->renderer = NULL;
    }

    if (container->window) {
        SDL_DestroyWindow(container->window);
        container->window = NULL;
    }

    lib_free(container);
}

/** \brief  Given a canvas index, create an sdl_container_t and return it.
 *
 * This creates a window using the dimensions from the canvas referred to by
 * canvas_idx, and finally allocates a renderer. This does not allocate the
 * texture -- that is done by `video_canvas_resize`.
 *
 * \return  a fully initialized and allocated sdl_container_t struct, or NULL on
 *          failure.
 */
static video_container_t* sdl_container_create(int canvas_idx)
{
    char rendername[256] = { 0 };
    char **renderlist = NULL;
    int renderamount = SDL_GetNumRenderDrivers();
    int it, l;
    int drv_index;
    unsigned int window_width = 0, window_height = 0;
    int window_x = 0, window_y = 0;
    unsigned int width = 0, height = 0;
    SDL_RendererInfo info;
    video_canvas_t* canvas = sdl_canvaslist[canvas_idx];
    video_container_t* container = NULL;
    SDL_WindowFlags flags = sdl2_ui_generate_flags_for_canvas(canvas);
    int vsync = 0;

    container = lib_calloc(1, sizeof(*container));

    width = canvas->width;
    height = canvas->height;
    if (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_CUSTOM) {
        width *= canvas->videoconfig->aspect_ratio;
    }
    if (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
        width *= canvas->geometry->pixel_aspect_ratio;
    }

    window_width = width;
    window_height = height;

    DBG(("sdl_container_create calculated width:%u height:%u", window_width, window_height));

    window_x = sdl_initial_xpos[canvas_idx];
    window_y = sdl_initial_ypos[canvas_idx];

    /* Obtain the Window with the corresponding size and behavior based on the flags */
    if ((sdl_initial_width[canvas_idx] != 0) || (sdl_initial_height[canvas_idx] != 0)) {
        window_width = sdl_initial_width[canvas_idx];
        window_height = sdl_initial_height[canvas_idx];
    }

    DBG(("sdl_container_create width:%u height:%u x:%d y:%d",
           window_width, window_height, window_x, window_y));

    container->window = SDL_CreateWindow("",
                                         window_x, window_y,
                                         window_width, window_height,
                                         flags);
    if (container->window == NULL) {
        sdl_container_destroy(container);
        log_error(sdlvideo_log, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_SetWindowData(container->window, VIDEO_SDL2_CANVAS_INDEX_KEY, (void*)(canvas));
    sdl_ui_set_window_icon(container->window);

    container->last_width = window_width;
    container->last_height = window_height;

    /* Allocate renderlist strings */
    renderlist = lib_malloc((renderamount + 1) * sizeof(char *));

    /* Fill in the renderlist and render info string */
    for (it = 0; it < renderamount; ++it) {
        SDL_GetRenderDriverInfo(it, &info);

        strcat(rendername, info.name);
        strcat(rendername, " ");
        renderlist[it] = lib_strdup(info.name);
    }
    renderlist[it] = NULL;

    /* Check for resource preferred renderer */
    drv_index = -1;
    if (sdl2_renderer_name != NULL && *sdl2_renderer_name != '\0') {
        for (it = 0; it < renderamount; ++it) {
            if (!strcmp(sdl2_renderer_name, renderlist[it])) {
                drv_index = it;
            }
        }
        if (drv_index == -1) {
            log_warning(sdlvideo_log, "Resource preferred backend %s not available, trying arch default backend(s)", sdl2_renderer_name);
        }
    }

    for (l = 0; l < renderamount; ++l) {
        lib_free(renderlist[l]);
    }
    lib_free(renderlist);
    renderlist = NULL;


    log_message(sdlvideo_log, "Available backends: %s", rendername);

    /* setup vsync */
    /* FIXME: this only works by setting the hint and then creating the
       renderer - so to do this at runtime some magic has to be implemented
       that destroys current renderer(s), changes the hint, and then creates
       them again */
    vsync = canvas->videoconfig->vsync;
    log_message(sdlvideo_log, "VSync is %s.", vsync ? "enabled" : "disabled");
    if (vsync) {
        SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
        container->renderer = SDL_CreateRenderer(container->window,
                                                drv_index,
                                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    } else {
        SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "0", SDL_HINT_OVERRIDE);
        container->renderer = SDL_CreateRenderer(container->window,
                                                drv_index,
                                                SDL_RENDERER_ACCELERATED);
    }

    if (!container->renderer && drv_index == -1) {
        log_warning(sdlvideo_log, "SDL_CreateRenderer() failed. Falling back to basic renderer: %s", SDL_GetError());
        SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "0", SDL_HINT_OVERRIDE);
        container->renderer = SDL_CreateRenderer(container->window,
                                                drv_index,
                                                0);
    }

    if (!container->renderer) {
        log_error(sdlvideo_log, "SDL_CreateRenderer() failed: %s", SDL_GetError());
        sdl_container_destroy(container);
        return NULL;
    }

    SDL_GetRendererInfo(container->renderer, &info);
    log_message(sdlvideo_log, "SDL2 backend driver selected: %s", info.name);
    SDL_SetRenderDrawColor(container->renderer, 0, 0, 0, 255);
    SDL_RenderClear(container->renderer);
    SDL_RenderPresent(container->renderer);

    /* Enable file/text drag and drop support */
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    SDL_SetWindowPosition(container->window, window_x, window_y);
    SDL_SetWindowSize(container->window, window_width, window_height);

    /* Explicitly minimize if the window was created minimized */
    if ((flags & SDL_WINDOW_MINIMIZED) != 0) {
        SDL_MinimizeWindow(container->window);
    }

    return container;
}

/** \brief Predicate function to determine if a canvas is visible to the user.
 *
 * \returns 1 if the canvas is currently visible to the user, or 0 if not.
 */
static int sdl_canvas_is_visible(struct video_canvas_s *canvas)
{
    if (canvas == sdl_active_canvas) {
        return 1;
    }

    int other_canvas_idx = canvas->index ^ 1;
    video_canvas_t* other_canvas = sdl_canvaslist[other_canvas_idx];

    if (canvas->container != other_canvas->container) {
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Main API */

/* called from raster/raster.c:realize_canvas */
video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
    /* nothing to do here, the real work is done in sdl_ui_init_finalize */
    return canvas;
}

void video_canvas_refresh(struct video_canvas_s *canvas,
                          unsigned int xs, unsigned int ys,
                          unsigned int xi, unsigned int yi,
                          unsigned int w, unsigned int h)
{
    SDL_Texture *texture_swap;
    uint8_t *backup;
    SDL_RendererFlip flip = 0;
    double angle = 0;

    /* If the canvas isn't initialized, skip this */
    if ((canvas == NULL) || (canvas->screen == NULL)) {
        return;
    }

    if (sdl_canvas_is_visible(canvas) == 0) {
        return;
    }

    if (sdl_vsid_state & SDL_VSID_ACTIVE) {
        sdl_vsid_draw();
    }

    if (sdl_vkbd_state & SDL_VKBD_ACTIVE) {
        sdl_vkbd_draw();
    }

    if (uistatusbar_state & (UISTATUSBAR_ACTIVE|UISTATUSBAR_ACTIVE_VDC)) {
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

    if (recreate_textures) {
        recreate_all_textures();
        recreate_textures = 0;
        /* NOTE: The texture isn't holding the screen's values
         *       here. We can get away with that because the call to
         *       SDL_UpdateTexture below updates the entire canvas */
    }

    /* Upload the new frame to the GPU texture. TODO: use SDL_LockTexture for this as the docs day it's faster. */
    SDL_UpdateTexture(canvas->texture, NULL, canvas->screen->pixels, canvas->screen->pitch);

    /* Render. */
    SDL_RenderClear(canvas->container->renderer);

    if (canvas->videoconfig->flipx) {
        flip |= SDL_FLIP_HORIZONTAL;
    }
    if (canvas->videoconfig->flipy) {
        flip |= SDL_FLIP_VERTICAL;
    }

    angle = canvas->videoconfig->rotate ? 90.0f : 0.0f;

    if (canvas->videoconfig->interlaced && !sdl_menu_state) {
        /*
         * Interlaced mode: Re-render last frame to render new frame over.
         * We don't do this if the SDL menu is showing, otherwise the first
         * render of the menu shows the emu screen behind it!
         */
        SDL_SetTextureBlendMode(canvas->previous_frame_texture, SDL_BLENDMODE_NONE);
        SDL_RenderCopyEx(canvas->container->renderer, canvas->previous_frame_texture, NULL, NULL, angle, NULL, flip);
        SDL_SetTextureBlendMode(canvas->texture, SDL_BLENDMODE_BLEND);
    } else {
        SDL_SetTextureBlendMode(canvas->texture, SDL_BLENDMODE_NONE);
    }

    if (canvas->videoconfig->rotate) {
        /* FIXME: when the output is rotated 90degrees, the texture must be scaled accordingly.
                  somehow this doesnt work without doing fancy magic like this... */
        int tw;
        int th;
        int curr_w;
        int curr_h;
        float scale;
        SDL_Rect rect = {0, 0, 0, 0};

        SDL_QueryTexture(canvas->texture, NULL, NULL, &tw, &th);
        SDL_GetWindowSize(canvas->container->window, &curr_w, &curr_h);

        scale = (double)curr_h / (double)tw;
        /* scale = (double)th / (double)curr_w; */
        /* scale /= 2.0f; */

        rect.x = (tw - th) / 2 - 1;
        rect.y = (th - (tw * scale)) / 2;
        rect.w = th;
        rect.h = tw * scale;

        DBG(("video_canvas_refresh angle:%f scale:%f", angle, scale));
        SDL_RenderCopyEx(canvas->container->renderer, canvas->texture, NULL, &rect, angle, NULL, flip);
    } else {
        SDL_RenderCopyEx(canvas->container->renderer, canvas->texture, NULL, NULL, angle, NULL, flip);
    }

    SDL_RenderPresent(canvas->container->renderer);

    /* Swap the textures references so we can easily re-render this frame under the next frame. */
    texture_swap = canvas->previous_frame_texture;
    canvas->previous_frame_texture = canvas->texture;
    canvas->texture = texture_swap;

    if (canvas->container->leaving_fullscreen) {
        int curr_w, curr_h, flags;
        int last_width = canvas->container->last_width;
        int last_height = canvas->container->last_height;

        SDL_GetWindowSize(canvas->container->window, &curr_w, &curr_h);
        flags = SDL_GetWindowFlags(canvas->container->window);
        canvas->container->leaving_fullscreen = 0;

        if ((curr_w != last_width || curr_h != last_height) &&
            (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP |
                      SDL_WINDOW_MAXIMIZED)) == 0) {
            log_message(sdlvideo_log, "Resolution anomaly leaving fullscreen: expected %dx%d, got %dx%d", last_width, last_height, curr_w, curr_h);
            SDL_SetWindowSize(canvas->container->window, last_width, last_height);
        }
    }

    ui_autohide_mouse_cursor();
}

int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette)
{
    unsigned int i, col = 0;
    video_render_color_tables_t *color_tables = &canvas->videoconfig->color_tables;
    SDL_PixelFormat *fmt;

    DBG(("video_canvas_set_palette canvas: %p (index:%d)", canvas, canvas->index));

    if (palette == NULL) {
        return 0; /* no palette, nothing to do */
    }

    canvas->palette = palette;

    if (canvas->screen == NULL) {
        log_error(LOG_DEFAULT, "FIXME: video_canvas_set_palette() canvas->screen is NULL");
        return 0;
    }
    fmt = canvas->screen->format;

    /* FIXME: needs further investigation how it can reach here without being fully initialized */
    /* NOTE: check removed in r44823 - with the check x128 can not properly set
             the palette for the VDC window. The original reason for having it
             doesn't seem to exist anymore either (see bug #788) */
#if 0
    if (canvas != sdl_active_canvas) {
        DBG(("video_canvas_set_palette: not active canvas, don't update hw palette"));
        return 0;
    }
    if (canvas->width != canvas->screen->w) {
        DBG(("video_canvas_set_palette: window not created, don't update hw palette"));
        return 0;
    }
#endif

    for (i = 0; i < palette->num_entries; i++) {
        if (canvas->depth % 8 == 0) {
            col = SDL_MapRGB(fmt, palette->entries[i].red, palette->entries[i].green, palette->entries[i].blue);
        }
        video_render_setphysicalcolor(canvas->videoconfig, i, col, canvas->depth);
    }

    if (canvas->depth % 8 == 0) {
        for (i = 0; i < 256; i++) {
            video_render_setrawrgb(color_tables, i, SDL_MapRGB(fmt, (Uint8)i, 0, 0), SDL_MapRGB(fmt, 0, (Uint8)i, 0), SDL_MapRGB(fmt, 0, 0, (Uint8)i));
        }
        video_render_initraw(canvas->videoconfig);
    }

    return 0;
}

static void sdl_correct_logical_size(void)
{
    for (int i = 0; i < sdl_num_screens; ++i) {
        video_canvas_t* canvas = sdl_canvaslist[i];
        video_container_t* container = canvas->container;

        if (container && canvas->texture) {
            int corrected_width, corrected_height;

            if (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_NONE) {
                SDL_GetWindowSize(container->window, &corrected_width, &corrected_height);
            } else {
                double aspect = (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_CUSTOM) ? canvas->videoconfig->aspect_ratio : canvas->geometry->pixel_aspect_ratio;
                corrected_width = canvas->width * aspect;
                corrected_height = canvas->height;
            }
            DBG(("sdl_correct_logical_size w:%d h:%d", corrected_width, corrected_height));
            SDL_RenderSetLogicalSize(container->renderer, corrected_width, corrected_height);
        }
    }
}

static void sdl_correct_logical_and_minimum_size(void)
{
    for (int i = 0; i < sdl_num_screens; ++i) {
        video_canvas_t* canvas = sdl_canvaslist[i];
        video_container_t* container = canvas->container;

        if (container && container->window && container->renderer && canvas->texture) {
            if (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_NONE) {
                SDL_SetWindowMinimumSize(container->window, canvas->width, canvas->height);
                sdl_correct_logical_size();
            } else {
                int width, height;
                sdl_correct_logical_size();
                SDL_RenderGetLogicalSize(container->renderer, &width, &height);
                DBG(("sdl_correct_logical_and_minimum_size w:%d h:%d", width, height));
                SDL_SetWindowMinimumSize(container->window, width, height);
            }
        }
    }
}

/** \brief Given a canvas, resizes the associated window to match and allocates textures
 *         for rendering the canvas to the container.
 *
 * This function is called from called from video_viewport_resize, and is
 * responsible for setting the fullscreen behavior of a window. Despite its
 * name, this does resize the texture.
 */
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
    unsigned int width, height;
    if (!(canvas && canvas->container && canvas->draw_buffer && canvas->videoconfig && canvas->fullscreenconfig)) {
        return;
    }

    width = canvas->draw_buffer->canvas_width * canvas->videoconfig->scalex;
    height = canvas->draw_buffer->canvas_height * canvas->videoconfig->scaley;

    DBG(("%s: %ux%u (%i)", __func__, width, height, canvas->index));

    /* Update the fullscreen status, if any */
    if (canvas->container) {
        if (canvas == sdl_active_canvas) {
            if (canvas->fullscreenconfig->enable) {
                if (canvas->fullscreenconfig->mode == FULLSCREEN_MODE_CUSTOM) {
                    SDL_SetWindowSize(canvas->container->window,
                                      canvas->videoconfig->fullscreen_custom_width,
                                      canvas->videoconfig->fullscreen_custom_height);
                    SDL_SetWindowFullscreen(canvas->container->window, SDL_WINDOW_FULLSCREEN);
                } else {
                    SDL_SetWindowFullscreen(canvas->container->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
            } else {
                int flags = SDL_GetWindowFlags(canvas->container->window);
                if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
                    SDL_SetWindowFullscreen(canvas->container->window, 0);
                    canvas->container->leaving_fullscreen = 1;
                }
            }
        }
    }

    /* Ignore bad values, or values that don't change anything */
    if (width == 0 || height == 0 ||
        (canvas->texture && width == canvas->width && height == canvas->height)) {
        return;
    }

    canvas->depth = sdl_bitdepth;
    canvas->width = canvas->actual_width = width;
    canvas->height = canvas->actual_height = height;

    if (canvas->container->renderer) {
        SDL_Surface *new_screen;

        new_screen = SDL_CreateRGBSurface(0, width, height, sdl_bitdepth, rmask, gmask, bmask, amask);
        if (!new_screen) {
            log_error(sdlvideo_log, "SDL_CreateRGBSurface() failed: %s\n", SDL_GetError());
            return;
        }
        if (canvas->screen) {
            SDL_FreeSurface(canvas->screen);
        }
        canvas->screen = new_screen;

        recreate_canvas_textures(canvas);

        log_message(sdlvideo_log, "%s (%s) %ux%u %ibpp %s", canvas->videoconfig->chip_name, (canvas == sdl_active_canvas) ? "active" : "inactive", width, height, sdl_bitdepth, (canvas->fullscreenconfig->enable) ? " (fullscreen)" : "");
#ifdef SDL_DEBUG
        log_message(sdlvideo_log, "Canvas %ux%u, real %ux%u", width, height, canvas->real_width, canvas->real_height);
#endif

        video_canvas_set_palette(canvas, canvas->palette);
    }
    sdl_correct_logical_and_minimum_size();
}

/* Resize window to w/h. */
void sdl2_video_resize_event(int canvas_idx, unsigned int w, unsigned int h)
{
    video_container_t* container = sdl_canvaslist[canvas_idx]->container;
    SDL_Window* window = container->window;
    int flags = SDL_GetWindowFlags(window);

    if ((flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP |
                  SDL_WINDOW_MAXIMIZED)) == 0) {
        /* We aren't in some fullscreen-or-close-to-it mode, and so this is
         * a "legitimate" resize. Record that size for comparison against
         * what we see when we leave fullscreen. */
        container->last_width = w;
        container->last_height = h;

        resources_set_int_sprintf("Window%dWidth", w, canvas_idx);
        resources_set_int_sprintf("Window%dHeight", h, canvas_idx);
    }

    sdl_correct_logical_size();
}

/* Resize window to stored real size */
void sdl_video_restore_size(void)
{
    for (int i=0; i<sdl_num_screens; i++) {
        video_container_t* container = sdl_canvaslist[i]->container;
        int w, h;

        if (container->renderer) {
            SDL_RenderGetLogicalSize(container->renderer, &w, &h);
            SDL_SetWindowSize(container->window, w, h);
        }
    }
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

    sdl_active_canvas_num = index;

    canvas = sdl_canvaslist[sdl_active_canvas_num];
    sdl_active_canvas = canvas;

    if (sdl_active_canvas->container) {
        SDL_SetWindowData(sdl_active_canvas->container->window,
                          VIDEO_SDL2_CANVAS_INDEX_KEY,
                          (void*)(sdl_active_canvas));
    }

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
    canvas->real_width = 0;
    canvas->real_height = 0;

#ifdef USE_SDL2UI
    canvas->container = NULL;
#endif

    /*
     * the render output can always be read from in SDL2,
     * it's not a direct video memory buffer.
     */
    canvas->videoconfig->readable = 1;
}

void video_canvas_destroy(struct video_canvas_s *canvas)
{
    int i;

    DBG(("%s: (%p, %i)", __func__, canvas, canvas->index));

    for (i = 0; i < sdl_num_screens; ++i) {
        if (sdl_canvaslist[i] == canvas) {
#ifdef USE_SDL2UI
            /* If the second window isn't visible, then both canvas lists should
               be sharing the same container. Set the other one to NULL
               directly so we don't accidentally double free. */
            if (sdl_canvaslist[i ^ 1]) {
                if (sdl_canvaslist[i]->container == sdl_canvaslist[i ^ 1]->container) {
                    sdl_canvaslist[i ^ 1]->container = NULL;
                }
            }
            sdl_container_destroy(sdl_canvaslist[i]->container);
            sdl_canvaslist[i]->container = NULL;
#endif

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

/** \brief  Hides the secondary window.
 *
 * Internally this just destroys the window and its textures.
 */
void sdl2_hide_second_window(void)
{
    int inactive_canvas_idx = sdl_active_canvas->index ^ 1;
    video_canvas_t* inactive_canvas = sdl_canvaslist[inactive_canvas_idx];
    video_container_t* inactive_container = inactive_canvas->container;
    video_container_t* active_container = sdl_active_canvas->container;

    if (active_container != inactive_container) {
        DBG(("%s active: %d, inactive: %d", __func__,
             sdl_active_canvas->index, inactive_canvas_idx));

        inactive_canvas->container = active_container;

        SDL_DestroyTexture(inactive_canvas->texture);
        inactive_canvas->texture = NULL;
        SDL_DestroyTexture(inactive_canvas->previous_frame_texture);
        inactive_canvas->previous_frame_texture = NULL;

        sdl_container_destroy(inactive_container);

        /* Force a recretion of the textures since we have effectively changed
           our renderer, and SDL textures can't be shared between renderers. */
        recreate_all_textures();

        sdl_ui_refresh();
    }
}

video_canvas_t *sdl2_get_canvas_from_index(int index)
{
    return sdl_canvaslist[index];
}

/** \brief  Shows the secondary window.
 *
 * Internally, this creates a new window by calling `sdl_container_create`.
 */
void sdl2_show_second_window(void)
{
    int inactive_canvas_idx = sdl_active_canvas->index ^ 1;
    video_canvas_t* inactive_canvas = sdl_canvaslist[inactive_canvas_idx];
    video_container_t* inactive_container = inactive_canvas->container;
    video_container_t* active_container = sdl_active_canvas->container;

    if (active_container == inactive_container) {
        video_container_t* new_container = sdl_container_create(inactive_canvas_idx);

        DBG(("%s active: %d, inactive: %d", __func__,
             sdl_active_canvas->index, inactive_canvas_idx));

        inactive_canvas->container = new_container;

        /* Force a recretion of the textures since we have effectively changed
           our renderer, and SDL textures can't be shared between renderers. */
        recreate_all_textures();
        sdl_ui_refresh();

        SDL_RaiseWindow(active_container->window);
    }
}

void sdl_ui_init_finalize(void)
{
    int minimized = 0;
    int dual_windows = 0;
    int hide_vdc = 0;
    video_container_t* container = NULL;

    resources_get_int("DualWindow", &dual_windows);
    if (machine_class == VICE_MACHINE_C128) {
        resources_get_int("C128HideVDC", &hide_vdc);
    }
    resources_get_int("StartMinimized", &minimized);

    /* Setup the primary window using the active canvas */
    container = sdl_container_create(sdl_active_canvas->index);

    if (dual_windows) {
            video_canvas_t* canvas = sdl_canvaslist[VIDEO_CANVAS_IDX_VICII];
            canvas->container = container;
            video_canvas_resize(sdl_canvaslist[VIDEO_CANVAS_IDX_VICII], 1);
            SDL_SetWindowPosition(container->window, sdl_initial_xpos[VIDEO_CANVAS_IDX_VICII], sdl_initial_ypos[VIDEO_CANVAS_IDX_VICII]);
            SDL_SetWindowSize(container->window, sdl_initial_width[VIDEO_CANVAS_IDX_VICII], sdl_initial_height[VIDEO_CANVAS_IDX_VICII]);
    } else {
        for (int i = 0; i < sdl_num_screens; i++) {
            video_canvas_t* canvas = sdl_canvaslist[i];
            canvas->container = container;
            video_canvas_resize(sdl_canvaslist[i], 1);
            /* restore the saved window position */
            SDL_SetWindowPosition(container->window, sdl_initial_xpos[i], sdl_initial_ypos[i]);
            SDL_SetWindowSize(container->window, sdl_initial_width[i], sdl_initial_height[i]);
        }
    }

    /* If we're setup for dual windows, then we need to allocate a new container
     * for the VDC. We do that here, but only associate the new window with the
     * VDC canvas.
     */
    if (dual_windows && !hide_vdc) {
        video_canvas_t* vdc_canvas = sdl_canvaslist[VIDEO_CANVAS_IDX_VDC];

        container = sdl_container_create(VIDEO_CANVAS_IDX_VDC);
        if (!container) {
            fprintf(stderr, "error: unable to create canvas container\n");
            archdep_vice_exit(-1);
        }

        vdc_canvas->container = container;
        video_canvas_resize(vdc_canvas, 1);
        /* restore the saved window position */
        SDL_SetWindowPosition(container->window, sdl_initial_xpos[VIDEO_CANVAS_IDX_VDC], sdl_initial_ypos[VIDEO_CANVAS_IDX_VDC]);
        SDL_SetWindowSize(container->window, sdl_initial_width[VIDEO_CANVAS_IDX_VDC], sdl_initial_height[VIDEO_CANVAS_IDX_VDC]);

        /* Explicitly raise the VIC-II window in dual head mode -- creating the
         * windows in reverse order still results in the VDC window being on top
         * because SDL does not raise windows on creation.
         */
        if (!minimized) {
            video_canvas_t* vic_canvas = sdl_canvaslist[VIDEO_CANVAS_IDX_VICII];
            container = vic_canvas->container;
            SDL_RaiseWindow(container->window);
        }
    }

    mousedrv_mouse_changed();
}

static int last_mouse_x = -1;
static int last_mouse_y = -1;

int sdl_ui_get_mouse_state(int *px, int *py, unsigned int *pbuttons)
{
    SDL_Window* window = sdl_active_canvas->container->window;
    SDL_Renderer* renderer = sdl_active_canvas->container->renderer;
    int x, y, w, h;
    Uint32 buttons;
    double ratio;

    if (!window || !renderer || !sdl_active_canvas) {
        /* Not initialized yet */
        return 0;
    }

    if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS)) {
        /* We don't have mouse focus */
        return 0;
    }

    buttons = SDL_GetMouseState(&x, &y);
    SDL_RenderGetLogicalSize(renderer, &w, &h);
    x = last_mouse_x;
    y = last_mouse_y;
    ratio = (double) w / (double)sdl_active_canvas->width;
    if (x < 0 || x > w || y < 0 || y > h) {
        return 0;
    }
    if (px) {
        *px = x / (ratio * sdl_active_canvas->videoconfig->scalex);
    }
    if (py) {
        *py = y / sdl_active_canvas->videoconfig->scaley;
    }
    if (pbuttons) {
        *pbuttons = buttons;
    }
    return 1;
}

void sdl_ui_consume_mouse_event(SDL_Event *event)
{
    if (event && event->type == SDL_MOUSEMOTION) {
        last_mouse_x = event->motion.x;
        last_mouse_y = event->motion.y;
    }
    ui_autohide_mouse_cursor();
}

void sdl_ui_set_window_title(char *title)
{
    if (sdl_active_canvas->container) {
        SDL_SetWindowTitle(sdl_active_canvas->container->window, title);
    }
}
