/** \file   zmbvdrv.c
 * \brief   zmbv movie driver using screenshot API
 *
 * \author  groepaz <groepaz@gmx.net>
 */

/*
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

/* #define DEBUG_ZMBV */
/* #define DEBUG_ZMBV_FRAMES */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "gfxoutput.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "math.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "uiapi.h"
#include "util.h"
#include "soundmovie.h"
#include "zmbvdrv.h"

#include "zmbv.h"
#include "zmbv_avi.h"

#ifdef DEBUG_ZMBV
#define LOG(x) log_printf    x
#else
#define LOG(x)
#endif

#ifdef DEBUG_ZMBV_FRAMES
#define LOGFRAMES(x) log_printf    x
#else
#define LOGFRAMES(x)
#endif

/******************************************************************************/

#define PALETTE_NUM_COLORS      256
#define PALETTE_COLORS_BPP      (8 * 3)

#define PALETTE_SIZE    (PALETTE_NUM_COLORS * (PALETTE_COLORS_BPP / 8))

#define VIDEO_BPP           8

#define MAX_AUDIO_BUFFER_SIZE   (((44800 * 2) / 50) * 2)

/* each KEYFRAME_INTERVAL frame will be key one */
#define KEYFRAME_INTERVAL  (300)

/******************************************************************************/

static int frameno = 0;

static zmvb_init_flags_t iflg = ZMBV_INIT_FLAG_NONE;

static int complevel = -1;  /* compression level, -1 means default */
static int no_zlib = 0;

static uint8_t cur_pal[PALETTE_SIZE];

static int16_t cur_audio[MAX_AUDIO_BUFFER_SIZE];

static zmbv_avi_t zavi;
static zmbv_codec_t zcodec;
static zmbv_format_t fmt;

static int work_buffer_size;
static void *video_work_buffer = NULL;

static int video_codec;
static int audio_codec;

static uint8_t *cur_screen = NULL;

/* general */
static int file_init_done = 1;

/* audio */
static soundmovie_buffer_t zmbvdrv_audio_in;
static int audio_init_done = 0;
static int audio_is_open = 0;

/* video */
static int video_init_done = 0;
static int video_is_open = 0;
static int video_width, video_height; /* initialized by  zmbvdrv_init_video */

static unsigned int framecounter;

/* resources */
static int format_index = 0;
static char *zmbv_format = NULL;

/* these are dictated by the emulator */
static int audio_freq = 48000;    /* initialized by zmbv_soundmovie_init */
static int audio_channels = 1;       /* initialized by zmbv_soundmovie_init */

static double video_framerate = 50.125f; /* initialized by zmbvdrv_init_video */

CLOCK clk_startframe;
CLOCK clk_frame_cycles;
CLOCK clk_last_audio_frame;
CLOCK clk_this_audio_frame;
CLOCK clk_last_video_frame;
CLOCK clk_this_video_frame;

static int zmbvdrv_init_file(void);

/******************************************************************************/

#define AV_CODEC_ID_NONE            0
#define AV_CODEC_ID_ZMBV            1
#define AV_CODEC_ID_PCM_S16LE       2

static gfxoutputdrv_codec_t avi_audio_codeclist[] = {
    { AV_CODEC_ID_PCM_S16LE,    "PCM uncompressed" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t raw_audio_codeclist[] = {
    { AV_CODEC_ID_NONE,         "none" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t avi_video_codeclist[] = {
    { AV_CODEC_ID_ZMBV,        "ZMBV (lossless)" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t raw_video_codeclist[] = {
    { AV_CODEC_ID_ZMBV,        "ZMBV (lossless)" },
    { 0, NULL }
};

#define VIDEO_OPTIONS \
    (GFXOUTPUTDRV_HAS_AUDIO_CODECS | \
     GFXOUTPUTDRV_HAS_VIDEO_CODECS)

#define RAW_VIDEO_OPTIONS \
    (GFXOUTPUTDRV_HAS_VIDEO_CODECS)

/* formatlist is filled from with available formats and codecs at init time */
gfxoutputdrv_format_t *zmbvdrv_formatlist = NULL;
static gfxoutputdrv_format_t formats_to_test[] =
{
    { "raw",        raw_audio_codeclist, raw_video_codeclist, RAW_VIDEO_OPTIONS },
    { "avi",        avi_audio_codeclist, avi_video_codeclist, VIDEO_OPTIONS },
    { NULL, NULL, NULL, 0 }
};

static int set_container_format(const char *val, void *param)
{
    int i;

    /* kludge to prevent crash at startup when using --help on the commandline */
    if (zmbvdrv_formatlist == NULL) {
        return 0;
    }

    format_index = -1;

    for (i = 0; zmbvdrv_formatlist[i].name != NULL; i++) {
        if (strcmp(val, zmbvdrv_formatlist[i].name) == 0) {
            format_index = i;
        }
    }
    format_index = 0;

    if (format_index < 0) {
        return -1;
    }

    util_string_set(&zmbv_format, val);

    return 0;
}

static int set_audio_codec(int val, void *param)
{
    audio_codec = val;
    return 0;
}

static int set_video_codec(int val, void *param)
{
    video_codec = val;
    return 0;
}

/*---------- Resources ------------------------------------------------*/

static const resource_string_t resources_string[] = {
    { "ZMBVFormat", "avi", RES_EVENT_NO, NULL,
      &zmbv_format, set_container_format, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "ZMBVAudioCodec", AV_CODEC_ID_PCM_S16LE, RES_EVENT_NO, NULL,
      &audio_codec, set_audio_codec, NULL },
    { "ZMBVVideoCodec", AV_CODEC_ID_ZMBV, RES_EVENT_NO, NULL,
      &video_codec, set_video_codec, NULL },
    RESOURCE_INT_LIST_END
};

static int zmbvdrv_resources_init(void)
{
    LOG(("zmbvdrv_resources_init"));
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*---------- Commandline options --------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    CMDLINE_LIST_END
};

static int zmbvdrv_cmdline_options_init(void)
{
    LOG(("zmbvdrv_cmdline_options_init"));
    return cmdline_register_options(cmdline_options);
}

/*---------------------------------------------------------------------*/

/*-----------------------*/
/* audio stream encoding */
/*-----------------------*/

/* called by zmbvdrv_init_video -> zmbvdrv_init_file */
static int zmbvdrv_open_audio(int speed, int channels)
{
    LOG(("zmbvdrv_open_audio(speed:%d channels:%d", speed, channels));

    zmbvdrv_audio_in.buffer = lib_malloc(MAX_AUDIO_BUFFER_SIZE);    /* is this correct? */
    if (zmbvdrv_audio_in.buffer == NULL) {
        log_error(LOG_DEFAULT, "zmbvdrv: Error allocating audio buffer (%u bytes)", (unsigned)MAX_AUDIO_BUFFER_SIZE);
        return -1;
    }
    zmbvdrv_audio_in.size = round((double)audio_freq / video_framerate);
    LOG(("zmbvdrv_open_audio freq:%d fps:%f bufsize:%d", audio_freq, video_framerate, zmbvdrv_audio_in.size));
#if 0
    memset(cur_audio, 0, zmbvdrv_audio_in.size * 2);
    if (zmbv_avi_write_chunk_audio(zavi, &cur_audio[0], zmbvdrv_audio_in.size) < 0) {
        LOG(("FATAL: can't write audio frame for screen #%d", 0));
    }
#endif
    return 0;
}

static void zmbvdrv_close_audio(void)
{
    LOG(("zmbvdrv_close_audio()"));

    audio_is_open = 0;
    if(zmbvdrv_audio_in.buffer != NULL) {
        lib_free(zmbvdrv_audio_in.buffer);
    }
    zmbvdrv_audio_in.buffer = NULL;
    zmbvdrv_audio_in.size = 0;
}

/* Soundmovie API soundmovie_funcs_t.init */
/* called via zmbvdrv_soundmovie_funcs->init */
static int zmbv_soundmovie_init(int speed, int channels, soundmovie_buffer_t ** audio_in)
{
    LOG(("zmbv_soundmovie_init(speed:%d channels: %d)", speed, channels));

    audio_init_done = 1;

    *audio_in = &zmbvdrv_audio_in;

    (*audio_in)->size = 0; /* not allocated yet */
    (*audio_in)->used = 0;

    /* we do not support resampling */
    audio_freq = speed;
    audio_channels = channels;

    if (video_init_done) {
        zmbvdrv_init_file();
    }

    return 0;
}

/* Soundmovie API soundmovie_funcs_t.encode */
/* called via zmbvdrv_soundmovie_funcs->encode */
/* triggered by soundffmpegaudio->write */
static int zmbv_soundmovie_encode(soundmovie_buffer_t *audio_in)
{
    int ret = 0;

    clk_last_audio_frame = clk_this_audio_frame;
    clk_this_audio_frame = maincpu_clk;

    LOGFRAMES(("zmbv_soundmovie_encode(size:%d used:%d channels:%d) clk:%ld frame:%d",
               audio_in->size, audio_in->used, audio_channels, clk_this_audio_frame, frameno));

    /* FIXME: we might have an endianess problem here, we might have to swap lo/hi on BE machines */
    if (audio_channels == 1) {
        int i, o;
#if 1
        /* convert mono -> stereo */
        for (i = o = 0; i < audio_in->used; i++, o+=2) {
            cur_audio[o] = audio_in->buffer[i];
            cur_audio[o+1] = audio_in->buffer[i];
        }
        /* write avi chunks */
        if (zmbv_avi_write_chunk_audio(zavi, &cur_audio[0], audio_in->used * 4) < 0) {
            LOG(("FATAL: can't write audio frame for screen #%d", frameno));
            ret = -1;
        }
#else
        /* FIXME: we should write the mono stream into the avi instead */
#endif
    } else if (audio_channels == 2) {
        int i, o;
        for (i = o = 0; i < audio_in->used; i+=2, o+=2) {
            cur_audio[o] = audio_in->buffer[i];
            cur_audio[o+1] = audio_in->buffer[i+1];
        }
        /* write avi chunks */
        if (zmbv_avi_write_chunk_audio(zavi, &audio_in->buffer[0], audio_in->used * 2) < 0) {
            LOG(("FATAL: can't write audio frame for screen #%d", frameno));
            ret = -1;
        }
    } else {
        ret = -1;
    }

    audio_in->used = 0;
    return ret;
}

/* Soundmovie API soundmovie_funcs_t.close */
/* called via ffmpegexedrv_soundmovie_funcs->close */
static void zmbv_soundmovie_close(void)
{
    LOG(("zmbv_soundmovie_close()"));
    /* just stop the whole recording */
    screenshot_stop_recording();
}

static soundmovie_funcs_t zmbvdrv_soundmovie_funcs = {
    zmbv_soundmovie_init,
    zmbv_soundmovie_encode,
    zmbv_soundmovie_close
};

/*-----------------------*/
/* video stream encoding */
/*-----------------------*/
static int zmbvdrv_fill_rgb_image(screenshot_t *screenshot)
{
    int x, y;
    int dx, dy;
    int bufferoffset;
    int x_dim = screenshot->width;
    int y_dim = screenshot->height;

    if ((video_width == 0) || (video_height == 0)) {
        return 0;
    }

    /* center the screenshot in the video */
    dx = (video_width - x_dim) / 2;
    dy = (video_height - y_dim) / 2;
    bufferoffset = screenshot->x_offset + (dx < 0 ? -dx : 0)
        + (screenshot->y_offset + (dy < 0 ? -dy : 0)) * screenshot->draw_buffer_line_size;

    for (x = 0; x < PALETTE_NUM_COLORS; x++) {
        cur_pal[(x * (PALETTE_COLORS_BPP / 8)) + 0] = screenshot->palette->entries[x].red;
        cur_pal[(x * (PALETTE_COLORS_BPP / 8)) + 1] = screenshot->palette->entries[x].green;
        cur_pal[(x * (PALETTE_COLORS_BPP / 8)) + 2] = screenshot->palette->entries[x].blue;
    }

    LOGFRAMES(("zmbvdrv_fill_rgb_image video_width/height: %dx%d", video_width, video_height));
    for (y = 0; y < video_height; y++) {
        for (x = 0; x < video_width; x++) {
            cur_screen[(y * video_width) + x] = screenshot->draw_buffer[bufferoffset + x];
        }
        bufferoffset += screenshot->draw_buffer_line_size;
    }
    LOGFRAMES(("zmbvdrv_fill_rgb_image done"));

    return 0;
}

static uint8_t *zmbvdrv_alloc_picture(int width, int height)
{
    /* the pixel format is always 8bpp, with a 256 entries, 24bit, palette */
    LOG(("zmbvdrv_alloc_picture width:%d height:%d", width, height));
    return lib_malloc(width * height);
}

/* called by zmbvdrv_init_file() */
static int zmbvdrv_open_video(int width, int height)
{
    LOG(("zmbvdrv_open_video width:%d height:%d", width, height));
    /* MOVE? open the codec */
    video_is_open = 1;
    /* FIXME: allocate the encoded raw picture */
    cur_screen = zmbvdrv_alloc_picture(width, height);
    if (cur_screen == NULL) {
        return -1;
    }
    return 0;
}

/* called by zmbvdrv_close() */
static void zmbvdrv_close_video(void)
{
    LOG(("zmbvdrv_close_video"));
    video_is_open = 0;
    if (cur_screen != NULL) {
        lib_free (cur_screen);
        cur_screen = NULL;
    }
}
/* called by zmbvdrv_save */
static void zmbvdrv_init_video(screenshot_t *screenshot)
{
    video_width = screenshot->width;
    video_height = screenshot->height;
    LOG(("zmbvdrv_init_video w:%d h:%d", video_width, video_height));
    video_init_done = 1;
    {
        double time_base, fps;
        clk_frame_cycles = machine_get_cycles_per_frame();
        time_base = ((double)machine_get_cycles_per_frame()) / ((double) machine_get_cycles_per_second());
        fps = 1.0f / time_base;
        LOG(("zmbvdrv_init_video fps: %f timebase: %f", fps, time_base));
        video_framerate = fps;
    }

    framecounter = 0;
    if (audio_init_done) {
        zmbvdrv_init_file();
    }
}

/* called by zmbvdrv_init_video */
static int zmbvdrv_init_file(void)
{
    LOG(("zmbvdrv_init_file() video_init_done:%d audio_init_done:%d", video_init_done, audio_init_done));
    if (!video_init_done || !audio_init_done) {
        return 0;
    }

    if (zmbvdrv_open_audio(audio_freq, audio_channels) < 0) {
        ui_error("zmbvdrv: Cannot open audio stream");
        screenshot_stop_recording();
        return -1;
    }

    if (zmbvdrv_open_video(video_width, video_height) < 0) {
        ui_error("zmbvdrv: Cannot open video stream");
        screenshot_stop_recording();
        return -1;
    }

    log_debug(LOG_DEFAULT, "zmbvdrv: Initialized file successfully");

    file_init_done = 1;

    return 0;
}

/* Driver API gfxoutputdrv_t.save */
/* called once to start recording video+audio */
static int zmbvdrv_save(screenshot_t *screenshot, const char *filename)
{
    LOG(("zmbvdrv_save(filename:'%s')", filename));

    audio_init_done = 0;
    video_init_done = 0;
    file_init_done = 0;

    /* complevel = 9; */
    /* no_zlib = 0; */

    clk_startframe = maincpu_clk;
    clk_this_video_frame = clk_this_audio_frame = clk_startframe;
    LOG(("zmbvdrv_save start clock: %ld", clk_startframe));

    if (no_zlib) {
        iflg |= ZMBV_INIT_FLAG_NOZLIB;
    }
    LOG(("zmbvdrv_save using compression level %d", complevel));

    zmbvdrv_init_video(screenshot);

    zavi = zmbv_avi_start(filename, video_width, video_height, video_framerate, audio_freq);
    if (zavi == NULL) {
        LOG(("FATAL: can't create output file!"));
        return -1;
    }

    /* init encoder */
    zcodec = zmbv_codec_new(iflg, complevel);
    if (zcodec == NULL) {
        LOG(("FATAL: can't create codec!"));
        return -1;
    }
    fmt = zmbv_bpp_to_format(VIDEO_BPP);
    work_buffer_size = zmbv_work_buffer_size(video_width, video_height, fmt);
    video_work_buffer = malloc(work_buffer_size);
    if (video_work_buffer == NULL) {
        LOG(("FATAL: can't init buffer!"));
        return -1;
    }
    if (zmbv_encode_setup(zcodec, video_width, video_height) < 0) {
        LOG(("FATAL: can't init encoder!"));
        free(video_work_buffer);
        video_work_buffer = NULL;
        return -1;
    }

    frameno = 0;

    soundmovie_start(&zmbvdrv_soundmovie_funcs);

    return 0;
}

/* Driver API gfxoutputdrv_t.close */
static int zmbvdrv_close(screenshot_t *screenshot)
{
    /* write the trailer, if any */

    soundmovie_stop();

    zmbvdrv_close_video();
    zmbvdrv_close_audio();

    /* close the output file */

    /* free the streams */
    if (video_work_buffer != NULL) { free(video_work_buffer); video_work_buffer = NULL; }
    zmbv_codec_free(zcodec);

    zmbv_avi_stop(zavi);

    /* free the stream */
    log_debug(LOG_DEFAULT, "zmbvdrv: Closed successfully");

    file_init_done = 0;

    return 0;
}

/* Driver API gfxoutputdrv_t.record */
/* triggered by screenshot_record, periodically called to output video data stream */
static int zmbvdrv_record(screenshot_t *screenshot)
{
    int ret = -1;
    int32_t written;
    int flags;
    CLOCK clk_diff;

    if (audio_init_done && video_init_done && !file_init_done) {
        zmbvdrv_init_file();
    }

    clk_last_video_frame = clk_this_video_frame;
    clk_this_video_frame = maincpu_clk;

    if (clk_this_video_frame > clk_this_audio_frame) {
        /* video ahead of audio */
        clk_diff = clk_this_video_frame - clk_this_audio_frame;
        if (clk_diff > clk_frame_cycles) {
            LOG(("zmbvdrv_record video>audio %ld %ld frame:%ld diff:%ld", clk_this_video_frame, clk_this_audio_frame, clk_frame_cycles, clk_diff));
            /*return 0;*/ /* skip this frame? */
        }
    } else if (clk_this_audio_frame > clk_this_video_frame) {
        /* audio is ahead of video */
        clk_diff = clk_this_audio_frame - clk_this_video_frame;
        if (clk_diff > clk_frame_cycles) {
            LOG(("zmbvdrv_record video<audio %ld %ld frame:%ld diff:%ld", clk_this_video_frame, clk_this_audio_frame, clk_frame_cycles, clk_diff));
        }
    }

    zmbvdrv_fill_rgb_image(screenshot);

    flags = ((frameno % KEYFRAME_INTERVAL == 0) ? ZMBV_PREP_FLAG_KEYFRAME : ZMBV_PREP_FLAG_NONE);

    frameno++;

    LOGFRAMES(("zmbvdrv_record: frame %d (clk:%ld)", frameno, clk_this_video_frame));

    /* encode video frame */
    if (zmbv_encode_prepare_frame(zcodec, flags, fmt, cur_pal, video_work_buffer, work_buffer_size) < 0) {
        LOG(("FATAL: can't prepare frame for screen #%d", frameno));
        goto quit;
    }
    for (int y = 0; y < video_height; ++y) {
        if (zmbv_encode_line(zcodec, cur_screen+(y*video_width)) < 0) {
            LOG(("FATAL: can't encode line #%d for screen #%d", y, frameno));
            goto quit;
        }
    }
    written = zmvb_encode_finish_frame(zcodec);
    if (written < 0) {
        LOG(("FATAL: can't finish frame for screen #%d", frameno));
        goto quit;
    }
    /* write avi chunk */
    if (zmbv_avi_write_chunk_video(zavi, video_work_buffer, written) < 0) {
        LOG(("FATAL: can't write compressed frame for screen #%d", frameno));
        goto quit;
    }
    ret = 0;
quit:
    if (ret < 0) {
        log_debug(LOG_DEFAULT, "Error while writing video frame");
        return -1;
    }

    return 0;
}

/* Driver API gfxoutputdrv_t.write */
static int zmbvdrv_write(screenshot_t *screenshot)
{
    return 0;
}


static void zmbvdrv_shutdown(void);


static gfxoutputdrv_t zmbv_drv = {
    GFXOUTPUTDRV_TYPE_VIDEO,
    "ZMBV",
    "ZMBV (Library)",
    NULL,
    NULL, /* filled in zmbv_get_formats_and_codecs */
    NULL, /* open */
    zmbvdrv_close,
    zmbvdrv_write,
    zmbvdrv_save,
    NULL,
    zmbvdrv_record,
    zmbvdrv_shutdown,
    zmbvdrv_resources_init,
    zmbvdrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    , NULL
#endif
};

/* gfxoutputdrv_t.shutdown */
static void zmbvdrv_shutdown(void)
{
    int i = 0;

    if (zmbv_drv.formatlist != NULL) {

        while (zmbv_drv.formatlist[i].name != NULL) {
            lib_free(zmbv_drv.formatlist[i].name);
            if (zmbv_drv.formatlist[i].audio_codecs != NULL) {
                lib_free(zmbv_drv.formatlist[i].audio_codecs);
            }
            if (zmbv_drv.formatlist[i].video_codecs != NULL) {
                lib_free(zmbv_drv.formatlist[i].video_codecs);
            }
            i++;
        }
        lib_free(zmbv_drv.formatlist);
    }
    lib_free(zmbv_format);
}

static void zmbv_get_formats_and_codecs(void)
{
    int i, j, ai = 0, vi = 0, f;
    gfxoutputdrv_codec_t *audio_codec_list;
    gfxoutputdrv_codec_t *video_codec_list;
    gfxoutputdrv_codec_t *ac, *vc;

    LOG(("zmbv_get_formats_and_codecs()"));

    f = 0;
    zmbvdrv_formatlist = lib_malloc(sizeof(gfxoutputdrv_format_t));

    for (i = 0; formats_to_test[i].name != NULL; i++) {
            audio_codec_list = NULL;
            video_codec_list = NULL;
            if (formats_to_test[i].audio_codecs != NULL) {
                ai = 0;
                audio_codec_list = lib_malloc(sizeof(gfxoutputdrv_codec_t));
                ac = formats_to_test[i].audio_codecs;
                for (j = 0; ac[j].name != NULL; j++) {
                    LOG(("audio_codec_list[%d].name='%s'", ai, ac[j].name));
                    audio_codec_list[ai++] = ac[j];
                    audio_codec_list = lib_realloc(audio_codec_list, (ai + 1) * sizeof(gfxoutputdrv_codec_t));
                }
                audio_codec_list[ai].name = NULL;
            }
            if (formats_to_test[i].video_codecs != NULL) {
                vi = 0;
                video_codec_list = lib_malloc(sizeof(gfxoutputdrv_codec_t));
                vc = formats_to_test[i].video_codecs;
                for (j = 0; vc[j].name != NULL; j++) {
                    LOG(("video_codec_list[%d].name='%s'", vi, formats_to_test[i].video_codecs[j].name));
                    video_codec_list[vi++] = formats_to_test[i].video_codecs[j];
                    video_codec_list = lib_realloc(video_codec_list, (vi + 1) * sizeof(gfxoutputdrv_codec_t));
                }
                video_codec_list[vi].name = NULL;
            }
            if (((audio_codec_list == NULL) || (ai > 0)) && ((video_codec_list == NULL) || (vi > 0))) {
                zmbvdrv_formatlist[f].flags = formats_to_test[i].flags;
                zmbvdrv_formatlist[f].name = lib_strdup(formats_to_test[i].name);
                LOG(("zmbvdrv_formatlist[%d].name='%s'", f, zmbvdrv_formatlist[f].name));
                zmbvdrv_formatlist[f].audio_codecs = audio_codec_list;
                zmbvdrv_formatlist[f++].video_codecs = video_codec_list;
                zmbvdrv_formatlist = lib_realloc(zmbvdrv_formatlist, (f + 1) * sizeof(gfxoutputdrv_format_t));
            }
    }
    zmbvdrv_formatlist[f].name = NULL;
    zmbv_drv.formatlist = zmbvdrv_formatlist;
}

/* public, init this output driver */
void gfxoutput_init_zmbv(int help)
{
    LOG(("gfxoutput_init_zmbv()"));
    if (help) {
        gfxoutput_register(&zmbv_drv);
        return;
    }

    zmbv_get_formats_and_codecs();
    gfxoutput_register(&zmbv_drv);
}
