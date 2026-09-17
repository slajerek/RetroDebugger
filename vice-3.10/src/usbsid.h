/*
 * usbsid.h
 *
 * Written by
 *  LouDnl <vice@mail.loudai.nl>
 *
 * Based on hardsid.h by
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
#ifndef VICE_USBSID_H
#define VICE_USBSID_H

#ifdef HAVE_USBSID
#include "sid-snapshot.h"
#include "types.h"

#define US_MAXSID 4

int usbsid_open(void);
int usbsid_close(void);
void usbsid_reset(bool us_reset);
int usbsid_read(uint16_t addr, int chipno);
void usbsid_store(uint16_t addr, uint8_t val, int chipno);
void usbsid_set_machine_parameter(long cycles_per_sec);
int usbsid_available(void);
void usbsid_set_audiomode(int val);
void usbsid_set_readmode(int val);
void usbsid_restart_ringbuffer(void);
void usbsid_set_buffsize(int val);
void usbsid_set_diffsize(int val);

int usbsid_drv_open(void);
int usbsid_drv_close(void);
void usbsid_drv_reset(bool us_reset);
int usbsid_drv_read(uint16_t addr, int chipno);
void usbsid_drv_store(uint16_t addr, uint8_t val, int chipno);
void usbsid_drv_set_machine_parameter(long cycles_per_sec);
int usbsid_drv_available(void);
void usbsid_drv_set_audiomode(int val);
void usbsid_drv_set_readmode(int val);
void usbsid_drv_restart_ringbuffer(void);
void usbsid_drv_set_buffsize(int val);
void usbsid_drv_set_diffsize(int val);

void usbsid_state_read(int chipno, struct sid_us_snapshot_state_s *sid_state);
void usbsid_state_write(int chipno, struct sid_us_snapshot_state_s *sid_state);

void usbsid_drv_state_read(int chipno, struct sid_us_snapshot_state_s *sid_state);
void usbsid_drv_state_write(int chipno, struct sid_us_snapshot_state_s *sid_state);

#endif /* HAVE_USBSID */
#endif /* VICE_USBSID_H */
