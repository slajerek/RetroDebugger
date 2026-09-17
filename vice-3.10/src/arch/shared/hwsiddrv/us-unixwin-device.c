/*
 * us-unixwin-device.c
 *
 * Written by
 *  LouDnl <vice@mail.loudai.nl>
 *
 * Based on vice code written by
 *  Simon White <sidplay2@yahoo.com>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, modified from the other hardware sid sources.
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

#if defined(HAVE_USBSID)

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h> /* TIMESPEC */
#include <time.h> /* TIMESPEC */

#include "alarm.h"
#include "us-unixwin.h"
#include "usbsid.h"
#include "log.h"
#include "maincpu.h"
#include "machine.h"
#include "resources.h"
#include "sid-resources.h"
#include "types.h"

#include "USBSIDInterface.h"

static int rc = -1, sids_found = -1, no_sids = -1;
static int r_audiomode = -1, audiomode = -1;
static int r_readmode = -1, readmode = -1;
static int buffsize = -1, diffsize = -1;
static int r_buffsize = -1, r_diffsize = -1;
const int d_buffsize = 8192, d_diffsize = 64;
static uint8_t sid_registers[0x20 * US_MAXSID];
static int usid_chipno = -1;

static CLOCK usid_main_clk;
static CLOCK usid_alarm_clk;
static alarm_t *usid_alarm = NULL;
static long raster_rate;

static log_t usbsid_log = LOG_DEFAULT;

/* pre declarations */
static void usbsid_alarm_handler(CLOCK offset, void *data);

USBSIDitf usbsid;

void us_device_reset(bool us_reset)
{
    if (sids_found > 0) {
        raster_rate = getrasterrate_USBSID(usbsid);
        usid_main_clk = maincpu_clk;
        usid_alarm_clk = raster_rate;
        alarm_set(usid_alarm, (usid_main_clk + raster_rate));
        if (us_reset) {
            reset_USBSID(usbsid);
            log_message(usbsid_log, "Reset sent");
        }
        no_sids = -1;
        log_message(usbsid_log, "Clocks reset");
    }
    return;
}

int us_device_open(void)
{
    usbsid_log = log_open("USBSID");

    if (!sids_found) {
        return -1;
    }

    if (sids_found > 0) {
        return sids_found;
    }

    sids_found = 0;

    log_debug(usbsid_log, "Detecting boards");

    if (usbsid == NULL) {
        usbsid = create_USBSID();
        if (usbsid) {
            /* Gather settings from resources */
            resources_get_int("SidUSBSIDReadMode", &r_readmode);
            resources_get_int("SidUSBSIDAudioMode", &r_audiomode);
            resources_get_int("SidUSBSIDDiffSize", &r_diffsize);
            resources_get_int("SidUSBSIDBufferSize", &r_buffsize);
            /* Apply settings to local variables */
            readmode = r_readmode;
            audiomode = r_audiomode;
            diffsize = r_diffsize;
            buffsize = r_buffsize;
            /* Update diff and buff size if needed */
            if (diffsize >= 16) {
                setdiffsize_USBSID(usbsid, diffsize);
            }
            if (buffsize >= 256) {
                setbuffsize_USBSID(usbsid, buffsize);
            }
        }

        if (readmode == 1) {
            rc = init_USBSID(usbsid, false, false);  /* threading and cycles disabled */
            if (rc >= 0) {
                log_message(usbsid_log, "Starting in read mode");
            }
        } else {  /*  (readmode == 0 || readmode == -1) */
            /* NOTICE: Digitunes only play with threaded cycles */
            rc = init_USBSID(usbsid, true, true);  /* threading and cycles enabled */
            if (rc >= 0) {
                log_message(usbsid_log, "Starting in normal mode");
            }
        }
        if (rc < 0) {
            return rc;
        }
    }

    log_message(usbsid_log, "Set audio mode to %s", (audiomode == 1 ? "Stereo" : "Mono"));
    setstereo_USBSID(usbsid, (audiomode == 1 ? audiomode : 0));

    usid_alarm = alarm_new(maincpu_alarm_context, "usbsid", usbsid_alarm_handler, NULL);
    sids_found = getnumsids_USBSID(usbsid);
    no_sids = 0;
    log_message(usbsid_log, "Alarm set, reset sids");
    us_device_reset(false);  /* No reset on init! */
    log_message(usbsid_log, "Opened");

    return rc;
}

int us_device_close(void)
{
    log_message(usbsid_log, "Start device closing");
    if (usbsid != NULL) {
        mute_USBSID(usbsid);
        close_USBSID(usbsid);
        usbsid = NULL;
    }

    /* Clean up vars */
    alarm_destroy(usid_alarm);
    usid_alarm = 0;
    sids_found = -1;
    no_sids = -1;
    rc = -1;
    usbsid = NULL;
    log_message(usbsid_log, "Closed");
    return 0;
}

int us_device_read(uint16_t addr, int chipno)
{   /* NOTICE: Disabled, unneeded */
    if (chipno < US_MAXSID) {
        addr = ((addr & 0x1F) + (chipno * 0x20));
        if (readmode == 1) {
            sid_registers[addr] = read_USBSID(usbsid, addr);
        }
        usid_chipno = chipno;
        return sid_registers[addr];
    }
    usid_chipno = -1;
    return 0x0;
}

CLOCK us_delay(void)
{   /* ISSUE: This should return an unsigned 64 bit integer but that makes vice stall indefinately on negative integers */
    if (maincpu_clk < usid_main_clk) {  /* Sync reset */
        usid_main_clk = maincpu_clk;
        return 0;
    }
    /* Without substracting 1 cycle this
     * can cause a clicking noise in cycle exact tunes
     * create audible skips in digitunes like Sky Buster */
    CLOCK cycles = maincpu_clk - usid_main_clk - 1;
    while (cycles > 0xffff) {
        cycles -= 0xffff;
    }
    usid_main_clk = maincpu_clk;
    return cycles;
}

void us_device_store(uint16_t addr, uint8_t val, int chipno) /* max chipno = 1 */
{
    if (chipno < US_MAXSID) {  /* remove 0x20 address limitation */
        addr = ((addr & 0x1F) + (chipno * 0x20));
        if (readmode == 0) {
            CLOCK cycles = us_delay();
            writeringcycled_USBSID(usbsid, addr, val, (uint16_t)cycles);
        } else if (readmode == 1) {
            write_USBSID(usbsid, addr, val);
        }
        usid_chipno = chipno;
        sid_registers[addr] = val;
        return;
    }
    usid_chipno = -1;
    return;
}

void us_set_machine_parameter(long cycles_per_sec)
{
    setclockrate_USBSID(usbsid, cycles_per_sec, true);
    raster_rate = getrasterrate_USBSID(usbsid);
    log_message(usbsid_log, "Clockspeed set to: %ld and rasterrate set to: %ld", cycles_per_sec, raster_rate);
    return;
}

unsigned int us_device_available(void)
{
    log_message(usbsid_log, "%d SIDs available", sids_found);
    return sids_found;
}

void us_set_readmode(int val)
{
    resources_get_int("SidUSBSIDReadMode", &r_readmode);
    if (readmode != val) {
        log_message(usbsid_log, "Set read mode from %d to %d (resource: %d)", readmode, val, r_readmode);
        readmode = val;
        if (val == 0) {
            enablethread_USBSID(usbsid);
        }
        if (val == 1) {
            disablethread_USBSID(usbsid);
        }
    }
    return;
}

void us_set_audiomode(int val)
{   /* Gets set by x64sc from SID settings and by VSID at SID file change */
    resources_get_int("SidUSBSIDAudioMode", &r_audiomode);
    log_message(usbsid_log, "Audio mode is '%s' (resource:%d val:%d)", (r_audiomode == 1 ? "Stereo" : "Mono"), r_audiomode, val);
    audiomode = r_audiomode;

    setstereo_USBSID(usbsid, audiomode);
}

void us_restart_ringbuffer(void)
{   /* Restarts the ringbuffer with a new value */
    if (buffsize != d_buffsize) {
        log_message(usbsid_log, "Restarting ringbuffer with buffer size:%d & diff size:%d", buffsize, diffsize);
        restartringbuffer_USBSID(usbsid);
    }
}

void us_set_buffsize(int val)
{   /* Set the ringbuffer size */
    resources_get_int("SidUSBSIDBufferSize", &r_buffsize);
    buffsize = r_buffsize;
    if (r_buffsize != d_buffsize) {
        log_message(usbsid_log, "Setting ringbuffer size to: %d (val:%d default:%d)", buffsize, val, d_buffsize);
        setbuffsize_USBSID(usbsid, buffsize);
        us_restart_ringbuffer();
    }
}

void us_set_diffsize(int val)
{   /* Set the ringbuffer head to tail difference size */
    resources_get_int("SidUSBSIDDiffSize", &r_diffsize);
    diffsize = r_diffsize;
    if (r_diffsize != d_diffsize) {
        log_message(usbsid_log, "Setting ringbuffer diff size to: %d  (val:%d default:%d)", diffsize, val, d_diffsize);
        setdiffsize_USBSID(usbsid, diffsize);
    }

}

static void usbsid_alarm_handler(CLOCK offset, void *data)
{
    CLOCK cycles = (usid_alarm_clk + offset) - usid_main_clk;

    if (cycles < raster_rate) {
        usid_alarm_clk = usid_main_clk + raster_rate;
    } else {
        setflush_USBSID(usbsid);
        usid_main_clk   = maincpu_clk - offset;
        usid_alarm_clk  = usid_main_clk + raster_rate;
    }
    alarm_set(usid_alarm, usid_alarm_clk);
    return;
}

/* ---------------------------------------------------------------------*/

void us_device_state_read(int chipno, struct sid_us_snapshot_state_s *sid_state)
{
    sid_state->usid_main_clk = usid_main_clk;
    sid_state->usid_alarm_clk = usid_alarm_clk;
    sid_state->lastaccess_chipno = usid_chipno;
}

void us_device_state_write(int chipno, struct sid_us_snapshot_state_s *sid_state)
{
    usid_main_clk = sid_state->usid_main_clk;
    usid_alarm_clk = sid_state->usid_alarm_clk;
    usid_chipno = sid_state->lastaccess_chipno;
}

#endif /* HAVE_USBSID */
