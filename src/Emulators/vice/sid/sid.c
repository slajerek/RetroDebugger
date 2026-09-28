/*
 * sid.c - MOS6581 (SID) emulation, hooks to actual implementation.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Michael Schwendt <sidplay@geocities.com>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Dag Lem <resid@nimrod.no>
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
#include <string.h>

#include "catweaselmkiii.h"
#include "fakesid.h"
#include "fastsid.h"
#include "hardsid.h"
#include "joyport.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "parsid.h"
#include "resources.h"
#include "sid-resources.h"
#include "sid-snapshot.h"
#include "sid.h"
#include "sound.h"
#include "ssi2001.h"
#include "usbsid.h"
#include "vicetypes.h"

#include "log.h"

#ifdef HAVE_MOUSE
#include "mouse.h"
#include "lightpen.h"
#endif

#ifdef HAVE_RESID
#include "resid.h"
#endif

#ifdef HAVE_RESID_FP
#include "resid-fp.h"
#endif

/* SID engine hooks. */
static sid_engine_t sid_engine;

/* read register value from sid */
static uint8_t lastsidread;

/* register data */
static uint8_t siddata[SOUND_SIDS_MAX][32];

static int (*sid_read_func)(uint16_t addr, int chipno);
static void (*sid_store_func)(uint16_t addr, uint8_t val, int chipno);
static int (*sid_dump_func)(int chipno);

static int sid_enable, sid_engine_type = -1;

#ifdef HAVE_MOUSE
static CLOCK pot_cycle = 0;  /* pot sampling cycle */
static uint8_t val_pot_x = 0xff, val_pot_y = 0xff; /* last sampling value */
#endif

static uint8_t c64d_sid_voiceMask = 0xFF;

uint8_t *sid_get_siddata(unsigned int channel)
{
    return siddata[channel];
}

/* ------------------------------------------------------------------------- */

static int sid_read_off(uint16_t addr, int chipno)
{
    uint8_t val;

    if (addr == 0x19 || addr == 0x1a) {
        val = 0xff;
    } else {
        if (addr == 0x1b || addr == 0x1c) {
            val = (uint8_t)(maincpu_clk % 256);
        } else {
            val = 0;
        }
    }

    /* FIXME: Change API, return uint8_t! */
    return (int)val;
}

static void sid_write_off(uint16_t addr, uint8_t val, int chipno)
{
}

/* ------------------------------------------------------------------------- */

static uint8_t sid_read_chip(uint16_t addr, int chipno)
{
    int val = -1;

    addr &= 0x1f;

    machine_handle_pending_alarms(0);

#ifdef HAVE_MOUSE
    if (chipno == 0 && (addr == 0x19 || addr == 0x1a)) {
        if ((maincpu_clk ^ pot_cycle) & ~511) {
            pot_cycle = maincpu_clk & ~511; /* simplistic 512 cycle sampling */
            val_pot_x = read_joyport_potx();
            val_pot_y = read_joyport_poty();
        }
        val = (addr == 0x19) ? val_pot_x : val_pot_y;

    } else {
#endif
        if (machine_class == VICE_MACHINE_C64SC
            || machine_class == VICE_MACHINE_SCPU64) {
            /* On x64sc, the read/write calls both happen before incrementing
               the clock, so don't mess with maincpu_clk here.  */
            val = sid_read_func(addr, chipno);
        } else {
            /* Account for that read functions in VICE are called _before_
               incrementing the clock. */
            maincpu_clk++;
			c64d_maincpu_clk++;
            val = sid_read_func(addr, chipno);
            maincpu_clk--;
			c64d_maincpu_clk--;
        }
#ifdef HAVE_MOUSE
    }
#endif

    /* Fallback when sound is switched off. */
    if (val < 0) {
        if (addr == 0x19 || addr == 0x1a) {
            val = 0xff;
        } else {
            if (addr == 0x1b || addr == 0x1c) {
                val = maincpu_clk % 256;
            } else {
                val = 0;
            }
        }
    }

    lastsidread = val;
    return val;
}

uint8_t sid_peek_chip(uint16_t addr, int chipno)
{
    addr &= 0x1f;

    /* FIXME: get 0x1b and 0x1c from engine */
    return siddata[chipno][addr];
}

/* write register value to sid */
void sid_store_chip(uint16_t addr, uint8_t value, int chipno)
{
    addr &= 0x1f;

    siddata[chipno][addr] = value;

    /* WARNING: assumes `maincpu_rmw_flag' is 0 or 1.  */
    machine_handle_pending_alarms(maincpu_rmw_flag + 1);

    if (maincpu_rmw_flag) {
        maincpu_clk--;
		c64d_maincpu_clk--;
        sid_store_func(addr, lastsidread, chipno);
        maincpu_clk++;
		c64d_maincpu_clk++;
    }

    sid_store_func(addr, value, chipno);
}

static int sid_dump_chip(int chipno)
{
    if (sid_dump_func) {
        return sid_dump_func(chipno);
    }

    return -1;
}

#define C64_NUM_SID_REGISTERS 32
void c64d_store_sid_data(uint8_t *sidDataStore, int sidNum)
{
	memcpy(sidDataStore, siddata[sidNum], C64_NUM_SID_REGISTERS);
}

/* ------------------------------------------------------------------------- */

extern int c64d_sid_register_written;
extern int c64d_sid_register_read;
extern unsigned char c64d_sid_write_value;
extern unsigned char c64d_sid_read_value;

uint8_t sid_read(uint16_t addr)
{
    uint8_t val;
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        val = sid_read_chip(addr, 1);
    }
    else if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        val = sid_read_chip(addr, 2);
    }
    else {
        val = sid_read_chip(addr, 0);
    }

    c64d_sid_register_read = addr & 0x1f;
    c64d_sid_read_value = val;
    return val;
}

uint8_t sid_peek(uint16_t addr)
{
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        return sid_peek_chip(addr, 1);
    }

    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        return sid_peek_chip(addr, 2);
    }

    return sid_peek_chip(addr, 0);
}

uint8_t sid2_read(uint16_t addr)
{
    return sid_read_chip(addr, 1);
}

uint8_t sid3_read(uint16_t addr)
{
    return sid_read_chip(addr, 2);
}

/* sid4-8 stubs (from VICE 3.10 multi-SID support) */
uint8_t sid4_read(uint16_t addr) { return sid_read_chip(addr, 3); }
uint8_t sid5_read(uint16_t addr) { return sid_read_chip(addr, 4); }
uint8_t sid6_read(uint16_t addr) { return sid_read_chip(addr, 5); }
uint8_t sid7_read(uint16_t addr) { return sid_read_chip(addr, 6); }
uint8_t sid8_read(uint16_t addr) { return sid_read_chip(addr, 7); }

uint8_t sid2_peek(uint16_t addr) { return sid_peek_chip(addr, 1); }
uint8_t sid3_peek(uint16_t addr) { return sid_peek_chip(addr, 2); }
uint8_t sid4_peek(uint16_t addr) { return sid_peek_chip(addr, 3); }
uint8_t sid5_peek(uint16_t addr) { return sid_peek_chip(addr, 4); }
uint8_t sid6_peek(uint16_t addr) { return sid_peek_chip(addr, 5); }
uint8_t sid7_peek(uint16_t addr) { return sid_peek_chip(addr, 6); }
uint8_t sid8_peek(uint16_t addr) { return sid_peek_chip(addr, 7); }

void sid_store(uint16_t addr, uint8_t byte)
{
    c64d_sid_register_written = addr & 0x1f;
    c64d_sid_write_value = byte;

    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        sid_store_chip(addr, byte, 1);
        return;
    }
    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        sid_store_chip(addr, byte, 2);
        return;
    }
    sid_store_chip(addr, byte, 0);
}

void sid2_store(uint16_t addr, uint8_t byte)
{
    sid_store_chip(addr, byte, 1);
}

void sid3_store(uint16_t addr, uint8_t byte)
{
    sid_store_chip(addr, byte, 2);
}

void sid4_store(uint16_t addr, uint8_t byte) { sid_store_chip(addr, byte, 3); }
void sid5_store(uint16_t addr, uint8_t byte) { sid_store_chip(addr, byte, 4); }
void sid6_store(uint16_t addr, uint8_t byte) { sid_store_chip(addr, byte, 5); }
void sid7_store(uint16_t addr, uint8_t byte) { sid_store_chip(addr, byte, 6); }
void sid8_store(uint16_t addr, uint8_t byte) { sid_store_chip(addr, byte, 7); }

int sid_dump(void)
{
    return sid_dump_chip(0);
}

int sid2_dump(void)
{
    return sid_dump_chip(1);
}

int sid3_dump(void)
{
    return sid_dump_chip(2);
}

int sid4_dump(void) { return sid_dump_chip(3); }
int sid5_dump(void) { return sid_dump_chip(4); }
int sid6_dump(void) { return sid_dump_chip(5); }
int sid7_dump(void) { return sid_dump_chip(6); }
int sid8_dump(void) { return sid_dump_chip(7); }

/* ------------------------------------------------------------------------- */

void sid_reset(void)
{
    sound_reset();

    memset(siddata, 0, sizeof(siddata));
}

static int sidengine;

int sid_sound_machine_set_engine_hooks(void)
{
    sidengine = -1;

    if (resources_get_int("SidEngine", &sidengine) < 0) {
        return 0;
    }

    sid_engine = fastsid_hooks;  /* default: fastsid always available */

    if (sidengine == SID_ENGINE_FASTSID) {
        sid_engine = fastsid_hooks;
    }

#ifdef HAVE_RESID
    if (sidengine == SID_ENGINE_RESID) {
        sid_engine = resid_hooks;
    }
#endif

#ifdef HAVE_RESID_FP
    if (sidengine == SID_ENGINE_RESID_FP) {
        sid_engine = residfp_hooks;
    }
#endif

    return (sidengine >= 0) ? 1 : 0;
}

sound_t *sid_sound_machine_open(int chipno)
{
	LOGD("sid_sound_machine_open");

    if (!sid_sound_machine_set_engine_hooks()) {
        return NULL;
    }

    return sid_engine.open(siddata[chipno], chipno);
}

/* manage temporary buffers. if the requested size is smaller or equal to the
 * size of the already allocated buffer, reuse it.  */
static int16_t *buf1 = NULL;
static int16_t *buf2 = NULL;
static int blen1 = 0;
static int blen2 = 0;

static int16_t *getbuf1(int len)
{
    if ((buf1 == NULL) || (blen1 < len)) {
        if (buf1) {
            lib_free(buf1);
        }
        blen1 = len;
        buf1 = lib_calloc(len, 1);
    }
    return buf1;
}

static int16_t *getbuf2(int len)
{
    if ((buf2 == NULL) || (blen2 < len)) {
        if (buf2) {
            lib_free(buf2);
        }
        blen2 = len;
        buf2 = lib_calloc(len, 1);
    }
    return buf2;
}

int sid_sound_machine_init_vbr(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
    return sid_engine.init(psid, speed * factor / 1000, cycles_per_sec, factor);
}

int sid_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
#ifdef HAVE_USBSID
    if (sid_engine_type == SID_ENGINE_USBSID) {
        usbsid_open();
    }
#endif
    int ret = sid_engine.init(psid, speed, cycles_per_sec, 1000);

	sid_engine.set_voice_mask(psid, c64d_sid_voiceMask);

	return ret;
}

void sid_sound_machine_close(sound_t *psid)
{
    if (sid_engine.close) {
        sid_engine.close(psid);
    }
    /* free the temp. buffers */
    if (buf1) {
        lib_free(buf1);
        buf1 = NULL;
    }
    if (buf2) {
        lib_free(buf2);
        buf2 = NULL;
    }
#ifdef HAVE_USBSID
    usbsid_close();
#endif
}

uint8_t sid_sound_machine_read(sound_t *psid, uint16_t addr)
{
    return sid_engine.read(psid, addr);
}

void sid_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t byte)
{
    sid_engine.store(psid, addr, byte);
}

void sid_sound_machine_reset(sound_t *psid, CLOCK cpu_clk)
{
    sid_engine.reset(psid, cpu_clk);
#ifdef HAVE_USBSID
    usbsid_reset(true);
#endif
}

int sid_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    int16_t *tmp_buf1;
    int16_t *tmp_buf2;
    int tmp_nr = 0;
    int tmp_delta_t = *delta_t;

    /* Cap scc to actual SID channel count (1-3). The sound system passes the
       global max across ALL chips (which includes digimax=4), but the SID
       only supports mono/stereo/triple configurations.
       sid_stereo is 0=mono, 1=stereo, 2=triple — use it directly instead of
       resources_get_int() to avoid hash+strcmp overhead per audio frame. */
    scc = sid_stereo + 1;

    if (soc == 1 && scc == 1) {
        return sid_engine.calculate_samples(psid[0], pbuf, nr, 1, delta_t);
    }
    if (soc == 1 && scc == 2) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
        }
        return tmp_nr;
    }
    if (soc == 1 && scc == 3) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_buf2 = getbuf2(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[0], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf2, nr, 1, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf, nr, 1, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf1[i]);
            pbuf[i] = sound_audio_mix(pbuf[i], tmp_buf2[i]);
        }
        return tmp_nr;
    }
    if (soc == 2 && scc == 1) {
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[(i * 2) + 1] = pbuf[i * 2];
        }
        return tmp_nr;
    }
    if (soc == 2 && scc == 2) {
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        return tmp_nr;
    }
    if (soc == 2 && scc == 3) {
        tmp_buf1 = getbuf1(2 * nr);
        tmp_nr = sid_engine.calculate_samples(psid[2], tmp_buf1, nr, 1, &tmp_delta_t);
        tmp_delta_t = *delta_t;
        tmp_nr = sid_engine.calculate_samples(psid[0], pbuf, nr, 2, &tmp_delta_t);
        tmp_nr = sid_engine.calculate_samples(psid[1], pbuf + 1, nr, 2, delta_t);
        for (i = 0; i < tmp_nr; i++) {
            pbuf[i * 2] = sound_audio_mix(pbuf[i * 2], tmp_buf1[i]);
            pbuf[(i * 2) + 1] = sound_audio_mix(pbuf[(i * 2) + 1], tmp_buf1[i]);
        }
    }
    return tmp_nr;
}

void sid_sound_machine_set_voice_mask(sound_t *psid, uint8_t voiceMask)
{
	c64d_sid_voiceMask = voiceMask;
	sid_engine.set_voice_mask(psid, voiceMask);
}

char *sid_sound_machine_dump_state(sound_t *psid)
{
    return sid_engine.dump_state(psid);
}

int sid_sound_machine_cycle_based(void)
{
    switch (sidengine) {
        case SID_ENGINE_FASTSID:
            return 0;
#ifdef HAVE_RESID
        case SID_ENGINE_RESID:
            return 1;
#endif
#ifdef HAVE_RESID_FP
      case SID_ENGINE_RESID_FP:
        return 1;
#endif
#ifdef HAVE_CATWEASELMKIII
        case SID_ENGINE_CATWEASELMKIII:
            return 0;
#endif
#ifdef HAVE_HARDSID
        case SID_ENGINE_HARDSID:
            return 0;
#endif
#ifdef HAVE_PARSID
        case SID_ENGINE_PARSID:
            return 0;
#endif
#ifdef HAVE_SSI2001
        case SID_ENGINE_SSI2001:
            return 0;
#endif
#ifdef HAVE_USBSID
        case SID_ENGINE_USBSID:
            return 0;
#endif
    }

    return 0;
}

int sid_sound_machine_channels(void)
{
    int channels = 0;

    resources_get_int("SidStereo", &channels);

    return channels + 1;
}

static void set_sound_func(void)
{
    if (sid_enable)
	{
        if (sid_engine_type == SID_ENGINE_FASTSID) {
            sid_read_func = sound_read;
            sid_store_func = sound_store;
            sid_dump_func = sound_dump;
        }
#ifdef HAVE_RESID
        if (sid_engine_type == SID_ENGINE_RESID) {
            sid_read_func = sound_read;
            sid_store_func = sound_store;
            sid_dump_func = sound_dump;
        }
#endif
#ifdef HAVE_RESID_FP
        if (sid_engine_type == SID_ENGINE_RESID_FP) {
            sid_read_func = sound_read;
            sid_store_func = sound_store;
            sid_dump_func = sound_dump;

        }
#endif
#ifdef HAVE_CATWEASELMKIII
        if (sid_engine_type == SID_ENGINE_CATWEASELMKIII) {
            sid_read_func = catweaselmkiii_read;
            sid_store_func = catweaselmkiii_store;
            sid_dump_func = NULL; /* TODO: catweasel dump */
        }
#endif
#ifdef HAVE_HARDSID
        if (sid_engine_type == SID_ENGINE_HARDSID) {
            sid_read_func = hardsid_read;
            sid_store_func = hardsid_store;
            sid_dump_func = NULL; /* TODO: hardsid dump */
        }
#endif
#ifdef HAVE_PARSID
        if (sid_engine_type == SID_ENGINE_PARSID) {
            sid_read_func = parsid_read;
            sid_store_func = parsid_store;
            sid_dump_func = NULL; /* TODO: parsid dump */
        }
#endif
#ifdef HAVE_SSI2001
        if (sid_engine_type == SID_ENGINE_SSI2001) {
            sid_read_func = ssi2001_read;
            sid_store_func = ssi2001_store;
            sid_dump_func = NULL; /* TODO: ssi2001 dump */
        }
#endif
#ifdef HAVE_USBSID
        if (sid_engine_type == SID_ENGINE_USBSID) {
            sid_read_func = usbsid_read;
            sid_store_func = usbsid_store;
            sid_dump_func = NULL; /* TODO: usbsid dump */
        }
#endif
    } else {
        sid_read_func = sid_read_off;
        sid_store_func = sid_write_off;
        sid_dump_func = NULL;
    }
}

void sid_sound_machine_enable(int enable)
{
    sid_enable = enable;

    set_sound_func();
}

int sid_engine_set(int engine)
{
	LOGD("sid_engine_set: engine=%d", engine);
#ifdef HAVE_CATWEASELMKIII
    if (engine == SID_ENGINE_CATWEASELMKIII
        && sid_engine_type != SID_ENGINE_CATWEASELMKIII) {
        if (catweaselmkiii_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_CATWEASELMKIII
        && sid_engine_type == SID_ENGINE_CATWEASELMKIII) {
        catweaselmkiii_close();
    }
#endif
#ifdef HAVE_HARDSID
    if (engine == SID_ENGINE_HARDSID
        && sid_engine_type != SID_ENGINE_HARDSID)
	{
		LOGD("hardsid_open");
        if (hardsid_open() < 0)
		{
            return -1;
        }
    }
    if (engine != SID_ENGINE_HARDSID
        && sid_engine_type == SID_ENGINE_HARDSID) {
        hardsid_close();
    }
#endif
#ifdef HAVE_PARSID
    if ((engine == SID_ENGINE_PARSID)
        && sid_engine_type != engine) {
        if (parsid_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_PARSID
        && sid_engine_type == SID_ENGINE_PARSID) {
        parsid_close();
    }
#endif
#ifdef HAVE_SSI2001
    if (engine == SID_ENGINE_SSI2001
        && sid_engine_type != SID_ENGINE_SSI2001) {
        if (ssi2001_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_SSI2001
        && sid_engine_type == SID_ENGINE_SSI2001) {
        ssi2001_close();
    }
#endif
#ifdef HAVE_USBSID
    if (engine == SID_ENGINE_USBSID
        && sid_engine_type != SID_ENGINE_USBSID) {
        if (usbsid_open() < 0) {
            return -1;
        }
    }
    if (engine != SID_ENGINE_USBSID
        && sid_engine_type == SID_ENGINE_USBSID) {
        usbsid_close();
    }
#endif

    sid_engine_type = engine;

    set_sound_func();

    return 0;
}

void sid_state_read(unsigned int channel, sid_snapshot_state_t *sid_state)
{
    sid_engine.state_read(sound_get_psid(channel), sid_state);
}

void sid_state_write(unsigned int channel, sid_snapshot_state_t *sid_state)
{
    /* VICE 3.10: improved NULL safety with diagnostics */
    if (sid_engine.state_write == NULL) {
        fprintf(stderr, "%s:%d:%s(): sidengine.state_write is NULL\n",
                __FILE__, __LINE__, __func__);
        return;
    }

    sound_t *psid = sound_get_psid(channel);
    if (psid == NULL) {
        fprintf(stderr, "%s:%d:%s(): sound_get_psid() returned NULL\n",
                __FILE__, __LINE__, __func__);
        return;
    }

    sid_engine.state_write(psid, sid_state);
}

void sid_set_machine_parameter(long clock_rate)
{
#ifdef HAVE_CATWEASELMKIII
    catweaselmkiii_set_machine_parameter(clock_rate);
#endif
#ifdef HAVE_HARDSID
    hardsid_set_machine_parameter(clock_rate);
#endif
#ifdef HAVE_USBSID
    usbsid_set_machine_parameter(clock_rate);
#endif
}

/* VICE 3.10: engine/machine max SID utility functions */

int sid_engine_get_max_sids(int engine)
{
    switch (engine) {
        case SID_ENGINE_FASTSID:
            return SID_ENGINE_FASTSID_NUM_SIDS;
        case SID_ENGINE_RESID:
            return SID_ENGINE_RESID_NUM_SIDS;
        case SID_ENGINE_CATWEASELMKIII:
            return SID_ENGINE_CATWEASELMKIII_NUM_SIDS;
        case SID_ENGINE_HARDSID:
            return SID_ENGINE_HARDSID_NUM_SIDS;
        case SID_ENGINE_PARSID:
            return SID_ENGINE_PARSID_NUM_SIDS;
        case SID_ENGINE_USBSID:
            return SID_ENGINE_USBSID_NUM_SIDS;
        default:
            return -1;
    }
}

int sid_machine_get_max_sids(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:
        case VICE_MACHINE_C64SC:
        case VICE_MACHINE_SCPU64:
            return SID_MACHINE_MAX_SID_C64;
        case VICE_MACHINE_C64DTV:
            return SID_MACHINE_MAX_SID_C64DTV;
        case VICE_MACHINE_C128:
            return SID_MACHINE_MAX_SID_C128;
        case VICE_MACHINE_VIC20:
            return SID_MACHINE_MAX_SID_VIC20;
        case VICE_MACHINE_PLUS4:
            return SID_MACHINE_MAX_SID_PLUS4;
        case VICE_MACHINE_PET:
            return SID_MACHINE_MAX_SID_PET;
        case VICE_MACHINE_CBM5x0:
            return SID_MACHINE_MAX_SID_CBM5x0;
        case VICE_MACHINE_CBM6x0:
            return SID_MACHINE_MAX_SID_CBM6x0;
        case VICE_MACHINE_VSID:
            return SID_MACHINE_MAX_SID_VSID;
        default:
            return 0;
    }
}

int sid_machine_engine_get_max_sids(int engine)
{
    int emax = sid_engine_get_max_sids(engine);
    int mmax = sid_machine_get_max_sids();

    return emax < mmax ? emax : mmax;
}

int sid_machine_can_have_multiple_sids(void)
{
    return sid_machine_get_max_sids() > 1;
}

/* c64d helper functions */

int c64d_get_sid_enable()
{
	return sid_enable;
}
