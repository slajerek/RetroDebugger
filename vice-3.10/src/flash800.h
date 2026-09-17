/*
 * flash800.h - (MX)29F800C[TB] Flash emulation (preliminary, incomplete).
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Extended by
 *  Chester Kollschen <mail@knightsofbytes.games>
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
 *  Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA.
 *
 */

#ifndef VICE_FLASH800_H
#define VICE_FLASH800_H

#include "types.h"

enum flash800_type_s {
    /* 800CB */
    FLASH800_TYPE_CB,
    FLASH800_TYPE_NUM
};
typedef enum flash800_type_s flash800_type_t;

enum flash800_state_s {
    FLASH800_STATE_READ,
    FLASH800_STATE_MAGIC_1,
    FLASH800_STATE_MAGIC_2,
    FLASH800_STATE_AUTOSELECT,
    FLASH800_STATE_BYTE_PROGRAM,
    FLASH800_STATE_BYTE_PROGRAM_ERROR,
    FLASH800_STATE_ERASE_MAGIC_1,
    FLASH800_STATE_ERASE_MAGIC_2,
    FLASH800_STATE_ERASE_SELECT,
    FLASH800_STATE_CHIP_ERASE,
    FLASH800_STATE_SECTOR_ERASE,
    FLASH800_STATE_SECTOR_ERASE_TIMEOUT,
    FLASH800_STATE_SECTOR_ERASE_SUSPEND
};
typedef enum flash800_state_s flash800_state_t;

#define FLASH800_ERASE_MASK_SIZE 8

typedef struct flash800_context_s {
    uint8_t *flash_data;
    flash800_state_t flash_state;
    flash800_state_t flash_base_state;

    uint8_t program_byte;
    uint8_t erase_mask[FLASH800_ERASE_MASK_SIZE];
    int flash_dirty;

    flash800_type_t flash_type;

    uint8_t last_read;
    struct alarm_s *erase_alarm;
} flash800_context_t;

struct alarm_context_s;

extern void flash800core_init(struct flash800_context_s *flash800_context,
                              struct alarm_context_s *alarm_context,
                              flash800_type_t type, uint8_t *data);
extern void flash800core_shutdown(struct flash800_context_s *flash800_context);
extern void flash800core_reset(struct flash800_context_s *flash800_context);

extern void flash800core_store(struct flash800_context_s *flash800_context,
                               unsigned int addr, uint8_t data);
extern uint8_t flash800core_read(struct flash800_context_s *flash800_context,
                              unsigned int addr);
extern uint8_t flash800core_peek(struct flash800_context_s *flash800_context,
                              unsigned int addr);

struct snapshot_s;

extern int flash800core_snapshot_write_module(struct snapshot_s *s,
                                              struct flash800_context_s *flash800_context,
                                              const char *name);
extern int flash800core_snapshot_read_module(struct snapshot_s *s,
                                             struct flash800_context_s *flash800_context,
                                             const char *name);

#endif
