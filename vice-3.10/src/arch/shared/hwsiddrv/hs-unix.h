/*
 * hs-unix.h - Linux hardsid specific prototypes.
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

#ifndef VICE_HS_UNIX_H
#define VICE_HS_UNIX_H

#include "sid-snapshot.h"
#include "types.h"

#define HS_MAXSID 2

int hs_linux_open(void);
int hs_isa_open(void);
int hs_pci_open(void);

int hs_linux_close(void);
int hs_isa_close(void);
int hs_pci_close(void);

void hs_linux_reset(void);

int hs_linux_read(uint16_t addr, int chipno);
int hs_isa_read(uint16_t addr, int chipno);
int hs_pci_read(uint16_t addr, int chipno);

void hs_linux_store(uint16_t addr, uint8_t val, int chipno);
void hs_isa_store(uint16_t addr, uint8_t val, int chipno);
void hs_pci_store(uint16_t addr, uint8_t val, int chipno);

int hs_linux_available(void);
int hs_isa_available(void);
int hs_pci_available(void);

void hs_linux_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hs_isa_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hs_pci_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);

void hs_linux_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hs_isa_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hs_pci_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);

#endif
