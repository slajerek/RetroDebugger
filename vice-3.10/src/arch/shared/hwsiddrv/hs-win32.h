/*
 * hs-win32.h
 *
 * Written by
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

#ifndef VICE_HS_WIN32_H
#define VICE_HS_WIN32_H

#include "sid-snapshot.h"
#include "types.h"

int hs_dll_open(void);
int hs_dll_close(void);
void hs_dll_reset(void);
int hs_dll_read(uint16_t addr, int chipno);
void hs_dll_store(uint16_t addr, uint8_t val, int chipno);
int hs_dll_available(void);
void hs_dll_set_device(unsigned int chipno, unsigned int device);
void hs_dll_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hs_dll_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);

#endif
