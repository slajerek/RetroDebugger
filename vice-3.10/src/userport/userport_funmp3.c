/*
 * userport_funmp3.c - Userport FunMP3 emulation.
 *    details about this cartridge can be found here
 *    http://www.christianes-components.de [credits: Markus Neeb FunMP3-Player]
 *
 * Written by
 *  pottendo <pottendo@gmx.net>
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

/* #define DEBUG_FUNMP3 */
#include "vice.h"

#if defined(USE_MPG123) && defined (HAVE_GLOB_H)
#include <mpg123.h>
#include <archdep.h>

#include "userport.h"
#include "joystick.h"
#include "userport_funmp3.h"
#include "log.h"
#include "sound.h"
#include "alarm.h"
#include "maincpu.h"
#include "machine.h"
#include "resources.h"
#include "lib.h"
#include "util.h"
#include "cmdline.h"

static int userport_funmp3_enable(int value);
static void userport_funmp3_store_pbx(uint8_t value, int pulse);
static void userport_funmp3_reset(void);

static int funmp3_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static void funmp3_sound_machine_close(sound_t *psid);
static int funmp3_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t);
static void funmp3_sound_reset(sound_t *psid, CLOCK cpu_clk);
static int funmp3_sound_machine_cycle_based(void);
static int funmp3_sound_machine_channels(void);
static int funmp3_set_dir(const char *val, void *v);
static void funmp3_start(uint8_t val);
static void funmp3_stop(void);

static userport_device_t userport_funmp3_device = {
    "Userport FunMP3 Player",             /* device name */
    JOYSTICK_ADAPTER_ID_NONE,             /* this is NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_AUDIO_OUTPUT,    /* device is a WIFI adapter */
    userport_funmp3_enable,               /* enable function */
    NULL,                                 /* read pb0-pb7 function */
    userport_funmp3_store_pbx,            /* store pb0-pb7 function */
    NULL,                                 /* read pa2 pin function */
    NULL,                                 /* store pa2 pin function */
    NULL,                                 /* NO read pa3 pin function */
    NULL,                                 /* NO store pa3 pin function */
    /* HACK: We put a 0 into the struct here, although pin 8 of the userport
       (which is PC2 on the C64) is actually used. This is needed so the device
       can be registered in xvic (where the pin is driven by PA6). */
    0,                                    /* pc pin IS needed */
    NULL,                                 /* NO store sp1 pin function */
    NULL,                                 /* NO read sp1 pin function */
    NULL,                                 /* NO store sp2 pin function */
    NULL,                                 /* NO read sp2 pin function */
    userport_funmp3_reset,                /* reset function */
    NULL,                                 /* NO powerup function */
    NULL,                                 /* snapshot write function */
    NULL,                                 /* snapshot read function */
};

/* FunMP3 sound chip */
static sound_chip_t funmp3_sound_chip = {
    NULL,                                   /* NO sound chip open function */
    funmp3_sound_machine_init,              /* sound chip init function */
    funmp3_sound_machine_close,             /* sound chip close function */
    funmp3_sound_machine_calculate_samples, /* sound chip calculate samples function */
    NULL,                                   /* NO sound chip store function */
    NULL,                                   /* NO sound chip read function */
    funmp3_sound_reset,                     /* sound chip reset function */
    funmp3_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, sound chip is NOT cycle based */
    funmp3_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, sound chip has 1 channel */
    0                                       /* chip enabled, toggled when sound chip is (de-)activated */
};


static log_t lh = LOG_DEFAULT;
static int userport_funmp3_enabled = 0;
static int mp3_err = MPG123_OK;
static mpg123_handle *mh = NULL;

static int mp3_playing = 0;     /* indicates if playing is on-going */
static int mp3_last = 0;        /* this is the last requested mp3, unless explicitly asked not to repeat, then it's 0 */
static char *funmp3_dir = NULL;
static const resource_string_t funmp3_resources[] =
{
    { "FunMP3Dir", ".", (resource_event_relevant_t)0, NULL,
      &funmp3_dir, funmp3_set_dir, NULL },
    RESOURCE_STRING_LIST_END,
};

static const cmdline_option_t cmdline_options[] =
{
    { "-funmp3dir", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FunMP3Dir", NULL,
      "<path>", "Set FunMP3 directory" },
    CMDLINE_LIST_END
};

int userport_funmp3_resources_init(void)
{
    if (resources_register_string(funmp3_resources) < 0) {
        return -1;
    }
    userport_funmp3_reset();
    return userport_device_register(USERPORT_DEVICE_FUNMP3, &userport_funmp3_device);
}

int userport_funmp3_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

void userport_funmp3_resources_shutdown(void)
{
}

void userport_funmp3_sound_chip_init(void)
{
    sound_chip_register(&funmp3_sound_chip);
}

static int funmp3_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    return 1;
}

static void funmp3_sound_machine_close(sound_t *psid)
{
    funmp3_stop();
}

static int funmp3_set_dir(const char *val, void *v)
{
    util_string_set(&funmp3_dir, val);
    return 0;
}

static uint8_t *bufr = NULL;
static int16_t *buf = NULL;
static size_t size;
static size_t act_pos = 0;
static int ret = MPG123_OK;

static int funmp3_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t)
{
    size_t i, j, rem;
    int16_t sample;
    int res = 0;
    if (!mp3_playing) {
        return 0;
    }
    rem = (size - act_pos);
    if (nr > rem) {
        /* if sound engine requests more than buffered,
           put remaining samples to the front and decode new chunk */
        if (rem > 0) {
            /* copy unconsumed samples to the front */
            for (i = act_pos, j = 0; i < size; i++, j++) {
                buf[j] = buf[i];
            }
        }
        act_pos = 0;
        ret = mpg123_read(mh, (unsigned char *)bufr + (rem * sizeof(int16_t)),
                          mpg123_outblock(mh), &size);
        buf = (int16_t *)bufr;
        size = size / sizeof(int16_t) + rem;
    }

    if ((ret == MPG123_OK) ||
        (ret == MPG123_DONE)) {
        for (i = 0; i < nr; ++i) {
            switch (soc) {
            default:
            case SOUND_OUTPUT_MONO:
                sample = sound_audio_mix(buf[act_pos], buf[act_pos]);
                pbuf[i] = sound_audio_mix(pbuf[i], sample);
                break;
            case SOUND_OUTPUT_STEREO:
                pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], buf[act_pos]);
                pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], buf[act_pos + 1]);
                break;
            }
            act_pos++;
        }
        res = nr;
    } else {
        log_error(lh, "mpg123_read() error: %d", ret);
        res = 0;
    }
    if (ret == MPG123_DONE) {
        funmp3_stop();
        if (mp3_last) {
            funmp3_start(255-mp3_last);
        }
    }
    return res;
}

static void funmp3_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    funmp3_stop();
}

static int funmp3_sound_machine_cycle_based(void)
{
    return 0;
}

static int funmp3_sound_machine_channels(void)
{
    return 1;
}

static void funmp3_start(uint8_t val)
{
    char *mp3_name = NULL;
    int rate;
    char fn[ARCHDEP_PATH_MAX];
    char *fn_abs;
    glob_t results;

    if (!mh) {
        log_message(lh, "mpg123 not initialized");
        return;
    }
    if (funmp3_dir) {
        snprintf(fn, ARCHDEP_PATH_MAX, "%s/%05d*", funmp3_dir, val);
    } else {
        snprintf(fn, ARCHDEP_PATH_MAX, "%05d*", val);
    }
    archdep_expand_path(&fn_abs, fn);
    if ((archdep_glob(fn_abs, 0, NULL, &results) != 0) ||
        (results.gl_pathc < 1)) {
        log_message(lh, "can't open '%s'", fn_abs);
        return;
    }
    mp3_name = lib_strdup(results.gl_pathv[0]);
    archdep_globfree(&results);
    log_message(lh, "request to play '%s'", mp3_name);

    mpg123_close(mh);
    mpg123_format_none(mh);
    resources_get_int("SoundSampleRate", &rate);
    if ((ret = mpg123_format(mh, (long)rate,
                             MPG123_MONO,
                             MPG123_ENC_SIGNED_16)) != MPG123_OK) {
        log_error(lh, "failed to set format: %d", ret);
        mpg123_exit();
        goto out;
    }
    if (mpg123_open(mh, mp3_name) != MPG123_OK) {
        log_error(lh, "failed to open: %s", mp3_name);
        goto out;
    }

    mpg123_volume(mh, 1.0);
    int channels = 1, encoding;
    /* this is needed, see commen in mpg123.h */
    mpg123_getformat(mh, (long *)&rate, &channels, &encoding);
    /*
    log_message(lh, "2 - mp3rate = %d, ch = %d, enc = %d, outblock = %lu, sample_size = %d",
                rate, channels, encoding, mpg123_outblock(mh), MPG123_SAMPLESIZE(encoding));
    */
    mp3_playing = 1;
    set_userport_flag(1);
    if (bufr) {
        lib_free(bufr);
    }
    bufr = lib_malloc(mpg123_outblock(mh) * 2); /* allocate 2x, ensures sufficient for boundary cases */
 out:
    lib_free(mp3_name);
    lib_free(fn_abs);
}

static void funmp3_stop(void)
{
    if (mh) {
        mpg123_close(mh);
    }
    mp3_playing = act_pos = size = 0;
    set_userport_flag(0);
}

static int userport_funmp3_enable(int value)
{
    if (userport_funmp3_enabled == value) {
        return 0;
    }
    if (lh == LOG_DEFAULT) {
        lh = log_open("FunMP3");
    }
    if (value) {
        /* Fixme: clockport mp3@64 driver may run and clash, as MMC is a cartridge */
        mp3_err = mpg123_init();
        if (mp3_err != MPG123_OK) {
            log_error(lh, "failed to intialized mpg123");
            return 0;
        }
        mh = mpg123_new(NULL, &mp3_err);
        if (!mh) {
            log_error(lh, "failed to create mpg123 handle");
            mpg123_exit();
            return 0;
        }
    } else {
        mp3_last = 0;
        funmp3_stop();
        if (mh) {
            mpg123_delete(mh);
            mh = NULL;
        }
        if (bufr) {
            lib_free(bufr);
            bufr = NULL;
        }
        mpg123_exit();
    }
    userport_funmp3_enabled = value;
    funmp3_sound_chip.chip_enabled = value;
    log_message(lh, "FunMP3 %s", value ? "enabled" : "disabled");
    return 0;
}

/* PC2 irq (pulse) triggers when C64 reads/writes to userport */
static void userport_funmp3_store_pbx(uint8_t val, int pulse)
{
    if (pulse == 1) {
        return;
    }
    /* log_message(lh, "%s: val = %d, pulse = %d", __FUNCTION__, val, pulse); */

    if (val == 0) {
        /* stop sound output */
        funmp3_stop();
        return;
    }
    if (val == 255) {
        /* stop repetition of last mp3 */
        mp3_last = 0;
        return;
    }
    funmp3_stop();
    mp3_last = val;             /* the default is to repeat */
    funmp3_start(255 - val);    /* that's how it is defined */
}

static void userport_funmp3_reset(void)
{
    funmp3_stop();
}

#endif /* MPG123 */
