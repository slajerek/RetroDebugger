/*
 * uc2.h - Cartridge handling, Universal Cartridge 2.
 *
 * Written by
 *  Thomas Winkler <t.winkler@aon.at>
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

#ifndef VICE_UC2_H
#define VICE_UC2_H

#include <stdio.h>

#include "types.h"

void uc2_config_init(void);
void uc2_config_setup(uint8_t *rawcart);

int uc15_bin_attach(const char *filename, uint8_t *rawcart);
int uc15_crt_attach(FILE *fd, uint8_t *rawcart);
int uc2_bin_attach(const char *filename, uint8_t *rawcart);
int uc2_crt_attach(FILE *fd, uint8_t *rawcart);
void uc2_detach(void);

uint8_t uc2_roml_read(uint16_t addr);
uint8_t uc2_romh_read(uint16_t addr);
int uc2_roml_no_ultimax_store(uint16_t addr, uint8_t value);
int uc2_romh_phi1_read(uint16_t addr, uint8_t *value);
int uc2_romh_phi2_read(uint16_t addr, uint8_t *value);
void uc2_roml_store(uint16_t addr, uint8_t value);
void uc2_romh_store(uint16_t addr, uint8_t value);

int uc2_peek_mem(export_t *ex, uint16_t addr, uint8_t *value);

uint8_t uc2_1000_7fff_read(uint16_t addr);
void uc2_1000_7fff_store(uint16_t addr, uint8_t value);
uint8_t uc2_a000_bfff_read(uint16_t addr);
void uc2_a000_bfff_store(uint16_t addr, uint8_t value);
uint8_t uc2_c000_cfff_read(uint16_t addr);
void uc2_c000_cfff_store(uint16_t addr, uint8_t value);


void uc2_reset(void);
void uc2_powerup(void);

struct snapshot_s;

int uc2_snapshot_write_module(struct snapshot_s *s);
int uc2_snapshot_read_module(struct snapshot_s *s);

#endif
