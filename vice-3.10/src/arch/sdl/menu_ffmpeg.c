/*
 * menu_ffmpeg.c - FFMPEG menu for SDL UI.
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

/* #define SDL_DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "types.h"

#include "ffmpegexedrv.h"
#include "gfxoutput.h"
#include "lib.h"
#include "menu_common.h"
#include "menu_ffmpeg.h"
#include "resources.h"
#include "screenshot.h"
#include "ui.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "util.h"
#include "videoarch.h"

static gfxoutputdrv_t *ffmpeg_drv = NULL;

/* FIXME: these should be dynamic too! */
#define MAX_FORMATS 15
#define MAX_CODECS  15

static ui_menu_entry_t format_menu[MAX_FORMATS + 1];
static ui_menu_entry_t video_codec_menu[MAX_CODECS + 1];
static ui_menu_entry_t audio_codec_menu[MAX_CODECS + 1];

static UI_MENU_CALLBACK(radio_FFMPEGAudioCodec_callback);
static UI_MENU_CALLBACK(radio_FFMPEGVideoCodec_callback);

/* FIXME: the sliders will also have to use dynamic callbacks once there is more
          than one gfxoutputdrv that wants to use them */
UI_MENU_DEFINE_SLIDER(FFMPEGVideoBitrate, VICE_FFMPEG_VIDEO_RATE_MIN, VICE_FFMPEG_VIDEO_RATE_MIN)
UI_MENU_DEFINE_SLIDER(FFMPEGAudioBitrate, VICE_FFMPEG_AUDIO_RATE_MIN, VICE_FFMPEG_AUDIO_RATE_MAX)

static const char *video_driver = NULL;
static gfxoutputdrv_format_t *videodriver_formatlist = NULL;

static UI_MENU_CALLBACK(custom_FFMPEGFormat_callback);

static void update_format_menu(const char *current_format);
static void update_codec_menus(const char *current_format);

/* activate this driver */
void sdl_menu_ffmpeg_set_driver(const char *videodriver)
{
    const char *w;
    video_driver = videodriver;

    ffmpeg_drv = gfxoutput_get_driver(videodriver);
    if (ffmpeg_drv == NULL) {
        ui_error("Driver '%s' not available.", videodriver);
        return;
    }

    videodriver_formatlist = ffmpeg_drv->formatlist;

    resources_get_string_sprintf("%sFormat", &w, videodriver);
    update_format_menu(w);
    update_codec_menus(w);
}

static const char* get_video_driver(void)
{
    if (video_driver) {
        return video_driver;
    }
    /* if none set, use a default */
    return "ZMBV";
}

static UI_MENU_CALLBACK(save_movie_callback)
{
    char *name = NULL;
    int width;
    int height;

    if (activated) {
        const char *videodriver = get_video_driver();
        if (videodriver == NULL) {
            ui_error("No video driver available.");
            return NULL;
        }

        ffmpeg_drv = gfxoutput_get_driver(videodriver);
        if (ffmpeg_drv == NULL) {
            ui_error("Driver '%s' not available.", videodriver);
            return NULL;
        }

        videodriver_formatlist = ffmpeg_drv->formatlist;

        if (videodriver_formatlist == NULL || videodriver_formatlist[0].name == NULL) {
            ui_error("FFMPEG not available.");
            return NULL;
        }

        name = sdl_ui_file_selection_dialog("Choose movie file", FILEREQ_MODE_SAVE_FILE);
        if (name != NULL) {
            width = sdl_active_canvas->draw_buffer->draw_buffer_width;
            height = sdl_active_canvas->draw_buffer->draw_buffer_height;
            memcpy(sdl_active_canvas->draw_buffer->draw_buffer, sdl_ui_get_draw_buffer(), width * height);
            util_add_extension(&name, ffmpeg_drv->default_extension);
            if (screenshot_save(videodriver, name, sdl_active_canvas) < 0) {
                ui_error("Cannot save movie.");
            }
            lib_free(name);
            return sdl_menu_text_exit_ui;
        }
    }
    return NULL;
}

#define OFFS_VIDEO_BITRATE  3
#define OFFS_AUDIO_BITRATE  4

ui_menu_entry_t ffmpeg_menu[] = {
    {   .string   = "Format",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)format_menu
    },
    {   .string   =  "Video codec",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)video_codec_menu
    },
    {   .string   = "Audio codec",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)audio_codec_menu
    },
    {   .string   = "Video bitrate",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = slider_FFMPEGVideoBitrate_callback,
        .data     = (ui_callback_data_t)"Set video bitrate"
    },
    {   .string   = "Audio bitrate",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = slider_FFMPEGAudioBitrate_callback,
        .data     = (ui_callback_data_t)"Set audio bitrate"
    },
    {   .string   = "Save movie",
        .type     = MENU_ENTRY_DIALOG,
        .callback = save_movie_callback
    },
    SDL_MENU_LIST_END
};

/** \brief  Regenerate the format submenu
 *
 * \param[in]   current_format  video driver name (value of "XXXFormat" resource)
 *
 * TODO:    Update code to use `videodriver_formatlist`
 */
static void update_format_menu(const char *current_format)
{
    int i;
    int k;
    k = 0;

    for (i = 0; videodriver_formatlist[i].name != NULL && i < MAX_FORMATS; i++) {
        char *name = videodriver_formatlist[i].name;

        if (videodriver_formatlist[i].audio_codecs != NULL &&
                videodriver_formatlist[i].video_codecs != NULL) {
            format_menu[k].action   = ACTION_NONE;
            format_menu[k].string   = name;
            format_menu[k].type     = MENU_ENTRY_RESOURCE_RADIO;
            format_menu[k].callback = custom_FFMPEGFormat_callback;
            format_menu[k].data     = (ui_callback_data_t)(name);
#ifdef SDL_DEBUG
            fprintf(stderr, "%s: format %i: %s selected\n",
                    __func__, i, name ? name : "(NULL)");
#endif
            k++;
#ifdef SDL_DEBUG
        } else {
            fprintf(stderr, "%s: format %i: %s not selected\n",
                    __func__, i, name ? name : "(NULL)");
#endif
        }
    }
    if (k == MAX_FORMATS) {
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: FIXME format %i > %i (MAX)\n", __func__, i, MAX_FORMATS);
#endif
    }

    format_menu[k].string = NULL;
}

/** \brief  Regenerate the audio/video codec submenus
 *
 * \param[in]   current_format  video driver name (value of "XXXFormat" resource)
 *
 * TODO:    Update code to use `videodriver_formatlist`
 */
static void update_codec_menus(const char *current_format)
{
    int i;
    gfxoutputdrv_format_t *format;
    gfxoutputdrv_codec_t *codec;

    int video_codec_id;
    int audio_codec_id;

    int codec_found;
    const char *videodriver = get_video_driver();

    video_codec_menu[0].string = NULL;
    audio_codec_menu[0].string = NULL;

    if (videodriver_formatlist == NULL || videodriver_formatlist[0].name == NULL) {
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: no driver found\n", __func__);
#endif
        return;
    }

#if 0
    /* Find currently selected format */
    format = ffmpeg_drv->formatlist;

    while (format) {
        if (!strcmp(format->name, current_format)) {
            break;
        } else {
            format++;
        }
    }
#else
    format = NULL;
    for (i = 0; videodriver_formatlist[i].name != NULL; i++) {
        if (strcmp(videodriver_formatlist[i].name, current_format) == 0) {
            /* got current format */
            format = &(videodriver_formatlist[i]);
            break;
        }
    }
#endif

    if (!format) {
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: format %s not found\n", __func__, current_format ? current_format : "(NULL)");
#endif
        return;
    }

    ffmpeg_menu[OFFS_AUDIO_BITRATE].status =
        (format->flags & GFXOUTPUTDRV_HAS_AUDIO_BITRATE) ? MENU_STATUS_ACTIVE : MENU_STATUS_NA;
    ffmpeg_menu[OFFS_VIDEO_BITRATE].status =
        (format->flags & GFXOUTPUTDRV_HAS_VIDEO_BITRATE) ? MENU_STATUS_ACTIVE : MENU_STATUS_NA;

    /* Update video codec menu */
    codec = format->video_codecs;
    i = 0;

    if (codec == NULL) {
        video_codec_menu[0].string = NULL;
    } else {

        /* get the currently used video codec */
        resources_get_int_sprintf("%sVideoCodec", &video_codec_id, videodriver);

        codec_found = 0;
        while (codec && codec->name) {
            video_codec_menu[i].action   = ACTION_NONE;
            /* FIXME: don't cast away const! */
            video_codec_menu[i].string   = (char *)(codec->name);
            video_codec_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
            video_codec_menu[i].callback = radio_FFMPEGVideoCodec_callback;
            video_codec_menu[i].data     = vice_int_to_ptr(codec->id);
#ifdef SDL_DEBUG
            fprintf(stderr, "%s: video codec %i: %s (%i)\n", __func__, i, (codec->name) ? codec->name : "(NULL)", codec->id);
#endif
            if (codec-> id == video_codec_id) {
                /* old video codec is present in the new codecs */
                codec_found = 1;
            }

            codec++;
            i++;

            if (i == MAX_CODECS) {
#ifdef SDL_DEBUG
                fprintf(stderr, "%s: FIXME video codec %i > %i (MAX)\n", __func__, i, MAX_CODECS);
#endif
                break;
            }
        }
        video_codec_menu[i].string = NULL;

        /* is the old codec still valid for the new driver? */
        if (!codec_found) {
            /* no: default to the first codec in the new submenu */
            resources_set_int_sprintf("%sVideoCodec", format->video_codecs[0].id, videodriver);
        }
    }


    /* Update audio codec menu */
    codec = format->audio_codecs;
    i = 0;

    if (codec == NULL) {
        audio_codec_menu[0].string = NULL;
    } else {

        /* get the currently selected audio codec */
        resources_get_int_sprintf("%sAudioCodec", &audio_codec_id, videodriver);
        codec_found = 0;
        while (codec && codec->name) {
            audio_codec_menu[i].action   = ACTION_NONE;
            /* FIXME: don't cast away const! */
            audio_codec_menu[i].string   = (char *)(codec->name);
            audio_codec_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
            audio_codec_menu[i].callback = radio_FFMPEGAudioCodec_callback;
            audio_codec_menu[i].data     = vice_int_to_ptr(codec->id);
#ifdef SDL_DEBUG
            fprintf(stderr, "%s: audio codec %i: %s (%i)\n", __func__, i, (codec->name) ? codec->name : "(NULL)", codec->id);
#endif

            if (audio_codec_id == codec->id) {
                /*old audio codec is present in the new codecs */
                codec_found = 1;
            }

            codec++;
            i++;

            if (i == MAX_CODECS) {
#ifdef SDL_DEBUG
                fprintf(stderr, "%s: FIXME audio codec %i > %i (MAX)\n", __func__, i, MAX_CODECS);
#endif
                break;
            }
        }
        audio_codec_menu[i].string = NULL;

        /* is the old codec still valid for the new driver? */
        if (!codec_found) {
            /* no: default to the first codec in the new submenu */
            resources_set_int_sprintf("%sAudioCodec", format->audio_codecs[0].id, videodriver);
        }
    }

}

/* callback for "XXXFormat" (toplevel) */
static UI_MENU_CALLBACK(custom_FFMPEGFormat_callback)
{
    const char *videodriver = get_video_driver();

    if (videodriver == NULL) {
        ui_error("No video driver available.");
        return NULL;
    }
    if (activated) {
        resources_set_string_sprintf("%sFormat", (char *)param, videodriver);
        update_codec_menus((const char *)param);
    } else {
        const char *w;

        resources_get_string_sprintf("%sFormat", &w, videodriver);
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: %sFormat = '%s'\n", __func__, videodriver, w);
#endif
        if (!strcmp(w, (char *)param)) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(radio_FFMPEGAudioCodec_callback)
{
    const char *videodriver = get_video_driver();

    if (videodriver == NULL) {
        ui_error("No video driver available.");
        return NULL;
    }
    if (activated) {
        const char *w;
        resources_set_int_sprintf("%sAudioCodec", vice_ptr_to_int(param), videodriver);
        resources_get_string_sprintf("%sFormat", &w, videodriver);
        update_codec_menus(w);
    } else {
        int w = 0;

        resources_get_int_sprintf("%sAudioCodec", &w, videodriver);
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: %sFormat = '%d'\n", __func__, videodriver, w);
#endif
        if (w == vice_ptr_to_int(param)) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(radio_FFMPEGVideoCodec_callback)
{
    const char *videodriver = get_video_driver();

    if (videodriver == NULL) {
        ui_error("No video driver available.");
        return NULL;
    }
    if (activated) {
        const char *w;
        resources_set_int_sprintf("%sVideoCodec", vice_ptr_to_int(param), videodriver);
        resources_get_string_sprintf("%sFormat", &w, videodriver);
        update_codec_menus(w);
    } else {
        int w = 0;

        resources_get_int_sprintf("%sVideoCodec", &w, videodriver);
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: %sFormat = '%d'\n", __func__, videodriver, w);
#endif
        if (w == vice_ptr_to_int(param)) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

void sdl_menu_ffmpeg_init(void)
{
    const char *w;
    const char *videodriver = get_video_driver();

    if (videodriver == NULL) {
        ui_error("No video driver available.");
        return;
    }

    ffmpeg_drv = gfxoutput_get_driver(videodriver);
    if (ffmpeg_drv == NULL) {
        ui_error("Driver '%s' not available.", videodriver);
        return;
    }

    videodriver_formatlist = ffmpeg_drv->formatlist;

    if (videodriver_formatlist == NULL || videodriver_formatlist[0].name == NULL) {
        ui_error("FFMPEG not available.");
#ifdef SDL_DEBUG
        fprintf(stderr, "%s: no driver found\n", __func__);
#endif
        return;
    }

    resources_get_string_sprintf("%sFormat", &w, videodriver);
    update_format_menu(w);
    update_codec_menus(w);
}

void sdl_menu_ffmpeg_shutdown(void)
{
    if (screenshot_is_recording()) {
        screenshot_stop_recording();
    }
}
