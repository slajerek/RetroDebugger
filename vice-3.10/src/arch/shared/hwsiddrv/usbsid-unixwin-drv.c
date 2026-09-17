/*
 * usbsid-unixwin-drv.c
 *
 * Written by
 *  LouDnl <vice@mail.loudai.nl>
 *
 * Based on hs-unix-linux.c written by
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

#include "usbsid.h"
#include "us-unixwin.h"
#include "types.h"

static int use_us_device = 0;

int usbsid_drv_open(void)
{
    int rc = 0;
    if (!(rc = us_device_open())) {
        use_us_device = 1;
        return 0;
    }

    return rc;
}

int usbsid_drv_close(void)
{

    if (use_us_device) {
        use_us_device = 0;
        us_device_close();
    }

    return 0;
}

void usbsid_drv_reset(bool us_reset)
{
    if (use_us_device) {
        us_device_reset(us_reset);
    }
}

int usbsid_drv_read(uint16_t addr, int chipno)
{
    if (use_us_device) {
        return us_device_read(addr, chipno);
    }

    return 0;
}

void usbsid_drv_store(uint16_t addr, uint8_t val, int chipno)
{
    if (use_us_device) {
        us_device_store(addr, val, chipno);
    }
}

void usbsid_drv_set_machine_parameter(long cycles_per_sec)
{
    if (use_us_device) {
        us_set_machine_parameter(cycles_per_sec);
    }
}

int usbsid_drv_available(void)
{

    if (use_us_device) {
        return us_device_available();
    }

    return 0;
}

void usbsid_drv_set_readmode(int val)
{
    if (use_us_device) {
        us_set_readmode(val);
    }
}

void usbsid_drv_set_audiomode(int val)
{
    if (use_us_device) {
        us_set_audiomode(val);
    }
}

void usbsid_drv_restart_ringbuffer(void)
{
    if (use_us_device) {
        us_restart_ringbuffer();
    }
}

void usbsid_drv_set_buffsize(int val)
{
    if (use_us_device) {
        us_set_buffsize(val);
    }
}

void usbsid_drv_set_diffsize(int val)
{
    if (use_us_device) {
        us_set_diffsize(val);
    }
}

void usbsid_drv_state_read(int chipno, struct sid_us_snapshot_state_s *sid_state)
{
    if (use_us_device) {
        us_device_state_read(chipno, sid_state);
    }
}

void usbsid_drv_state_write(int chipno, struct sid_us_snapshot_state_s *sid_state)
{
    if (use_us_device) {
        us_device_state_write(chipno, sid_state);
    }
}

#endif /* HAVE_USBSID */
