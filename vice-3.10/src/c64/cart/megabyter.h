/*
 * megabyter.h - Cartridge handling of the megabyter cart.
 *
 * Written by
 *  Chester Kollschen <mail@chesterkollschen.com>
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

#ifndef VICE_MEGABYTER_H
#define VICE_MEGABYTER_H

#include "types.h"

extern int megabyter_resources_init(void);
extern void megabyter_resources_shutdown(void);
extern int megabyter_cmdline_options_init(void);

extern uint8_t megabyter_roml_read(uint16_t addr);
extern void megabyter_roml_store(uint16_t addr, uint8_t value);
extern void megabyter_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

extern void megabyter_config_init(void);
extern void megabyter_config_setup(uint8_t *rawcart);
extern int megabyter_bin_attach(const char *filename, uint8_t *rawcart);
extern int megabyter_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
extern void megabyter_detach(void);
extern int megabyter_bin_save(const char *filename);
extern int megabyter_crt_save(const char *filename);
extern int megabyter_flush_image(void);

struct snapshot_s;

extern int megabyter_snapshot_write_module(struct snapshot_s *s);
extern int megabyter_snapshot_read_module(struct snapshot_s *s);

#endif
