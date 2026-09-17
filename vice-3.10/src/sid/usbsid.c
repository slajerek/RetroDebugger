/*
 * usbsid.c - Generic usbsid abstraction layer.
 *
* Written by
 *  LouDnl <vice@mail.loudai.nl>
 *
 * Based on hardsid.c by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifdef HAVE_USBSID

#include <stdio.h>
#include <string.h>

#include "usbsid.h"
#include "log.h"
#include "sid-snapshot.h"
#include "types.h"


#define DEBUG_USBSID
/* define to trace usbsid stuff without having a usbsid */
/* #define DEBUG_USBSID_DUMMY */

#ifdef DEBUG_USBSID
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

#if defined(DEBUG_USBSID_DUMMY)

#define usbsid_drv_available() 1
#define usbsid_drv_reset(bool us_reset) log_printf("[USBSID] usbsid_drv_reset")
#define usbsid_drv_open() (log_printf("[USBSID] usbsid_drv_open"), 0)
#define usbsid_drv_close() log_printf("[USBSID] usbsid_drv_close")
#define usbsid_drv_read(addr, chipno)  (log_printf("[USBSID] usbsid_drv_read addr:%02x chip:%d", addr, chipno), 1)
#define usbsid_drv_store(addr, val, chipno) log_printf("[USBSID] usbsid_drv_store addr:%02x val:%02x chip:%d", addr, val, chipno)
#define usbsid_drv_state_read(chipno, sid_state) log_printf("[USBSID] usbsid_drv_state_read chip:%d sid_state:%p", chipno, sid_state)
#define usbsid_drv_state_write(chipno, sid_state) log_printf("[USBSID] usbsid_drv_state_write chip:%d sid_state:%p", chipno, sid_state)
#define usbsid_drv_set_readmode(val) log_printf("[USBSID] usbsid_drv_set_readmode read_mode:%p", val)

#endif

static int usbsid_is_open = -1;

/* Buffer containing current register state of SIDs */
static uint8_t sidbuf[0x20 * US_MAXSID];

int usbsid_open(void)
{
    DBG(("usbsid_open"));
    if (usbsid_is_open) {
        usbsid_is_open = usbsid_drv_open();
        if(usbsid_is_open >= 0) {
            memset(sidbuf, 0, sizeof(sidbuf));
        }
    }

    if (usbsid_is_open == -2) {
        log_error(LOG_DEFAULT, "[USBSID] Failed to open USBSID");
    }

    return usbsid_is_open;
}

int usbsid_close(void)
{
    if (!usbsid_is_open) {
        usbsid_drv_close();
        usbsid_is_open = -1;
    }
    return usbsid_is_open;
}

void usbsid_reset(bool us_reset)
{
    if (!usbsid_is_open) {
        usbsid_drv_reset(us_reset);
    }
}

int usbsid_read(uint16_t addr, int chipno)
{
    if (!usbsid_is_open && chipno < US_MAXSID) {
        int val = usbsid_drv_read(addr, chipno);
        sidbuf[(chipno * 0x20) + addr] = (uint8_t)val;
        return val;
    }

    return usbsid_is_open;
}

void usbsid_store(uint16_t addr, uint8_t val, int chipno)
{
    if (!usbsid_is_open && chipno < US_MAXSID) {
        /* write to sidbuf[] for write-only registers */
        sidbuf[(chipno * 0x20) + addr] = val;
        usbsid_drv_store(addr, val, chipno);
    }
}

void usbsid_set_machine_parameter(long cycles_per_sec)
{
    usbsid_drv_set_machine_parameter(cycles_per_sec);
}

/* return 0 if usbsid is not available, != 0 if it is */
int usbsid_available(void)
{
    if (usbsid_is_open < 0) {
        usbsid_open();
        /* usbsid_is_open is expected >= 0 now */
    }

    if (usbsid_is_open >= 0) {
        DBG(("usbsid_available: %d (calling usbsid_drv_available)", usbsid_is_open));
        return usbsid_drv_available();
    }
    DBG(("usbsid_available: %d", 0));
    return 0;
}

void usbsid_set_readmode(int val)
{
    if (!usbsid_is_open) {
        usbsid_drv_set_readmode(val);
    }
}

void usbsid_set_audiomode(int val)
{
    if (!usbsid_is_open) {
        usbsid_drv_set_audiomode(val);
    }
}

void usbsid_restart_ringbuffer(void)
{
    if (!usbsid_is_open) {
        usbsid_drv_restart_ringbuffer();
    }
}

void usbsid_set_buffsize(int val)
{
    if (!usbsid_is_open) {
        usbsid_drv_set_buffsize(val);
    }
}

void usbsid_set_diffsize(int val)
{
    if (!usbsid_is_open) {
        usbsid_drv_set_diffsize(val);
    }
}


/* ---------------------------------------------------------------------*/

void usbsid_state_read(int chipno, struct sid_us_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < US_MAXSID) {
        for (i = 0; i < 32; ++i) {
            sid_state->regs[i] = sidbuf[i + (chipno * 0x20)];
        }
        usbsid_drv_state_read(chipno, sid_state);
    }
}

void usbsid_state_write(int chipno, struct sid_us_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < US_MAXSID) {
        for (i = 0; i < 32; ++i) {
            sidbuf[i + (chipno * 0x20)] = sid_state->regs[i];
        }
        usbsid_drv_state_write(chipno, sid_state);
    }
}
#else
int usbsid_available(void)
{
    return 0;
}

#endif /* HAVE_USBSID */
