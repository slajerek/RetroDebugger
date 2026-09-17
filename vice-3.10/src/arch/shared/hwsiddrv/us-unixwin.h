/*
 * us-unixwin.h - Linux USBSID specific prototypes.
 *
 * Written by
 *  LouDnl <vice@mail.loudai.nl>
 *
 * Based on hs-unix.h written by
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

#ifndef VICE_US_UNIXWIN_H
#define VICE_US_UNIXWIN_H

#include "sid-snapshot.h"
#include "types.h"

#define US_MAXSID 4

int us_device_open(void);

int us_device_close(void);

void us_device_reset(bool us_reset);

int us_device_read(uint16_t addr, int chipno);

CLOCK us_delay(void);

void us_device_store(uint16_t addr, uint8_t val, int chipno);

void us_set_machine_parameter(long cycles_per_sec);

unsigned int us_device_available(void);

void us_set_readmode(int val);

void us_set_audiomode(int val);

void us_restart_ringbuffer(void);

void us_set_buffsize(int val);

void us_set_diffsize(int val);

void us_device_state_read(int chipno, struct sid_us_snapshot_state_s *sid_state);

void us_device_state_write(int chipno, struct sid_us_snapshot_state_s *sid_state);

#endif /* VICE_US_UNIXWIN_H */
