/*
 * uc1.h - Cartridge handling, Universal Cartridge 1
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

#ifndef VICE_UC1_H
#define VICE_UC1_H

#include <stdio.h>

#include "types.h"

void uc1_config_init(void);
void uc1_config_setup(uint8_t *rawcart);

int uc1_bin_attach(const char *filename, uint8_t *rawcart);
int uc1_crt_attach(FILE *fd, uint8_t *rawcart);
void uc1_detach(void);

uint8_t uc1_roml_read(uint16_t addr);
uint8_t uc1_romh_read(uint16_t addr);
int uc1_roml_no_ultimax_store(uint16_t addr, uint8_t value);
int uc1_romh_phi1_read(uint16_t addr, uint8_t *value);
int uc1_romh_phi2_read(uint16_t addr, uint8_t *value);
void uc1_roml_store(uint16_t addr, uint8_t value);
void uc1_romh_store(uint16_t addr, uint8_t value);

uint8_t uc1_1000_7fff_read(uint16_t addr);
void uc1_1000_7fff_store(uint16_t addr, uint8_t value);
uint8_t uc1_a000_bfff_read(uint16_t addr);
void uc1_a000_bfff_store(uint16_t addr, uint8_t value);
uint8_t uc1_c000_cfff_read(uint16_t addr);
void uc1_c000_cfff_store(uint16_t addr, uint8_t value);

int uc1_peek_mem(export_t *ex, uint16_t addr, uint8_t *value);


void uc1_reset(void);
void uc1_powerup(void);

struct snapshot_s;

int uc1_snapshot_write_module(struct snapshot_s *s);
int uc1_snapshot_read_module(struct snapshot_s *s);

#endif
