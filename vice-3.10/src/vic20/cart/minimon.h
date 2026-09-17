/*
 * minimon.h -- VIC20 "Minimon" Cartridge emulation.
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

#ifndef VICE_MINIMON_H
#define VICE_MINIMON_H

#include <stdio.h>

#include "types.h"

void minimon_config_setup(uint8_t *rawcart);

const char *minimon_get_file_name(void);

int minimon_bin_attach(const char *filename, uint8_t *rawcart);
/* int minimon_bin_attach(const char *filename); */

int minimon_crt_attach(FILE *fd, uint8_t *rawcart);

int minimon_bin_save(const char *filename);
int minimon_crt_save(const char *filename);
int minimon_flush_image(void);

void minimon_detach(void);

void minimon_powerup(void);
void minimon_reset(void);
void minimon_freeze(void);

int minimon_cart_enabled(void);

int minimon_enable(void);
int minimon_disable(void);

int minimon_blk5_read(uint16_t addr, uint8_t *value);

struct snapshot_s;

int minimon_snapshot_write_module(struct snapshot_s *s);
int minimon_snapshot_read_module(struct snapshot_s *s);

int minimon_cmdline_options_init(void);

int minimon_resources_init(void);
void minimon_resources_shutdown(void);

#endif
