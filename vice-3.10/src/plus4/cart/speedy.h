
/*
 * speedy.h - Speedy Freezer Cartridge
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_SPEEDY_H
#define VICE_SPEEDY_H

#include "types.h"

uint8_t speedy_c1lo_read(uint16_t addr);

int speedy_kernal_read(uint16_t addr, uint8_t *value);
int speedy_fd00_read(uint16_t addr, uint8_t *value);
int speedy_fe00_read(uint16_t addr, uint8_t *value);

uint8_t *speedy_get_tedmem_base(unsigned int segment);

void speedy_config_setup(uint8_t *rawcart);
int speedy_bin_attach(const char *filename, uint8_t *rawcart);
int speedy_crt_attach(FILE *fd, uint8_t *rawcart);

void speedy_detach(void);

void speedy_reset(void);
void speedy_freeze(void);

int speedy_snapshot_write_module(snapshot_t *s);
int speedy_snapshot_read_module(snapshot_t *s);

#endif
