/** \file   src/screenshot.c
 * \brief   Screenshot/video recording module
 *
 * screenshot.c - Create a screenshot.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andreas Matthies <andreas.matthies@gmx.net>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "cmdline.h"
#include "gfxoutput.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "uiapi.h"
#include "util.h"
#include "video.h"
#include "monitor.h"
#include "ui.h"
#include "vsync.h"

/* #define DEBUG_SCREENSHOT */

#ifdef DEBUG_SCREENSHOT
#define DBG(x) log_printf x
#else
#define DBG(x)
#endif

static log_t screenshot_log = LOG_DEFAULT;
static gfxoutputdrv_t *recording_driver;
static struct video_canvas_s *recording_canvas;

static int reopen = 0;
static char *reopen_recording_drivername;
static struct video_canvas_s *reopen_recording_canvas;
static char *reopen_filename;
static char *autosave_screenshot_format;


/** \brief  Initialize module
 *
 * \return  0 on success (always)
 */
int screenshot_init(void)
{
    /* Setup logging system.  */
    screenshot_log = log_open("Screenshot");
    recording_driver = NULL;
    recording_canvas = NULL;

    reopen_recording_drivername = NULL;
    reopen_filename = NULL;
    return 0;
}


/** \brief  Clean up memory resources used by the module
 */
void screenshot_shutdown(void)
{
    lib_free(reopen_recording_drivername);
    lib_free(reopen_filename);
    lib_free(autosave_screenshot_format);
}



/*-----------------------------------------------------------------------*/

static void screenshot_line_data(screenshot_t *screenshot, uint8_t *data,
                                 unsigned int line, unsigned int mode)
{
    unsigned int i;
    uint8_t *line_base;
    uint8_t color;

    if (line > screenshot->height) {
        log_error(screenshot_log, "Invalild line `%u' request.", line);
        return;
    }

#define BUFFER_LINE_START(i, n) ((i)->draw_buffer + (n) * (i)->draw_buffer_line_size)

    line_base = BUFFER_LINE_START(screenshot,
                                  (line + screenshot->y_offset)
                                  * screenshot->size_height);

    switch (mode) {
        case SCREENSHOT_MODE_PALETTE:
            for (i = 0; i < screenshot->width; i++) {
                data[i] = screenshot->color_map[line_base[i * screenshot->size_width + screenshot->x_offset]];
            }
            break;
        case SCREENSHOT_MODE_RGB32:
            for (i = 0; i < screenshot->width; i++) {
                color = screenshot->color_map[line_base[i * screenshot->size_width + screenshot->x_offset]];
                data[i * 4] = screenshot->palette->entries[color].red;
                data[i * 4 + 1] = screenshot->palette->entries[color].green;
                data[i * 4 + 2] = screenshot->palette->entries[color].blue;
                data[i * 4 + 3] = 0;
            }
            break;
        case SCREENSHOT_MODE_RGB24:
            for (i = 0; i < screenshot->width; i++) {
                color = screenshot->color_map[line_base[i * screenshot->size_width + screenshot->x_offset]];
                data[i * 3] = screenshot->palette->entries[color].red;
                data[i * 3 + 1] = screenshot->palette->entries[color].green;
                data[i * 3 + 2] = screenshot->palette->entries[color].blue;
            }
            break;
        default:
            log_error(screenshot_log, "Invalid mode %u.", mode);
    }
}

/*-----------------------------------------------------------------------*/
static int screenshot_save_core(screenshot_t *screenshot, gfxoutputdrv_t *drv,
                                const char *filename)
{
    unsigned int i;

    screenshot->width = screenshot->max_width & ~3;
    screenshot->height = screenshot->last_displayed_line - screenshot->first_displayed_line + 1;
    screenshot->y_offset = screenshot->first_displayed_line;

    screenshot->color_map = lib_calloc(1, 256);

    for (i = 0; i < screenshot->palette->num_entries; i++) {
        screenshot->color_map[i] = i;
    }

    screenshot->convert_line = screenshot_line_data;

    if (drv != NULL) {
        if (drv->save_native != NULL) {
            /* It's a native screenshot. */
            if ((drv->save_native)(screenshot, filename) < 0) {
                log_error(screenshot_log, "Saving failed...");
                lib_free(screenshot->color_map);
                return -1;
            }
        } else {
            /* It's a usual screenshot. */
            if ((drv->save)(screenshot, filename) < 0) {
                log_error(screenshot_log, "Saving failed...");
                lib_free(screenshot->color_map);
                return -1;
            }
        }
    } else {
        /* We're recording a movie */
        if (vsync_get_warp_mode() == 0) {
            /* skip recording when warpmode is active, this doesn't really work -
               and unless we significantly change what warpmode does can not work */
            if ((recording_driver->record)(screenshot) < 0) {
                log_error(screenshot_log, "Recording failed...");
                lib_free(screenshot->color_map);
                return -1;
            }
        }
    }

    lib_free(screenshot->color_map);
    return 0;
}

/*-----------------------------------------------------------------------*/

int screenshot_save(const char *drvname, const char *filename,
                    struct video_canvas_s *canvas)
{
    screenshot_t screenshot;
    gfxoutputdrv_t *drv;
    int result;

    DBG(("screenshot_save(%s, %s, ...)", drvname, filename));

    if ((drv = gfxoutput_get_driver(drvname)) == NULL) {
        return -1;
    }

    if (recording_driver == drv) {
        ui_error("Sorry. Multiple recording is not supported.");
        return -1;
    }

    if (machine_screenshot(&screenshot, canvas) < 0) {
        log_error(screenshot_log, "Retrieving screen geometry failed.");
        return -1;
    }

    if (drv->record != NULL) {
        recording_driver = drv;
        recording_canvas = canvas;

        reopen_recording_drivername = lib_strdup(drvname);
        reopen_recording_canvas = canvas;
        reopen_filename = lib_strdup(filename);
    }

    result = screenshot_save_core(&screenshot, drv, filename);
    DBG(("screenshot_save_core result:%d", result));
    if (result < 0) {
        recording_driver = NULL;
        recording_canvas = NULL;
    }

    return result;
}

#ifdef FEATURE_CPUMEMHISTORY
int memmap_screenshot_save(const char *drvname, const char *filename, int x_size, int y_size, uint8_t *gfx, uint8_t *palette)
{
    gfxoutputdrv_t *drv;

    if ((drv = gfxoutput_get_driver(drvname)) == NULL) {
        return -1;
    }

    if ((drv->savememmap)(filename, x_size, y_size, gfx, palette) < 0) {
        log_error(screenshot_log, "Saving failed...");
        return -1;
    }
    return 0;
}
#endif

/* called for each frame */
int screenshot_record(void)
{
    screenshot_t screenshot;
    int result;

    if (recording_driver == NULL) {
        return 0;
    }

    /* DBG(("screenshot_record")); */

    /* Retrive framebuffer and screen geometry.  */
    if (recording_canvas != NULL) {
        if (machine_screenshot(&screenshot, recording_canvas) < 0) {
            log_error(screenshot_log, "Retrieving screen geometry failed.");
            return -1;
        }
    } else {
        /* should not happen */
        log_error(screenshot_log, "Canvas is unknown.");
        return -1;
    }

    result = screenshot_save_core(&screenshot, NULL, NULL);
    DBG(("screenshot_record result:%d", result));
    if (result < 0) {
        /*log_error(screenshot_log, "Video recording failed, stopping...");*/
        screenshot_stop_recording();
        ui_display_recording(UI_RECORDING_STATUS_NONE);
    }

    return result;
}

void screenshot_stop_recording(void)
{
    if (recording_driver != NULL && recording_driver->close != NULL) {
        recording_driver->close(NULL);
    }

    recording_driver = NULL;
    recording_canvas = NULL;
}

int screenshot_is_recording(void)
{
    return (recording_driver == NULL ? 0 : 1);
}

void screenshot_prepare_reopen(void)
{
    reopen = (screenshot_is_recording() ? 1 : 0);
}

void screenshot_try_reopen(void)
{
    if (reopen == 1) {
        screenshot_save(reopen_recording_drivername,
                        reopen_filename,
                        reopen_recording_canvas);
    }
    reopen = 0;
}

const char *screenshot_get_fext_for_format(const char *format)
{
    gfxoutputdrv_t *driver = gfxoutput_get_driver(format);
    return driver != NULL ? driver->default_extension : NULL;
}

const char *screenshot_get_quickscreenshot_format(void)
{
    const char *format;

    if (resources_get_string("QuicksaveScreenshotFormat", &format) ||
        format == NULL) {
        log_error(LOG_DEFAULT, "Quicksave screenshot format resource not set in configuration?");
        return NULL;
    }
    return format;
}

/* Create a string in the format 'yyyymmddHHMMssff' of the current time
 * Returns pointer to string if successful, NULL if an error occured.
 */
char *screenshot_create_datetime_string(void)
{
    time_t stamp = time(NULL);
    struct tm *stamp_tm = localtime(&stamp);
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
#else
    static unsigned int count = 0;
#endif
    char *result, buf[40];

    if (stamp_tm == NULL ||
        strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", stamp_tm) == 0) {
        log_error(LOG_DEFAULT, "Could not generate autosave screenshot timestamp!");
        return NULL;
    }

#ifdef HAVE_GETTIMEOFDAY
    if (gettimeofday(&tv, NULL) < 0 ||
        (result = lib_msprintf("%s%02ld", buf, (tv.tv_usec / 10000))) == NULL) {
        log_error(LOG_DEFAULT, "Could not generate autosave screenshot microsecond timestamp!");
        return NULL;
    }
#else
    /* TODO: add support for time functions? */
    if ((result = lib_msprintf("%s-%u", buf, count)) == NULL) {
        log_error(LOG_DEFAULT, "Could not generate autosave screenshot file name!");
        return NULL;
    }
    count++;
#endif
    return result;
}

char *screenshot_create_quickscreenshot_filename(const char *format)
{
    char *date, *result;
    const char *fext;

    if ((fext = screenshot_get_fext_for_format(format)) == NULL) {
        log_error(LOG_DEFAULT, "Invalid or unsupported autosave screenshot format '%s'!", format);
        return NULL;
    }
    if ((date = screenshot_create_datetime_string()) == NULL) {
        return NULL;
    }
    result = lib_msprintf("vice-screen-%s.%s", date, fext);
    lib_free(date);

    return result;
}


static int set_autosave_screenshot_format(const char *val, void *param)
{
    if (!val || val[0] == '\0' ||
        screenshot_get_fext_for_format(val) == NULL) {
        log_error(LOG_DEFAULT, "Invalid or unset autosave screenshot format, defaulting to %s",
            SCREENSHOT_DEFAULT_QUICKSCREENSHOT_FORMAT);
        util_string_set(&autosave_screenshot_format, SCREENSHOT_DEFAULT_QUICKSCREENSHOT_FORMAT);
    } else {
        util_string_set(&autosave_screenshot_format, val);
    }
    return 0;
}


static resource_string_t resources_string[] = {
    { "QuicksaveScreenshotFormat", SCREENSHOT_DEFAULT_QUICKSCREENSHOT_FORMAT, RES_EVENT_NO, NULL,
      &autosave_screenshot_format, set_autosave_screenshot_format, NULL },
    RESOURCE_STRING_LIST_END
};

static int quicksave_set_func(const char *value, void *extra_param)
{
    char *s;
    int i = 0;;
    if (value == NULL) {
        return -1;
    }
    s = lib_strdup(value);
    while(s[i] != 0) {
        s[i] = toupper(s[i]);
        i++;
    }
    i = resources_set_string("QuicksaveScreenshotFormat", s);
    lib_free(s);
    return i;
}

static cmdline_option_t cmdline_options[] =
{
    { "-quicksaveformat", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      quicksave_set_func, NULL, "QuicksaveScreenshotFormat", NULL,
      "<Format>",
    /* KLUDGES: this should really be generated at runtime */
    "Specify format of quicksave screenshots ("
#ifdef HAVE_PNG
    "png, "
#endif
#ifdef HAVE_GIF
    "gif ,"
#endif
    "bmp, iff, pcx, ppm, 4bt, artstudio, koala, minipaint)"
    },
    CMDLINE_LIST_END
};

int screenshot_resources_init(void)
{
    return resources_register_string(resources_string);
}

int screenshot_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}


/** \brief  Callback for vsync_on_vsync_do()
 *
 * Create a screenshot on vsync to avoid tearing.
 *
 * This function is called on the VICE thread, so the canvas is retrieved in
 * ui_media_auto_screenshot() on the UI thread and passed via \a param.
 *
 * \param[in]   param   video canvas
 */
static void screenshot_auto_screenshot_vsync_callback(void *param)
{
    struct video_canvas_s *canvas = param;
    const char *format;
    char *filename;

    format = screenshot_get_quickscreenshot_format();
    if (format == NULL) {
        log_error(LOG_DEFAULT, "Failed to autosave screenshot.");
        return;
    }

    filename = screenshot_create_quickscreenshot_filename(format);
    if (filename == NULL || screenshot_save(format, filename, canvas) < 0) {
        log_error(LOG_DEFAULT, "Failed to autosave screenshot.");
    } else {
        log_message(LOG_DEFAULT, "Autosaved %s screenshot to %s", format, filename);
    }
    lib_free(filename);
}


/** \brief  Create screenshot with autogenerated filename
 *
 * Creates a screenshot with an autogenerated filename with an ISO-8601-ish
 * timestamp. See screenshot.c screenshot_get_quickscreenshot_filename().
 */
void screenshot_ui_auto_screenshot(void)
{
    if (monitor_is_inside_monitor()) {
        /* screenshot immediately if monitor is open */
        screenshot_auto_screenshot_vsync_callback((void *)ui_get_active_canvas());
    } else {
        /* queue screenshot grab on vsync to avoid tearing */
        vsync_on_vsync_do(screenshot_auto_screenshot_vsync_callback, (void *)ui_get_active_canvas());
    }
}
