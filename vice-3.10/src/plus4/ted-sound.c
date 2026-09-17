/*
 * ted-sound.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown @ axelero . hu>
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

#include "digiblaster.h"
#include "lib.h"
#include "maincpu.h"
#include "machine.h"
#include "plus4.h"
#include "plus4speech.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-resources.h"
#include "sound.h"
#include "ted-sound.h"

/* #define DEBUG_TEDSOUND */

#ifdef DEBUG_TEDSOUND
#pragma GCC diagnostic warning "-O3"
#pragma GCC diagnostic warning "-Wall"
#pragma GCC diagnostic warning "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#define DBG(x)     log_printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int ted_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static void ted_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val);
static uint8_t ted_sound_machine_read(sound_t *psid, uint16_t addr);

#ifdef SOUND_SYSTEM_FLOAT
static int ted_sound_machine_calculate_samples(sound_t **psid, float *pbuf, int nr, int sound_chip_channels, CLOCK *delta_t);
#else
static int ted_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, CLOCK *delta_t);
#endif

static int ted_sound_machine_cycle_based(void)
{
    return 0;   /* we are NOT cycle based */
}

static int ted_sound_machine_channels(void)
{
    return 1;
}

#ifdef SOUND_SYSTEM_FLOAT
/* stereo mixing placement of the TED sound */
static sound_chip_mixing_spec_t ted_sound_mixing_spec[SOUND_CHIP_CHANNELS_MAX] = {
    {
        100, /* left channel volume % in case of stereo output, default output to both */
        100  /* right channel volume % in case of stereo output, default output to both */
    }
};
#endif

/* TED sound device */
static sound_chip_t ted_sound_chip = {
    NULL,                                /* NO sound chip open function */
    ted_sound_machine_init,              /* sound chip init function */
    NULL,                                /* NO sound chip close function */
    ted_sound_machine_calculate_samples, /* sound chip calculate samples function */
    ted_sound_machine_store,             /* sound chip store function */
    ted_sound_machine_read,              /* sound chip read function */
    ted_sound_reset,                     /* sound chip reset function */
    ted_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, chip is NOT cycle based */
    ted_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, sound chip has 1 channel */
#ifdef SOUND_SYSTEM_FLOAT
    ted_sound_mixing_spec,               /* stereo mixing placement specs */
#endif
    1                                    /* sound chip enabled flag, chip is always enabled */
};

static uint16_t ted_sound_chip_offset = 0;

void ted_sound_chip_init(void)
{
    ted_sound_chip_offset = sound_chip_register(&ted_sound_chip);
}

/* ------------------------------------------------------------------------- */

static uint8_t plus4_sound_data[5];

/* dummy function for now */
int machine_sid2_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid3_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid4_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid5_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid6_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid7_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid8_check_range(unsigned int sid_adr)
{
    return 0;
}

void machine_sid2_enable(int val)
{
}

struct plus4_sound_s {
    /* Voice 0 collect number of cycles elapsed */
    uint32_t voice0_accu;
    /* Voice 0 toggle sign and reload accu if accu reached 0 */
    uint32_t voice0_reload;
    /* Voice 0 sign of the square wave */
    int16_t voice0_sign;
    uint8_t voice0_output_enabled;

    /* Voice 1 collect number of cycles elapsed */
    uint32_t voice1_accu;
    /* Voice 1 toggle sign and reload accu if accu reached 0 */
    uint32_t voice1_reload;
    /* Voice 1 sign of the square wave */
    int16_t voice1_sign;
    uint8_t voice1_output_enabled;

    uint8_t voice0_cached_output;
    uint8_t voice1_cached_output;
    uint16_t digital_cached_output;

    uint32_t oscStep;

    /* Volume multiplier  */
    int16_t volume;
    /* 8 cycles units per sample  */
    uint32_t speed;
    uint32_t sample_position_integer;
    uint32_t sample_position_remainder;
    uint32_t sample_length_integer;
    uint32_t sample_length_remainder;
    /* Digital output?  */
    uint8_t digital;
    /* Noise generator active?  */
    uint8_t noise;
    uint8_t noise_shift_register;
};

static struct plus4_sound_s snd;

#define CTRL_VOICE0_ENABLE  0x10
#define CTRL_VOICE1_ENABLE  0x20
#define CTRL_NOISE_ENABLE   0x40
#define CTRL_DIGITAL_ENABLE 0x80

#define PRECISION 12
#define OSCRELOADVAL (0x400 << PRECISION)

/* table derived from sdl-yape:
    bit 9:         voice1 output=1
    bit 8:         voice0 output=1
    bit 7-0 volume (8-f are all the same)
*/
static const int16_t volumeTable[4 * 16] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x024a, 0x064a, 0x0a4a, 0x0e4a, 0x124a, 0x164a, 0x1a4a,
    0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a,
    0x0000, 0x024a, 0x064a, 0x0a4a, 0x0e4a, 0x124a, 0x164a, 0x1a4a,
    0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a, 0x1e4a,
    0x0000, 0x0494, 0x0cd4, 0x1596, 0x1f30, 0x29a2, 0x34ec, 0x410e,
    0x4e08, 0x4e08, 0x4e08, 0x4e08, 0x4e08, 0x4e08, 0x4e08, 0x4e08
};

static inline void clock_shift_register(void)
{
    snd.noise_shift_register = (snd.noise_shift_register << 1) |
                               (((snd.noise_shift_register >> 7) ^
                                 (snd.noise_shift_register >> 5) ^
                                 (snd.noise_shift_register >> 4) ^
                                 (snd.noise_shift_register >> 1)) & 1);
}

static inline void reset_shift_register(void)
{
    snd.noise_shift_register = 0xff;
}

/* the general logic was heavily inspired by SDL-YAPE:

   https://github.com/calmopyrin/yapesdl/blob/master/tedsound.cpp
 */

#ifdef SOUND_SYSTEM_FLOAT
/* FIXME! only the non-float code below has the new fixed TED sound stuff */
#warning "only the non-float code has the fixed TED sound stuff"
static int ted_sound_machine_calculate_samples(sound_t **psid, float *pbuf, int nr, int scc, CLOCK *delta_t)
{
    int i;
    int j;
    int16_t volume;
    float sample;

    if (snd.digital) {
        for (i = 0; i < nr; i++) {
            sample = (snd.volume * (snd.voice0_output_enabled + snd.voice1_output_enabled)) / 32767.0;
            pbuf[i] = sample;
        }
    } else {
        for (i = 0; i < nr; i++) {
            snd.sample_position_remainder += snd.sample_length_remainder;
            if (snd.sample_position_remainder >= snd.speed) {
                snd.sample_position_remainder -= snd.speed;
                snd.sample_position_integer++;
            }
            snd.sample_position_integer += snd.sample_length_integer;
            if (snd.sample_position_integer >= 8) {
                /* Advance state engine */
                uint32_t ticks = snd.sample_position_integer >> 3;
                if (snd.voice0_accu <= ticks) {
                    uint32_t delay = ticks - snd.voice0_accu;
                    snd.voice0_sign ^= CTRL_VOICE0_ENABLE;
                    snd.voice0_accu = OSCRELOADVAL - snd.voice0_reload;
                    if (snd.voice0_accu == 0) {
                        snd.voice0_accu = OSCRELOADVAL;
                    }
                    if (delay >= snd.voice0_accu) {
                        snd.voice0_sign = ((delay / snd.voice0_accu)
                                           & 1) ? snd.voice0_sign ^ CTRL_VOICE0_ENABLE
                                          : snd.voice0_sign;
                        snd.voice0_accu = snd.voice0_accu - (delay % snd.voice0_accu);
                    } else {
                        snd.voice0_accu -= delay;
                    }
                } else {
                    snd.voice0_accu -= ticks;
                }

                if (snd.voice1_accu <= ticks) {
                    uint32_t delay = ticks - snd.voice1_accu;
                    snd.voice1_sign ^= CTRL_VOICE1_ENABLE;
                    clock_shift_register();
                    snd.voice1_accu = OSCRELOADVAL - snd.voice1_reload;
                    if (snd.voice1_accu == 0) {
                        snd.voice1_accu = OSCRELOADVAL;
                    }
                    if (delay >= snd.voice1_accu) {
                        snd.voice1_sign = ((delay / snd.voice1_accu)
                                           & 1) ? snd.voice1_sign ^ CTRL_VOICE1_ENABLE
                                          : snd.voice1_sign;
                        for (j = 0; j < (int)(delay / snd.voice1_accu); j++) {
                            clock_shift_register();
                        }
                        snd.voice1_accu = snd.voice1_accu - (delay % snd.voice1_accu);
                    } else {
                        snd.voice1_accu -= delay;
                    }
                } else {
                    snd.voice1_accu -= ticks;
                }
            }
            snd.sample_position_integer = snd.sample_position_integer & 7;

            volume = 0;

            if (snd.voice0_output_enabled && snd.voice0_sign) {
                volume += snd.volume;
            }
            if (snd.voice1_output_enabled && !snd.noise && snd.voice1_sign) {
                volume += snd.volume;
            }
            if (snd.voice1_output_enabled && snd.noise && (!(snd.noise_shift_register & 1))) {
                volume += snd.volume;
            }


            sample = volume / 32767.0;

            pbuf[i] = sample;
        }
    }
    return nr;
}
#else
static int ted_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t)
{
    int i;
    int16_t volume;

    if (snd.digital) {
        for (i = 0; i < nr; i++) {
            pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], snd.digital_cached_output);
            if (soc == SOUND_OUTPUT_STEREO) {
                pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], snd.digital_cached_output);
            }
        }
    } else {
        for (i = 0; i < nr; i++) {
            snd.sample_position_remainder += snd.sample_length_remainder;
            if (snd.sample_position_remainder >= snd.speed) {
                snd.sample_position_remainder -= snd.speed;
                snd.sample_position_integer++;
            }
            snd.sample_position_integer += snd.sample_length_integer;
            if (snd.sample_position_integer >= 8) {
                /* Advance state engine */
                if ((snd.voice0_reload & (0x3ff << PRECISION)) != (0x3ff << PRECISION)) {
                    if((snd.voice0_accu += snd.oscStep) >= OSCRELOADVAL) {
                        snd.voice0_sign ^= CTRL_VOICE0_ENABLE;
                        snd.voice0_cached_output = snd.volume | (snd.voice0_sign & snd.voice0_output_enabled);
                        snd.voice0_accu = snd.voice0_reload + (snd.voice0_accu - OSCRELOADVAL);
                    }
                }

                if ((snd.voice1_reload & (0x3ff << PRECISION)) != (0x3ff << PRECISION)) {
                    if((snd.voice1_accu += snd.oscStep) >= OSCRELOADVAL) {
                        snd.voice1_sign ^= CTRL_VOICE1_ENABLE;
                        snd.voice1_cached_output = snd.volume |
                                                   (snd.voice1_sign & snd.voice1_output_enabled) |
                                                   (((snd.noise_shift_register & 1) ? CTRL_NOISE_ENABLE : 0) & snd.noise) >> 1;
                        clock_shift_register();
                        snd.voice1_accu = snd.voice1_reload + (snd.voice1_accu - OSCRELOADVAL);
                    }
                }
            }
            snd.sample_position_integer = snd.sample_position_integer & 7;
#if 0
printf("%02x: %02x %02x  %02x %02x\n", snd.volume,
       snd.voice0_output_enabled, snd.voice0_sign,
       snd.voice1_output_enabled, snd.voice1_sign);
#endif
            volume = volumeTable[snd.voice0_cached_output | snd.voice1_cached_output];

            pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], volume);
            if (soc == SOUND_OUTPUT_STEREO) {
                pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], volume);
            }
        }
    }
    return nr;
}
#endif

static int ted_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    uint8_t val;

    DBG(("ted_sound_machine_init speed: %d cycles_per_sec: %d\n", speed, cycles_per_sec));
    snd.speed = speed;
    snd.sample_length_integer = cycles_per_sec / speed;
    snd.sample_length_remainder = cycles_per_sec % speed;
    snd.sample_position_integer = 0;
    snd.sample_position_remainder = 0;

    snd.oscStep = (int)(((cycles_per_sec / 8) * (double)(1 << PRECISION)) / (double)(speed) + 0.5);;

    snd.voice0_reload = ((plus4_sound_data[0] | (plus4_sound_data[4] << 8)) + 1) & 0x3ff;
    snd.voice1_reload = ((plus4_sound_data[1] | (plus4_sound_data[2] << 8)) + 1) & 0x3ff;

    val = plus4_sound_data[3];  /* control register */
    snd.volume = val & 0x0f;
    snd.voice0_output_enabled = val & CTRL_VOICE0_ENABLE;
    snd.voice1_output_enabled = val & CTRL_VOICE1_ENABLE;
    snd.noise = ((val & 0x60) == CTRL_NOISE_ENABLE) ? CTRL_NOISE_ENABLE : 0;
    snd.digital = val & CTRL_DIGITAL_ENABLE;

    snd.voice0_sign = 0;
    snd.voice0_accu = 0;
    snd.voice1_sign = 0;
    snd.voice1_accu = 0;
    reset_shift_register();

    snd.voice0_cached_output = 0;
    snd.voice1_cached_output = 0;
    snd.digital_cached_output = 0;

    return 1;
}

static void ted_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val)
{
    unsigned int freq;
    switch (addr) {
        case 0x0e: /* voice0 freq lo */
            plus4_sound_data[0] = val;
            freq = plus4_sound_data[0] | (plus4_sound_data[4] << 8);
            if (freq == 0x3fe) {
                snd.voice0_sign = CTRL_VOICE0_ENABLE;
                snd.voice0_cached_output = snd.volume | snd.voice0_output_enabled;
            }
            snd.voice0_reload = ((freq + 1) & 0x3ff) << PRECISION;
            break;
        case 0x0f: /* voice1 freq lo */
            plus4_sound_data[1] = val;
            freq = plus4_sound_data[1] | (plus4_sound_data[2] << 8);
            if (freq == 0x3fe) {
                snd.voice1_sign = CTRL_VOICE1_ENABLE;
                snd.voice1_cached_output = snd.volume | snd.voice1_output_enabled | (snd.noise >> 1);
            }
            snd.voice1_reload = ((freq + 1) & 0x3ff) << PRECISION;
            break;
        case 0x10: /* voice1 freq hi */
            plus4_sound_data[2] = val & 3;
            freq = plus4_sound_data[1] | (plus4_sound_data[2] << 8);
            if (freq == 0x3fe) {
                snd.voice1_sign = CTRL_VOICE1_ENABLE;
                snd.voice1_cached_output = snd.volume | snd.voice1_output_enabled | (snd.noise >> 1);
            }
            snd.voice1_reload = ((freq + 1) & 0x3ff) << PRECISION;
            break;
        case 0x11:
            /* bit 0-3  volume
                   4    voice 0 enable
                   5    voice 1 enable
                   6    noise enable
                   7    digital mode enabled
             */
            snd.volume = val & 0x0f;
            snd.voice0_output_enabled = (val & CTRL_VOICE0_ENABLE);
            snd.voice1_output_enabled = (val & CTRL_VOICE1_ENABLE);
            snd.noise = ((val & 0x60) == CTRL_NOISE_ENABLE) ? CTRL_NOISE_ENABLE : 0;
            snd.digital = val & CTRL_DIGITAL_ENABLE;
            if (snd.digital) {
                snd.voice0_sign = CTRL_VOICE0_ENABLE;
                snd.voice0_accu = snd.voice0_reload;
                snd.voice1_sign = CTRL_VOICE1_ENABLE;
                snd.voice1_accu = snd.voice1_reload;
                reset_shift_register();
                snd.digital_cached_output = volumeTable[val & 0x3f];
            }
            snd.voice0_cached_output = snd.volume |
                                       (snd.voice0_sign & snd.voice0_output_enabled);
            snd.voice1_cached_output = snd.volume |
                                       (snd.voice1_sign & snd.voice1_output_enabled) |
                                       ((((snd.noise_shift_register & 1) ? CTRL_NOISE_ENABLE : 0) & snd.noise) >> 1);
            plus4_sound_data[3] = val;
            break;
        case 0x12: /* voice0 freq hi */
            plus4_sound_data[4] = val & 3;
            freq = plus4_sound_data[0] | (plus4_sound_data[4] << 8);
            if (freq == 0x3fe) {
                snd.voice0_sign = CTRL_VOICE0_ENABLE;
                snd.voice0_cached_output = snd.volume | snd.voice0_output_enabled;
            }
            snd.voice0_reload = ((freq + 1) & 0x3ff) << PRECISION;
            break;
    }
#if 0
    DBG(("freq0:%04x freq1:%04x ctrl:%02x\n",
            plus4_sound_data[0] | (plus4_sound_data[4] << 8),
            plus4_sound_data[1] | (plus4_sound_data[2] << 8),
            plus4_sound_data[3]));
#endif
}

static uint8_t ted_sound_machine_read(sound_t *psid, uint16_t addr)
{
    switch (addr) {
        case 0x0e:
            return plus4_sound_data[0];
        case 0x0f:
            return plus4_sound_data[1];
        case 0x10:
            return plus4_sound_data[2] | 0xc0;
        case 0x11:
            return plus4_sound_data[3];
        case 0x12:
            return plus4_sound_data[4];
    }

    return 0;
}

void ted_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    uint16_t i;

    snd.voice0_sign = 0;
    snd.voice1_sign = 0;
    snd.voice0_accu = 0;
    snd.voice1_accu = 0;
    reset_shift_register();
    snd.digital = 0;
    snd.voice0_cached_output = 0;
    snd.voice1_cached_output = 0;
    snd.digital_cached_output = 0;

    /* FIXME: this is is almost certainly not correct */
    for (i = 0x0e; i <= 0x12; i++) {
        ted_sound_store(i, 0);
    }
}

/* ---------------------------------------------------------------------*/

void ted_sound_store(uint16_t addr, uint8_t value)
{
    sound_store((uint16_t)(ted_sound_chip_offset | addr), value, 0);
}

uint8_t ted_sound_read(uint16_t addr)
{
    uint8_t value;

    value = sound_read((uint16_t)(ted_sound_chip_offset | addr), 0);

    if (addr == 0x12) {
        value &= 3;
    }

    return value;
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
