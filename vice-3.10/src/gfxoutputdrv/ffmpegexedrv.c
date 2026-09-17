/** \file   ffmpegexedrv.c
 * \brie    Movie driver using FFMPEG executable
 *
 * \author  Andreas Matthies <andreas.matthies@gmx.net>
 * \author  groepaz@gmx.net
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

/*
    This is a video output driver implemented to replace the original FFMPEG
    driver. Instead of linking in the libraries, this one calls the ffmpeg
    executable and pipes the data to it.

    bugs/open ends:
    - audio-only saving does not work correctly, apparently due to how the codecs
      are being listed (and somehow the results come out wrong)
    - there should probably be an endianess conversion for the audio stream
    - there should probably be a check that makes sure "ffmpeg" exists and
      can be executed
    - the available codecs/containers should get queried from the ffmpeg binary

    * https://ffmpeg.org/ffmpeg.html
    * https://ffmpeg.org/~michael/nut.txt
    * https://github.com/lu-zero/nut/blob/master/src/
    * https://gist.github.com/victusfate/b1fbc822957020bfc063

 */

/* #define DEBUG_FFMPEG */
/* #define DEBUG_FFMPEG_FRAMES */

#include "vice.h"

#include <assert.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "archdep.h"
#include "archdep_sleep.h"
#include "cmdline.h"
#include "coproc.h"
#include "ffmpegexedrv.h"
#include "gfxoutput.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "soundmovie.h"
#include "uiapi.h"
#include "util.h"
#include "vicesocket.h"

/* #define VICE_IS_SERVER */

#ifdef DEBUG_FFMPEG
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

#ifdef DEBUG_FFMPEG_FRAMES
#define DBGFRAMES(x) log_printf  x
#else
#define DBGFRAMES(x)
#endif

#define AV_CODEC_ID_NONE      0
#define AV_CODEC_ID_MP2             1
#define AV_CODEC_ID_MP3             2
#define AV_CODEC_ID_FLAC            3
#define AV_CODEC_ID_PCM_S16LE       4
#define AV_CODEC_ID_AAC             5
#define AV_CODEC_ID_AC3             6
#define AV_CODEC_ID_MPEG4           7
#define AV_CODEC_ID_MPEG1VIDEO      8
#define AV_CODEC_ID_FFV1            9
#define AV_CODEC_ID_H264            10
#define AV_CODEC_ID_THEORA          11
#define AV_CODEC_ID_H265            12

/* FIXME: check/fix make sure this returns valid ffmpeg vcodec/acodec strings */
static char *av_codec_get_option(int id)
{
    switch(id) {
        case AV_CODEC_ID_NONE: return "none";
        case AV_CODEC_ID_MP2: return "mp2";
        case AV_CODEC_ID_MP3: return "mp3";
        case AV_CODEC_ID_FLAC: return "flac";
        case AV_CODEC_ID_PCM_S16LE: return "s16le";
        case AV_CODEC_ID_AAC: return "aac";
        case AV_CODEC_ID_AC3: return "ac3";
        case AV_CODEC_ID_MPEG4: return "mpeg4";
        case AV_CODEC_ID_MPEG1VIDEO: return "mpeg1video";
        case AV_CODEC_ID_FFV1: return "ffv1";
        case AV_CODEC_ID_H264: return "h264";
        case AV_CODEC_ID_THEORA: return "theora";
        case AV_CODEC_ID_H265: return "h265";
        default: return "unknown";
    }
}

/******************************************************************************/

/* FIXME: some SDL UIs use ffmpegdrv_formatlist directly */
#define ffmpegexedrv_formatlist ffmpegdrv_formatlist

static gfxoutputdrv_codec_t mp4_audio_codeclist[] = {
    { AV_CODEC_ID_AAC,          "AAC" },
    { AV_CODEC_ID_MP3,          "MP3" },
    { AV_CODEC_ID_AC3,          "AC3" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t mp4_video_codeclist[] = {
    { AV_CODEC_ID_H264,         "H264" },
    { AV_CODEC_ID_H265,         "H265" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t ogg_audio_codeclist[] = {
    { AV_CODEC_ID_FLAC,         "FLAC" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t ogg_video_codeclist[] = {
    { AV_CODEC_ID_THEORA,       "Theora" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t avi_audio_codeclist[] = {
    { AV_CODEC_ID_MP2,          "MP2" },
    { AV_CODEC_ID_MP3,          "MP3" },
    { AV_CODEC_ID_FLAC,         "FLAC" },
    { AV_CODEC_ID_PCM_S16LE,    "PCM uncompressed" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t avi_video_codeclist[] = {
    { AV_CODEC_ID_MPEG4,        "MPEG4 (DivX)" },
    { AV_CODEC_ID_MPEG1VIDEO,   "MPEG1" },
    { AV_CODEC_ID_FFV1,         "FFV1 (lossless)" },
    { AV_CODEC_ID_H264,         "H264" },
    { AV_CODEC_ID_THEORA,       "Theora" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t mp3_audio_codeclist[] = {
    { AV_CODEC_ID_MP3,          "mp3" },
    { 0, NULL }
};

static gfxoutputdrv_codec_t mp2_audio_codeclist[] = {
    { AV_CODEC_ID_MP2,          "mp2" },
    { 0, NULL }
};

#if 0
static gfxoutputdrv_codec_t none_codeclist[] = {
    { AV_CODEC_ID_NONE, "" },
    { 0, NULL }
};
#endif

#define VIDEO_OPTIONS \
    (GFXOUTPUTDRV_HAS_AUDIO_CODECS | \
     GFXOUTPUTDRV_HAS_VIDEO_CODECS | \
     GFXOUTPUTDRV_HAS_AUDIO_BITRATE | \
     GFXOUTPUTDRV_HAS_VIDEO_BITRATE | \
     GFXOUTPUTDRV_HAS_HALF_VIDEO_FRAMERATE)

#define AUDIO_OPTIONS \
    (GFXOUTPUTDRV_HAS_AUDIO_CODECS | \
     GFXOUTPUTDRV_HAS_AUDIO_BITRATE)

/* formatlist is filled from with available formats and codecs at init time */
gfxoutputdrv_format_t *ffmpegexedrv_formatlist = NULL;
static gfxoutputdrv_format_t output_formats_to_test[] =
{
    { "mp4",        mp4_audio_codeclist, mp4_video_codeclist, VIDEO_OPTIONS },
    { "ogg",        ogg_audio_codeclist, ogg_video_codeclist, VIDEO_OPTIONS },
    { "avi",        avi_audio_codeclist, avi_video_codeclist, VIDEO_OPTIONS },
    { "matroska",   mp4_audio_codeclist, mp4_video_codeclist, VIDEO_OPTIONS },
    { "wav",        avi_audio_codeclist, NULL,                AUDIO_OPTIONS },
    { "mp3",        mp3_audio_codeclist, NULL,                AUDIO_OPTIONS }, /* formats expects png which fails in VICE */
    { "mp2",        mp2_audio_codeclist, NULL,                AUDIO_OPTIONS },
    { NULL, NULL, NULL, 0 }
};

/******************************************************************************/

/* general */
static int file_init_done;

#define DUMMY_FRAMES_VIDEO  1
#define DUMMY_FRAMES_AUDIO  ((int)round(fps * (double)AUDIO_SKIP_SECONDS))

#define AUDIO_SKIP_SECONDS  4

#define SOCKETS_RANGE_FIRST 53248
#define SOCKETS_RANGE_LAST  57343
static int current_video_port = SOCKETS_RANGE_FIRST;
static int current_audio_port = SOCKETS_RANGE_FIRST + 1;

/* input video stream */
#define INPUT_VIDEO_BPP     3

static double time_base;
static double fps;                  /* frames per second */
static uint64_t framecounter = 0;   /* number of processed video frames */

typedef struct {
    uint8_t *data;
    int linesize;
} VIDEOFrame;
static VIDEOFrame *video_st_frame;

/* input audio stream */
#define AUDIO_BUFFER_SAMPLES        0x400
#define AUDIO_BUFFER_MAX_CHANNELS   2

static soundmovie_buffer_t ffmpegexedrv_audio_in;
static int audio_init_done;
static int audio_is_open;
static int audio_has_codec = -1;
static uint64_t audio_input_counter = 0;    /* total samples played */
static int audio_input_sample_rate = -1;    /* samples per second */
static int audio_input_channels = -1;

/* output video */
static int video_init_done;
static int video_is_open;
static int video_has_codec = -1;
static int video_width = -1;
static int video_height = -1;

/* ffmpeg interface */
static int ffmpeg_stdin = 0;
static int ffmpeg_stdout = 0;
static vice_pid_t ffmpeg_pid = 0;

static vice_network_socket_t *ffmpeg_video_socket = NULL;
static vice_network_socket_t *ffmpeg_audio_socket = NULL;
#ifdef VICE_IS_SERVER
static vice_network_socket_t *ffmpeg_video_listen_socket = NULL;
static vice_network_socket_t *ffmpeg_audio_listen_socket = NULL;
#endif
static char *outfilename = NULL;

log_t ffmpeg_log = LOG_DEFAULT;

/******************************************************************************/

static int ffmpegexedrv_init_file(void);
static void ffmpegexedrv_shutdown(void);

/******************************************************************************/
/* resources */

static char *ffmpegexe_format = NULL;    /* FFMPEGFormat */
static int format_index;    /* FFMPEGFormat */
static int audio_codec;
static int video_codec;
static int audio_bitrate;
static int video_bitrate;
static int video_halve_framerate;

static int set_container_format(const char *val, void *param)
{
    int i;

    /* kludge to prevent crash at startup when using --help on the commandline */
    if (ffmpegexedrv_formatlist == NULL) {
        return 0;
    }

    format_index = -1;

    for (i = 0; ffmpegexedrv_formatlist[i].name != NULL; i++) {
        if (strcmp(val, ffmpegexedrv_formatlist[i].name) == 0) {
            format_index = i;
        }
    }

    if (format_index < 0) {
        return -1;
    }

    if (ffmpegexe_format) {
        lib_free(ffmpegexe_format);
        ffmpegexe_format = NULL;
    }
    util_string_set(&ffmpegexe_format, val);

    return 0;
}

static int set_audio_bitrate(int val, void *param)
{
    audio_bitrate = val;

    if ((audio_bitrate < VICE_FFMPEG_AUDIO_RATE_MIN)
        || (audio_bitrate > VICE_FFMPEG_AUDIO_RATE_MAX)) {
        audio_bitrate = VICE_FFMPEG_AUDIO_RATE_DEFAULT;
    }
    return 0;
}

static int set_video_bitrate(int val, void *param)
{
    video_bitrate = val;

    if ((video_bitrate < VICE_FFMPEG_VIDEO_RATE_MIN)
        || (video_bitrate > VICE_FFMPEG_VIDEO_RATE_MAX)) {
        video_bitrate = VICE_FFMPEG_VIDEO_RATE_DEFAULT;
    }
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

static int set_video_halve_framerate(int value, void *param)
{
    int val = value ? 1 : 0;

    if (video_halve_framerate != val && screenshot_is_recording()) {
        ui_error("Can't change framerate while recording. Try again later.");
        return 0;
    }

    video_halve_framerate = val;

    return 0;
}

/*---------- Resources ------------------------------------------------*/

static const resource_string_t resources_string[] = {
/* FIXME: register only here, not in the internal ffmpeg driver */
    { "FFMPEGFormat", "mp4", RES_EVENT_NO, NULL,
      &ffmpegexe_format, set_container_format, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
/* FIXME: register only here, not in the internal ffmpeg driver */
    { "FFMPEGAudioBitrate", VICE_FFMPEG_AUDIO_RATE_DEFAULT,
      RES_EVENT_NO, NULL,
      &audio_bitrate, set_audio_bitrate, NULL },
    { "FFMPEGVideoBitrate", VICE_FFMPEG_VIDEO_RATE_DEFAULT,
      RES_EVENT_NO, NULL,
      &video_bitrate, set_video_bitrate, NULL },
    { "FFMPEGAudioCodec", AV_CODEC_ID_AAC, RES_EVENT_NO, NULL,
      &audio_codec, set_audio_codec, NULL },
    { "FFMPEGVideoCodec", AV_CODEC_ID_H264, RES_EVENT_NO, NULL,
      &video_codec, set_video_codec, NULL },
    { "FFMPEGVideoHalveFramerate", 0, RES_EVENT_NO, NULL,
      &video_halve_framerate, set_video_halve_framerate, NULL },
    RESOURCE_INT_LIST_END
};

/* Driver API gfxoutputdrv_t.resources_init */
static int ffmpegexedrv_resources_init(void)
{
    ffmpeg_log = log_open("FFMPEG");

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*---------- Commandline options --------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
/* FIXME: register only here, not in the internal ffmpeg driver */
    { "-ffmpegaudiobitrate", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FFMPEGAudioBitrate", NULL,
      "<value>", "Set bitrate for audio stream in media file" },
    { "-ffmpegvideobitrate", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FFMPEGVideoBitrate", NULL,
      "<value>", "Set bitrate for video stream in media file" },
    CMDLINE_LIST_END
};

/* Driver API gfxoutputdrv_t.cmdline_options_init */
static int ffmpegexedrv_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/*---------------------------------------------------------------------*/

static void log_resource_values(const char *func)
{
    DBG(("%s FFMPEGFormat:%s", func, ffmpegexe_format));
    DBG(("%s FFMPEGVideoCodec:%d:'%s'", func, video_codec, av_codec_get_option(video_codec)));
    DBG(("%s FFMPEGVideoBitrate:%d", func, video_bitrate));
    DBG(("%s FFMPEGAudioCodec:%d:'%s'", func, audio_codec, av_codec_get_option(audio_codec)));
    DBG(("%s FFMPEGAudioBitrate:%d", func, audio_bitrate));
    DBG(("%s FFMPEGVideoHalveFramerate:%d", func, video_halve_framerate));
}

static void prepare_port_numbers(void)
{
    current_video_port += 2;
    current_audio_port += 2;
    if (current_audio_port > SOCKETS_RANGE_LAST) {
        current_video_port = SOCKETS_RANGE_FIRST;
        current_audio_port = SOCKETS_RANGE_FIRST + 1;
    }
    log_message(ffmpeg_log, "prepare_port_numbers %d:%d", current_video_port, current_audio_port);
}

static ssize_t write_video_frame(VIDEOFrame *pic)
{
    ssize_t len = INPUT_VIDEO_BPP * video_height * video_width;
    ssize_t res;

    if ((video_has_codec > 0) && (video_codec != AV_CODEC_ID_NONE)) {
        if (ffmpeg_video_socket == 0) {
            log_error(ffmpeg_log, "FFMPEG: write_video_frame ffmpeg_video_socket is 0 (framecount:%"PRIu64")", framecounter);
            return -1;
        }
        res = vice_network_send(ffmpeg_video_socket, pic->data, len, 0 /* flags */);
        if (res < 0) {
            return -1;
        }
        return len - res;
    }
    return 0;
}

static int write_initial_video_frames(void)
{
    int len;
    int frm;
    /* clear frame */
    len = INPUT_VIDEO_BPP * video_height * video_width;
    DBG(("video len:%d (%d)", len, len * DUMMY_FRAMES_VIDEO));
    memset(video_st_frame->data, 0, len);
    for (frm = 0; frm < DUMMY_FRAMES_VIDEO; frm++) {
        if (write_video_frame(video_st_frame) < 0) {
            return -1;
        }
    }
    return 0;
}

static int write_initial_audio_frames(void)
{
    ssize_t len;
    int frm;
    ssize_t res;

    /* clear frame */
    len = (sizeof(uint16_t) * audio_input_sample_rate) / fps;
    if (video_halve_framerate) {
        len /= 2;
    }
    DBG(("audio len:%zd (%zd)", len, len * DUMMY_FRAMES_AUDIO));
    memset(ffmpegexedrv_audio_in.buffer, 0, len);
    for (frm = 0; frm < DUMMY_FRAMES_AUDIO; frm++) {
        res = vice_network_send(ffmpeg_audio_socket, ffmpegexedrv_audio_in.buffer, len, 0 /* flags */);
        if (res != len) {
            log_error(ffmpeg_log, "ffmpegexedrv: Error writing to AUDIO socket");
            return -1;
        }
        if (audio_input_channels == 2) {
            /* stereo - send twice the amount of data */
            res = vice_network_send(ffmpeg_audio_socket, ffmpegexedrv_audio_in.buffer, len, 0 /* flags */);
            if (res != len) {
                log_error(ffmpeg_log, "ffmpegexedrv: Error writing to AUDIO socket");
                return -1;
            }
        }
    }
    return 0;
}

#ifdef VICE_IS_SERVER
/* Try to find two ports that we can use. This doesn't work as expected :/ */
static void find_ports(void)
{
    int port;
    for (port = 49152; port < 65535; port++) {
        vice_network_socket_address_t *ad = NULL;
        vice_network_socket_t *s = NULL;
        ad = vice_network_address_generate("127.0.0.1", port);
        if (!ad) {
            log_error(ffmpeg_log, "Bad device name (port:%d).\n", port);
        }
        /* connect socket */
        s = vice_network_server(ad);
        if (!s) {
            log_error(ffmpeg_log, "Bad port number (port:%d).\n", port);
        } else {
            /*log_error(ffmpeg_log, "Good port number (port:%d).\n", port);*/
            vice_network_socket_close(s);
        }
        if (ad) {
            vice_network_address_close(ad);
        }
    }
}
#endif

static int test_ffmpeg_executable(void)
{
#if 0
    int ret;
    char *argv[5];
    /* `exec*()' does not want these to be constant...  */
    argv[0] = lib_strdup("ffmpeg");
    argv[1] = lib_strdup("-hide_banner");
    argv[2] = lib_strdup("-loglevel");
    argv[3] = lib_strdup("quiet");
    argv[4] = NULL;

    ret = archdep_spawn("ffmpeg", argv, NULL, NULL);

    lib_free(argv[0]);
    lib_free(argv[1]);
    lib_free(argv[2]);
    lib_free(argv[3]);

    /* NOTE: ffmpeg returns 1 (not 0) on success */
    if (ret != 1) {
        log_error(ffmpeg_log, "ffmpeg executable can not be started. (ret:%d)", ret);
        return -1;
    }
    return 0;
#else
    static int test_stdin = -1;
    static int test_stdout = -1;
    static vice_pid_t test_pid = VICE_PID_INVALID;
    int res = -1;
    size_t n;
    static char output[0x40];
    static char command[0x40] = {
        "ffmpeg 2>&1"   /* redirect stderr to stdout. this better works on the big 3 */
    };
    /* kill old process in case it is still running for whatever reason */
    if (test_pid != VICE_PID_INVALID) {
        kill_coproc(test_pid);
        test_pid = VICE_PID_INVALID;
    }
    if (test_stdin != -1) {
        close(test_stdin);
        test_stdin = -1;
    }
    if (test_stdout != -1) {
        close(test_stdout);
        test_stdout = -1;
    }
    DBG(("test_ffmpeg_executable: '%s'", command));
    /*log_printf("fork command:%s", command);*/
    if (fork_coproc(&test_stdin, &test_stdout, command, &test_pid) < 0) {
        log_error(ffmpeg_log, "Cannot fork ffmpeg process '%s'.", command);
        goto testend;
    }
    /*log_printf("test_ffmpeg_executable pid:%d stdin:%d stdout:%d", test_pid, test_stdin, test_stdout);*/
#ifdef WINDOWS_COMPILE
    if (test_pid == VICE_PID_INVALID) {
        log_error(ffmpeg_log, "Cannot fork ffmpeg process '%s' (pid == NULL).", command);
#else
    if (test_pid <= 0) {
        log_error(ffmpeg_log, "Cannot fork ffmpeg process '%s' (pid <= 0).", command);
#endif
        goto testend;
    }

    /* FIXME: stdout is 0 on error ? */
    if (test_stdout) {
        memset(output, 0, 0x40);
        n = read(test_stdout, output, 0x3f);
        /*log_printf("test_ffmpeg_executable got: '%s'", output);*/
        output[6] = 0;
        if ((n >= 6) && !strcmp("ffmpeg", output)) {
            res = 0;
            /*log_printf("test_ffmpeg_executable tested ok");*/
        }
    }
    if (res < 0) {
        log_error(ffmpeg_log, "ffmpeg not found.\n");
    }
testend:
    /* kill new process */
    if (test_pid != VICE_PID_INVALID) {
        kill_coproc(test_pid);
        test_pid = VICE_PID_INVALID;
    }
    if (test_stdin != -1) {
        close(test_stdin);
        test_stdin = -1;
    }
    if (test_stdout != -1) {
        close(test_stdout);
        test_stdout = -1;
    }
    return res;
#endif
}

static int start_ffmpeg_executable(void)
{
    char fpsstring[0x20];
    char *dot;
    static char command[0x400];
    static char tempcommand[0x400];
    int n;
    int audio_connected = 0;
    int video_connected = 0;

    log_resource_values(__FUNCTION__);

    /* FPS of the input, including "half framerate" */
    sprintf(fpsstring, "%f", fps);
    dot = strchr(fpsstring,',');
    if (dot) {
        *dot = '.';
    }

    strcpy(command,
            "ffmpeg "
            "-nostdin "
            /* Caution: at least on windows we must avoid that the ffmpeg
               executable produces output on stdout - if it does, the process
               may block and wait for ffmpeg_stderr being read */
            "-hide_banner "
#ifdef DEBUG_FFMPEG
            /* "-loglevel trace " */
            "-loglevel verbose "
            /* "-loglevel error " */
#else
            "-loglevel quiet "
#endif
    );

    /* options to define the input video format */
    if ((video_has_codec > 0) && (video_codec != AV_CODEC_ID_NONE)) {
        sprintf(tempcommand,
                "-f rawvideo "
                "-pixel_format rgb24 "
                "-framerate %s "              /* exact fps */
                "-r %s "              /* exact fps */
                "-s %dx%d "                         /* size */
                /*"-readrate_initial_burst 0 "*/        /* no initial burst read */
                /*"-readrate 1 "*/                      /* Read input at native frame rate */
                "-thread_queue_size 512 "
#ifdef VICE_IS_SERVER
                "-i tcp://127.0.0.1:%d "
#else
                "-i tcp://127.0.0.1:%d?listen "
#endif
                , fpsstring, fpsstring
                , video_width, video_height
                , current_video_port
        );
        strcat(command, tempcommand);
    }

    /* options to define the input audio format */
    if ((audio_has_codec > 0) && (audio_codec != AV_CODEC_ID_NONE)) {
            sprintf(tempcommand,
                    "-f s16le "                     /* input audio stream format */
                    "-acodec pcm_s16le "            /* audio codec */
                    "-ac %d "                       /* input audio channels */
                    "-ar %d "                       /* input audio stream sample rate */
                    "-ss %d "                       /* skip seconds at start */
                    "-thread_queue_size 512 "
#ifdef VICE_IS_SERVER
                    "-i tcp://127.0.0.1:%d "
#else
                    "-i tcp://127.0.0.1:%d?listen "
#endif
                    , audio_input_channels
                    , audio_input_sample_rate
                    , AUDIO_SKIP_SECONDS
                    , current_audio_port
            );
            strcat(command, tempcommand);
    }

    /* options for the output file */
    sprintf(tempcommand,
            "-y "           /* overwrite existing file */
            "-f %s "        /* outfile format/container */
            "-shortest "    /* Finish encoding when the shortest output stream ends. */
            /*"-shortest_buf_duration 1 "*/ /* the maximum duration of buffered frames in seconds */
            , ffmpegexe_format                      /* outfile format/container */
    );
    strcat(command, tempcommand);
    /* options for the output file (video) */
    if ((video_has_codec > 0) && (video_codec != AV_CODEC_ID_NONE)) {
        sprintf(tempcommand,
                "-framerate %s "              /* exact fps */
                "-r %s "              /* exact fps */
                "-vcodec %s "   /* outfile video codec */
                "-b:v %d "      /* outfile video bitrate */
                , fpsstring, fpsstring
                , av_codec_get_option(video_codec)     /* outfile video codec */
                , video_bitrate               /* outfile video bitrate */
        );
        strcat(command, tempcommand);
    }
    /* options for the output file (audio) */
    if ((audio_has_codec > 0) && (audio_codec != AV_CODEC_ID_NONE)) {
        sprintf(tempcommand,
                "-acodec %s "   /* outfile audio codec */
                "-b:a %d "      /* outfile audio bitrate */
                , av_codec_get_option(audio_codec)     /* outfile audio codec */
                , audio_bitrate               /* outfile audio bitrate */
        );
        strcat(command, tempcommand);
    }

    /* last not least the output file name */
    strcat(command, outfilename ? outfilename : "outfile.avi");

#ifndef VICE_IS_SERVER
    /* kill old process in case it is still running for whatever reason */
    if (ffmpeg_pid != 0) {
        kill_coproc(ffmpeg_pid);
        ffmpeg_pid = 0;
    }

    /*DBG(("forking ffmpeg: '%s'", command));*/
    if (fork_coproc(&ffmpeg_stdin, &ffmpeg_stdout, command, &ffmpeg_pid) < 0) {
        log_error(ffmpeg_log, "Cannot fork process '%s'.", command);
        return -1;
    }
#endif

    if ((video_has_codec > 0) && (video_codec != AV_CODEC_ID_NONE)) {
        vice_network_socket_address_t *ad = NULL;
        ad = vice_network_address_generate("127.0.0.1", current_video_port);
        if (!ad) {
            log_error(ffmpeg_log, "Bad device name.\n");
            return -1;
        }
        /* connect socket */
        for (n = 0; n < 200; n++) {
#ifdef VICE_IS_SERVER
            ffmpeg_video_listen_socket = vice_network_server(ad);
            if (!ffmpeg_video_listen_socket) {
#else
            ffmpeg_video_socket = vice_network_client(ad);
            if (!ffmpeg_video_socket) {
#endif
                /*log_error(ffmpeg_log, "ffmpegexedrv: Error connecting AUDIO socket");*/
                archdep_usleep(1000);
            } else {
                log_message(ffmpeg_log, "ffmpegexedrv: VIDEO connected");
                video_connected = 1;
                break;
            }
        }
        if (!video_connected) {
            log_error(ffmpeg_log, "ffmpegexedrv: Error connecting VIDEO socket");
            return -1;
        }
    }

#ifndef VICE_IS_SERVER
    if (write_initial_video_frames() < 0) {
        return -1;
    }
#endif

    if ((audio_has_codec > 0) && (audio_codec != AV_CODEC_ID_NONE)) {
        vice_network_socket_address_t *ad = NULL;
        ad = vice_network_address_generate("127.0.0.1", current_audio_port);
        if (!ad) {
            log_error(ffmpeg_log, "Bad device name.\n");
            return -1;
        }
        /* connect socket */
        for (n = 0; n < 200; n++) {
#ifdef VICE_IS_SERVER
            ffmpeg_audio_listen_socket = vice_network_server(ad);
            if (!ffmpeg_audio_listen_socket) {
#else
            ffmpeg_audio_socket = vice_network_client(ad);
            if (!ffmpeg_audio_socket) {
#endif
                /*log_error(ffmpeg_log, "ffmpegexedrv: Error connecting AUDIO socket");*/
                archdep_usleep(1000);
            } else {
                log_message(ffmpeg_log, "ffmpegexedrv: AUDIO connected");
                audio_connected = 1;
                break;
            }
        }
        if (!audio_connected) {
            log_error(ffmpeg_log, "ffmpegexedrv: Error connecting AUDIO socket");
            return -1;
        }
    }

#ifndef VICE_IS_SERVER
    if (write_initial_audio_frames() < 0) {
        return -1;
    }
#endif

#ifdef VICE_IS_SERVER
    /* kill old process in case it is still running for whatever reason */
    if (ffmpeg_pid != 0) {
        kill_coproc(ffmpeg_pid);
        ffmpeg_pid = 0;
    }
    /*DBG(("forking ffmpeg: '%s'", command));*/
    if (fork_coproc(&ffmpeg_stdin, &ffmpeg_stdout, command, &ffmpeg_pid) < 0) {
        log_error(ffmpeg_log, "Cannot fork process '%s'.", command);
        return -1;
    }

    do {
        int audio_available = 0;
        int video_available = 0;

        /*archdep_sleep(1);*/
        if (ffmpeg_audio_socket != NULL) {
            audio_available = vice_network_select_poll_one(ffmpeg_audio_socket);
            DBG(("ffmpeg_audio_socket available: %d", audio_available));
        } else if (ffmpeg_audio_listen_socket != NULL) {
            /* we have no connection yet, allow for connection */

            if (vice_network_select_poll_one(ffmpeg_audio_listen_socket)) {
                ffmpeg_audio_socket = vice_network_accept(ffmpeg_audio_listen_socket);
                write_initial_audio_frames();
            }
            DBG(("ffmpeg_audio_socket connected: %p", ffmpeg_audio_socket));
        }
        if (ffmpeg_video_socket != NULL) {
            video_available = vice_network_select_poll_one(ffmpeg_video_socket);
            DBG(("ffmpeg_video_socket available: %d", video_available));
        } else if (ffmpeg_video_listen_socket != NULL) {
            /* we have no connection yet, allow for connection */

            if (vice_network_select_poll_one(ffmpeg_video_listen_socket)) {
                ffmpeg_video_socket = vice_network_accept(ffmpeg_video_listen_socket);
                if (write_initial_video_frames() < 0) {
                    return -1;
                }
            }
            DBG(("ffmpeg_video_socket connected: %p", ffmpeg_video_socket));
        }
    } while ((ffmpeg_audio_socket == NULL) || (ffmpeg_video_socket == NULL));
#endif

    log_message(ffmpeg_log, "ffmpegexedrv: pipes are ready");
    return 0;
}

static void close_video_stream(void)
{
    if (ffmpeg_video_socket != NULL) {
        vice_network_socket_close(ffmpeg_video_socket);
        ffmpeg_video_socket = NULL;
    }
}

static void close_audio_stream(void)
{
    if (ffmpeg_audio_socket != NULL) {
        vice_network_socket_close(ffmpeg_audio_socket);
        ffmpeg_audio_socket = NULL;
    }
}

static void close_stream(void)
{
    if (ffmpeg_video_socket != NULL) {
        vice_network_socket_close(ffmpeg_video_socket);
        ffmpeg_video_socket = NULL;
    }
    if (ffmpeg_audio_socket != NULL) {
        vice_network_socket_close(ffmpeg_audio_socket);
        ffmpeg_audio_socket = NULL;
    }
#ifdef VICE_IS_SERVER
    if (ffmpeg_video_listen_socket != NULL) {
        vice_network_socket_close(ffmpeg_video_listen_socket);
        ffmpeg_video_listen_socket = NULL;
    }
    if (ffmpeg_audio_listen_socket != NULL) {
        vice_network_socket_close(ffmpeg_audio_listen_socket);
        ffmpeg_audio_listen_socket = NULL;
    }
#endif
    if (ffmpeg_stdin != -1) {
        close(ffmpeg_stdin);
        ffmpeg_stdin = -1;
    }
    if (ffmpeg_stdout != -1) {
        close(ffmpeg_stdout);
        ffmpeg_stdout = -1;
    }
    /* do not kill ffmpeg here, it should die when the streams close. if it is
       killed early the resulting file will be broken */
#if 0
    if (ffmpeg_pid != 0) {
        kill_coproc(ffmpeg_pid);
        ffmpeg_pid = 0;
    }
#endif

    prepare_port_numbers();
}

/*****************************************************************************
   audio stream encoding
 *****************************************************************************/

/* called by ffmpegexedrv_init_video -> ffmpegexedrv_init_file */
static int ffmpegexedrv_open_audio(void)
{
    size_t audio_inbuf_size;
    DBG(("ffmpegexedrv_open_audio (%d,%d)", audio_input_channels, AUDIO_BUFFER_MAX_CHANNELS));
    /*assert((audio_input_channels > 0));*/
    /*if (audio_input_channels < 1) {
        log_warning(ffmpeg_log, "ffmpegexedrv_open_audio audio_input_channels < 1 (%d,%d)", audio_input_channels, AUDIO_BUFFER_MAX_CHANNELS);
    }*/

    audio_is_open = 1;

    /* FIXME: audio_input_channels NOT ready yet */
    ffmpegexedrv_audio_in.size = AUDIO_BUFFER_SAMPLES;
    ffmpegexedrv_audio_in.used = 0;
    /* audio_inbuf_size = AUDIO_BUFFER_SAMPLES * sizeof(int16_t) * audio_input_channels; */
    audio_inbuf_size = AUDIO_BUFFER_SAMPLES * sizeof(int16_t) * AUDIO_BUFFER_MAX_CHANNELS;

    ffmpegexedrv_audio_in.buffer = lib_malloc(audio_inbuf_size);
    if (ffmpegexedrv_audio_in.buffer == NULL) {
        log_error(ffmpeg_log, "ffmpegexedrv: Error allocating audio buffer (%u bytes)", (unsigned)audio_inbuf_size);
        return -1;
    }
    return 0;
}

static void ffmpegexedrv_close_audio(void)
{
    DBG(("ffmpegexedrv_close_audio"));

    close_audio_stream();

    audio_input_sample_rate = -1;
    audio_is_open = 0;
    audio_input_counter = 0;

    if (ffmpegexedrv_audio_in.buffer) {
        lib_free(ffmpegexedrv_audio_in.buffer);
    }
    ffmpegexedrv_audio_in.buffer = NULL;
    ffmpegexedrv_audio_in.size = 0;
}

/* Soundmovie API soundmovie_funcs_t.init */
/* called via ffmpegexedrv_soundmovie_funcs->init */
static int ffmpegexe_soundmovie_init(int speed, int channels, soundmovie_buffer_t ** audio_in)
{
    DBG(("ffmpegexe_soundmovie_init(speed: %d channels: %d)", speed, channels));

    audio_init_done = 1;

    *audio_in = &ffmpegexedrv_audio_in;
    (*audio_in)->used = 0;

    audio_input_sample_rate = speed;
    audio_input_channels = channels;

#if 0
    if (video_init_done) {
        ffmpegexedrv_init_file();
    }
#endif

    return start_ffmpeg_executable();
}

/* Soundmovie API soundmovie_funcs_t.encode */
/* called via ffmpegexedrv_soundmovie_funcs->encode */
/* triggered by soundffmpegaudio->write */
static int ffmpegexe_soundmovie_encode(soundmovie_buffer_t *audio_in)
{
    ssize_t res;
#ifdef DEBUG_FFMPEG_FRAMES
    double frametime = (double)framecounter / fps;
    double audiotime = (double)audio_input_counter / (double)audio_input_sample_rate;
    if (audio_in) {
        DBGFRAMES(("ffmpegexe_soundmovie_encode(framecount:%lu, audiocount:%lu frametime:%f, audiotime:%f size:%d used:%d)",
            framecounter, audio_input_counter, frametime, audiotime, audio_in->size, audio_in->used));
    } else {
        DBG(("ffmpegexe_soundmovie_encode (NULL)"));
    }
#endif

    if (ffmpeg_audio_socket == 0) {
        log_error(ffmpeg_log, "FFMPEG: ffmpegexe_soundmovie_encode ffmpeg_audio_socket is 0 (framecount:%"PRIu64")", audio_input_counter);
        return 0;
    }

    if ((audio_has_codec > 0) && (audio_codec != AV_CODEC_ID_NONE)) {
        /* FIXME: we might have an endianess problem here, we might have to swap lo/hi on BE machines */
        if (audio_input_channels == 1) {
            res = vice_network_send(ffmpeg_audio_socket, &audio_in->buffer[0], audio_in->used * 2, 0 /* flags */);
            if (res != audio_in->used * 2) {
                return -1;
            }
            audio_input_counter += audio_in->used;
        } else if (audio_input_channels == 2) {
            res = vice_network_send(ffmpeg_audio_socket, &audio_in->buffer[0], audio_in->used * 2, 0 /* flags */);
            if (res != audio_in->used * 2) {
                return -1;
            }
            audio_input_counter += audio_in->used / 2;
        } else {
            return -1;
        }
    }

    audio_in->used = 0;
    return 0;
}

/* Soundmovie API soundmovie_funcs_t.close */
/* called via ffmpegexedrv_soundmovie_funcs->close */
static void ffmpegexe_soundmovie_close(void)
{
    DBG(("ffmpegexe_soundmovie_close"));
    /* just stop the whole recording */
    screenshot_stop_recording();
}

static soundmovie_funcs_t ffmpegexedrv_soundmovie_funcs = {
    ffmpegexe_soundmovie_init,
    ffmpegexe_soundmovie_encode,
    ffmpegexe_soundmovie_close
};

/*****************************************************************************
   video stream encoding
 *****************************************************************************/

static int video_fill_rgb_image(screenshot_t *screenshot, VIDEOFrame *pic)
{
    int x, y;
    int dx, dy;
    int colnum;
    int bufferoffset;
    int x_dim = screenshot->width;
    int y_dim = screenshot->height;
    int pix = 0;
    /* center the screenshot in the video */
    pic->linesize = video_width * INPUT_VIDEO_BPP;
    dx = (video_width - x_dim) / 2;
    dy = (video_height - y_dim) / 2;
    bufferoffset = screenshot->x_offset + (dx < 0 ? -dx : 0)
        + (screenshot->y_offset + (dy < 0 ? -dy : 0)) * screenshot->draw_buffer_line_size;

    for (y = 0; y < video_height; y++) {
        for (x = 0; x < video_width; x++) {
            colnum = screenshot->draw_buffer[bufferoffset + x];
            pic->data[pix + INPUT_VIDEO_BPP * x] = screenshot->palette->entries[colnum].red;
            pic->data[pix + INPUT_VIDEO_BPP * x + 1] = screenshot->palette->entries[colnum].green;
            pic->data[pix + INPUT_VIDEO_BPP * x + 2] = screenshot->palette->entries[colnum].blue;
        }
        bufferoffset += screenshot->draw_buffer_line_size;
        pix += pic->linesize;
    }

    return 0;
}

/* called by ffmpegexedrv_open_video() */
static VIDEOFrame* video_alloc_picture(int bpp, int width, int height)
{
    VIDEOFrame *picture;

    picture = lib_malloc(sizeof(VIDEOFrame));
    if (!picture) {
        return NULL;
    }
    picture->data = lib_malloc(bpp * width * height);
    if (!picture->data) {
        lib_free(picture);
        log_debug(ffmpeg_log, "ffmpegexedrv: Could not allocate frame data");
        return NULL;
    }

    picture->linesize = width;
    return picture;
}

static void video_free_picture(VIDEOFrame *picture)
{
    lib_free(picture->data);
    lib_free(picture);
}

/* called by ffmpegexedrv_init_file */
static int ffmpegexedrv_open_video(void)
{
    DBG(("ffmpegexedrv_open_video w:%d h:%d", video_width, video_height));

    video_is_open = 1;

    /* allocate the encoded raw picture */
    video_st_frame = video_alloc_picture(INPUT_VIDEO_BPP, video_width, video_height);
    if (!video_st_frame) {
        log_debug(ffmpeg_log, "ffmpegexedrv: could not allocate picture");
        return -1;
    }

    return 0;
}

/* called by ffmpegexedrv_close() */
static void ffmpegexedrv_close_video(void)
{
    DBG(("ffmpegexedrv_close_video"));
    close_video_stream();
    video_is_open = 0;
    if (video_st_frame) {
        video_free_picture(video_st_frame);
        video_st_frame = NULL;
    }
    framecounter = 0;
}

/* called by ffmpegexedrv_save */
static void ffmpegexedrv_init_video(screenshot_t *screenshot)
{
    DBG(("ffmpegexedrv_init_video"));
    video_init_done = 1;

    /* resolution should be a multiple of 16 */
    /* video_fill_rgb_image only implements cutting so */
    /* adding black border was removed */
    video_width = screenshot->width & ~0xf;
    video_height = screenshot->height & ~0xf;
    /* frames per second */
    log_resource_values(__FUNCTION__);

    DBG(("ffmpegexedrv_init_video w:%d h:%d (halve framerate:%d)",
           video_width, video_height, video_halve_framerate));
    time_base = machine_get_cycles_per_frame()
                                    / (double)(video_halve_framerate ?
                                        machine_get_cycles_per_second() / 2 :
                                        machine_get_cycles_per_second());
    fps = 1.0f / time_base;
    DBG(("ffmpegexedrv_init_video fps: %f timebase: %f", fps, time_base));
    framecounter = 0;

    ffmpegexedrv_init_file();
}


/*****************************************************************************/

/* called by ffmpegexedrv_init_video */
static int ffmpegexedrv_init_file(void)
{
    DBG(("ffmpegexedrv_init_file"));
#if 0
    if (!video_init_done || !audio_init_done) {
        return 0;
    }
#endif

    if (ffmpegexedrv_open_video() < 0) {
        screenshot_stop_recording();
        ui_error("ffmpegexedrv: Cannot open video stream");
        return -1;
    }

    if (ffmpegexedrv_open_audio() < 0) {
        screenshot_stop_recording();
        ui_error("ffmpegexedrv: Cannot open audio stream");
        return -1;
    }

    log_debug(ffmpeg_log, "ffmpegexedrv: Initialized file successfully");

    /*start_ffmpeg_executable();*/

    file_init_done = 1;

    return 0;
}

/* Driver API gfxoutputdrv_t.save */
/* called once to start recording video+audio */
static int ffmpegexedrv_save(screenshot_t *screenshot, const char *filename)
{
    gfxoutputdrv_format_t *format;

    DBG(("ffmpegexedrv_save(%s)", filename));

    audio_init_done = 0;
    video_init_done = 0;
    file_init_done = 0;

    if (test_ffmpeg_executable() < 0) {
        screenshot_stop_recording();
        archdep_sleep(1);
        if (test_ffmpeg_executable() < 0) {
            archdep_sleep(1);
            if (test_ffmpeg_executable() < 0) {
                ui_error("ffmpeg executable could not be started.");
                /* Do not return -1, since that would just pop up a second error message,
                which will eventually appear behind the main window, and make the UI
                seem to hang. We can do this, since there is no further error handling
                depending on the return value. */
                return 0;
            }
        }
    }

    log_resource_values(__FUNCTION__);

    DBG(("FFMPEGFormat:%s (format_index:%d)", ffmpegexe_format, format_index));

    if (format_index < 0) {
        return -1;
    }

    format = &ffmpegexedrv_formatlist[format_index];

    audio_has_codec = (format->audio_codecs != NULL);
    /* the codec from resource */
    DBG(("ffmpegexedrv_save(audio_has_codec: %d audio_codec:%d:%s)",
            audio_has_codec, audio_codec, av_codec_get_option(audio_codec)));

    video_has_codec = (format->video_codecs != NULL);
    /* the codec from resource */
    DBG(("ffmpegexedrv_save(video_has_codec: %d video_codec:%d:%s)",
            video_has_codec, video_codec, av_codec_get_option(video_codec)));

    outfilename = lib_strdup(filename);

    ffmpegexedrv_init_video(screenshot);

    soundmovie_start(&ffmpegexedrv_soundmovie_funcs);

    return 0;
}

/* Driver API gfxoutputdrv_t.close */
static int ffmpegexedrv_close(screenshot_t *screenshot)
{
    DBG(("ffmpegexedrv_close"));

    soundmovie_stop();

    ffmpegexedrv_close_video();
    ffmpegexedrv_close_audio();

    /* free the streams */
    close_stream();

    log_debug(ffmpeg_log, "ffmpegexedrv: Closed successfully");

    file_init_done = 0;

    if (outfilename) {
        lib_free(outfilename);
        outfilename = NULL;
    }

    return 0;
}

/* Driver API gfxoutputdrv_t.record */
/* triggered by screenshot_record, periodically called to output video data stream */
static int ffmpegexedrv_record(screenshot_t *screenshot)
{
    double frametime = (double)framecounter / fps;
    double audiotime = (double)audio_input_counter / (double)audio_input_sample_rate;
    DBGFRAMES(("ffmpegexedrv_record(framecount:%lu, audiocount:%lu frametime:%f, audiotime:%f)",
        framecounter, audio_input_counter, frametime, audiotime));
    /* log_resource_values(__FUNCTION__); */

    framecounter++;

    if (video_halve_framerate && (framecounter & 1)) {
        /* drop every second frame */
        return 0;
    }

    if (video_halve_framerate) {
        frametime /= 2;
    }

    /* the video is ahead */
    if (frametime > (audiotime + (time_base * 1.5f))) {
        /* drop one frame */
        framecounter--;
        DBG(("video is ahead, dropping a frame (framecount:%lu, audiocount:%lu frametime:%f, audiotime:%f)",
            framecounter, audio_input_counter, frametime, audiotime));
        return 0;
    }

    /*DBGFRAMES(("ffmpegexedrv_record (%u)", framecounter));*/
    video_fill_rgb_image(screenshot, video_st_frame);

    if (write_video_frame(video_st_frame) < 0) {
        return -1;
    }

    /* the video is late */
    if (frametime < (audiotime - (time_base * 1.5f))) {
        /* insert one frame */
        framecounter++;
        DBG(("video is late, inserting a frame (framecount:%lu, audiocount:%lu frametime:%f, audiotime:%f)",
            framecounter, audio_input_counter, frametime, audiotime));
        if (write_video_frame(video_st_frame) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Driver API gfxoutputdrv_t.write */
static int ffmpegexedrv_write(screenshot_t *screenshot)
{
    DBG(("ffmpegexedrv_write"));
    return 0;
}

/******************************************************************************/

static gfxoutputdrv_t ffmpegexe_drv = {
    GFXOUTPUTDRV_TYPE_VIDEO,
    "FFMPEG",
    "FFMPEG (Executable)",
    NULL,
    NULL, /* filled in get_formats_and_codecs */
    NULL, /* open */
    ffmpegexedrv_close,
    ffmpegexedrv_write,
    ffmpegexedrv_save,
    NULL,
    ffmpegexedrv_record,
    ffmpegexedrv_shutdown,
    ffmpegexedrv_resources_init,
    ffmpegexedrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    , NULL
#endif
};

/* gfxoutputdrv_t.shutdown */
static void ffmpegexedrv_shutdown(void)
{
    int i = 0;

    DBG(("ffmpegexedrv_shutdown"));

    /* kill old process in case it is still running for whatever reason */
    if (ffmpeg_pid != 0) {
        kill_coproc(ffmpeg_pid);
        ffmpeg_pid = 0;
    }

    if (ffmpegexe_drv.formatlist != NULL) {

        while (ffmpegexe_drv.formatlist[i].name != NULL) {
            lib_free(ffmpegexe_drv.formatlist[i].name);
            if (ffmpegexe_drv.formatlist[i].audio_codecs != NULL) {
                lib_free(ffmpegexe_drv.formatlist[i].audio_codecs);
            }
            if (ffmpegexe_drv.formatlist[i].video_codecs != NULL) {
                lib_free(ffmpegexe_drv.formatlist[i].video_codecs);
            }
            i++;
        }
        if (ffmpegexe_drv.formatlist) {
            lib_free(ffmpegexe_drv.formatlist);
            ffmpegexe_drv.formatlist = NULL;
        }
    }
/* ??? this is actually ffmpegexe_drv.formatlist
    if (ffmpegexedrv_formatlist) {
        lib_free(ffmpegexedrv_formatlist);
    }
*/

    if (ffmpegexe_format) {
        lib_free(ffmpegexe_format);
        ffmpegexe_format = NULL;
    }
}

/* FIXME: This should interrogate the ffmpeg binary and list all available
          formats and codecs */
static void get_formats_and_codecs(void)
{
    int i, j, ai = 0, vi = 0, f;
    gfxoutputdrv_codec_t *audio_codec_list;
    gfxoutputdrv_codec_t *video_codec_list;
    gfxoutputdrv_codec_t *ac, *vc;

    f = 0;
    ffmpegexedrv_formatlist = lib_malloc(sizeof(gfxoutputdrv_format_t));

    for (i = 0; output_formats_to_test[i].name != NULL; i++) {
            audio_codec_list = NULL;
            video_codec_list = NULL;
            if (output_formats_to_test[i].audio_codecs != NULL) {
                ai = 0;
                audio_codec_list = lib_malloc(sizeof(gfxoutputdrv_codec_t));
                ac = output_formats_to_test[i].audio_codecs;
                for (j = 0; ac[j].name != NULL; j++) {
                    DBG(("audio_codec_list[%d].name='%s'\n", ai, ac[j].name));
                    audio_codec_list[ai++] = ac[j];
                    audio_codec_list = lib_realloc(audio_codec_list, (ai + 1) * sizeof(gfxoutputdrv_codec_t));
                }
                audio_codec_list[ai].name = NULL;
            }
            if (output_formats_to_test[i].video_codecs != NULL) {
                vi = 0;
                video_codec_list = lib_malloc(sizeof(gfxoutputdrv_codec_t));
                vc = output_formats_to_test[i].video_codecs;
                for (j = 0; vc[j].name != NULL; j++) {
                    DBG(("video_codec_list[%d].name='%s'", vi, output_formats_to_test[i].video_codecs[j].name));
                    video_codec_list[vi++] = output_formats_to_test[i].video_codecs[j];
                    video_codec_list = lib_realloc(video_codec_list, (vi + 1) * sizeof(gfxoutputdrv_codec_t));
                }
                video_codec_list[vi].name = NULL;
            }
            if (((audio_codec_list == NULL) || (ai > 0)) && ((video_codec_list == NULL) || (vi > 0))) {
                ffmpegexedrv_formatlist[f].flags = output_formats_to_test[i].flags;
                ffmpegexedrv_formatlist[f].name = lib_strdup(output_formats_to_test[i].name);
                DBG(("ffmpegexedrv_formatlist[%d].name='%s'", f, ffmpegexedrv_formatlist[f].name));
                ffmpegexedrv_formatlist[f].audio_codecs = audio_codec_list;
                ffmpegexedrv_formatlist[f++].video_codecs = video_codec_list;
                ffmpegexedrv_formatlist = lib_realloc(ffmpegexedrv_formatlist, (f + 1) * sizeof(gfxoutputdrv_format_t));
            }
    }
    ffmpegexedrv_formatlist[f].name = NULL;
    ffmpegexe_drv.formatlist = ffmpegexedrv_formatlist;
}

/* public, init this output driver */
void gfxoutput_init_ffmpegexe(int help)
{
    if (help) {
        gfxoutput_register(&ffmpegexe_drv);
        return;
    }

    get_formats_and_codecs();

    gfxoutput_register(&ffmpegexe_drv);
}
