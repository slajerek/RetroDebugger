/*
 * flash800core.c - (MX)29F800C[TB] Flash emulation (preliminary, incomplete).
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Extended by
 *  Marko Makela <marko.makela@iki.fi>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "flash800.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "snapshot.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* #define FLASH_DEBUG_ENABLED */

#ifdef FLASH_DEBUG_ENABLED
#define FLASH_DEBUG(x) log_printf x
#else
#define FLASH_DEBUG(x)
#endif

struct flash_types_s {
    uint8_t manufacturer_ID;
    uint8_t device_ID;
    uint8_t device_ID_addr;
    unsigned int size;
    unsigned int sector_mask;
    unsigned int sector_size;
    unsigned int sector_shift;
    unsigned int magic_1_addr;
    unsigned int magic_2_addr;
    unsigned int magic_1_mask;
    unsigned int magic_2_mask;
    uint8_t status_toggle_bits;
    unsigned int erase_sector_timeout_cycles;
    unsigned int erase_sector_cycles;
    unsigned int erase_chip_cycles;
};
typedef struct flash_types_s flash_types_t;

static const flash_types_t flash_types[FLASH800_TYPE_NUM] = {
    /* MX29F800CB */
    { 0xc2, 0x58, 1,
      0x100000,
      0x0f0000, 0x010000, 16,
      0xaaa, 0x555, 0xfff, 0xfff,
      0x40,
      40, 700000, 8000000}, /* may take up to 15s and 32s */
};

/* -------------------------------------------------------------------------- */

inline static int flash_magic_1(flash800_context_t *flash800_context, unsigned int addr)
{
    return ((addr & flash_types[flash800_context->flash_type].magic_1_mask) == flash_types[flash800_context->flash_type].magic_1_addr);
}

inline static int flash_magic_2(flash800_context_t *flash800_context, unsigned int addr)
{
    return ((addr & flash_types[flash800_context->flash_type].magic_2_mask) == flash_types[flash800_context->flash_type].magic_2_addr);
}

inline static void flash_clear_erase_mask(flash800_context_t *flash800_context)
{
    int i;

    for (i = 0; i < FLASH800_ERASE_MASK_SIZE; ++i) {
        flash800_context->erase_mask[i] = 0;
    }
}

inline static unsigned int flash_sector_to_addr(flash800_context_t *flash800_context, unsigned int sector)
{
    unsigned int sector_size = flash_types[flash800_context->flash_type].sector_size;

    return sector * sector_size;
}

inline static unsigned int flash_addr_to_sector_number(flash800_context_t *flash800_context, unsigned int addr)
{
    unsigned int sector_addr = flash_types[flash800_context->flash_type].sector_mask & addr;
    unsigned int sector_shift = flash_types[flash800_context->flash_type].sector_shift;

    return sector_addr >> sector_shift;
}

inline static void flash_add_sector_to_erase_mask(flash800_context_t *flash800_context, unsigned int addr)
{
    unsigned int sector_num = flash_addr_to_sector_number(flash800_context, addr);

    flash800_context->erase_mask[sector_num >> 3] |= (uint8_t)(1 << (sector_num & 0x7));
}

inline static void flash_erase_sector(flash800_context_t *flash800_context, unsigned int sector)
{
    unsigned int sector_size = flash_types[flash800_context->flash_type].sector_size;
    unsigned int sector_addr;

    sector_addr = flash_sector_to_addr(flash800_context, sector);

    FLASH_DEBUG(("Erasing 0x%x - 0x%x", sector_addr, sector_addr + sector_size - 1));
    memset(&(flash800_context->flash_data[sector_addr]), 0xff, sector_size);
    flash800_context->flash_dirty = 1;
}

inline static void flash_erase_chip(flash800_context_t *flash800_context)
{
    FLASH_DEBUG(("Erasing chip"));
    memset(flash800_context->flash_data, 0xff, flash_types[flash800_context->flash_type].size);
    flash800_context->flash_dirty = 1;
}

inline static int flash_program_byte(flash800_context_t *flash800_context, unsigned int addr, uint8_t byte)
{
    uint8_t old_data = flash800_context->flash_data[addr];
    uint8_t new_data = old_data & byte;

    FLASH_DEBUG(("Programming 0x%05x with 0x%02x (%02x->%02x)", addr, byte, old_data, old_data & byte));
    flash800_context->program_byte = byte;
    flash800_context->flash_data[addr] = new_data;
    flash800_context->flash_dirty = 1;

    return (new_data == byte) ? 1 : 0;
}

inline static int flash_write_operation_status(flash800_context_t *flash800_context)
{
    return ((flash800_context->program_byte ^ 0x80) & 0x80)   /* DQ7 = inverse of programmed data */
           | (((unsigned int)maincpu_clk & 2) << 5)           /* DQ6 = toggle bit (2 us) */
           | (1 << 5)                                         /* DQ5 = timeout */
    ;
}

inline static int flash_erase_operation_status(flash800_context_t *flash800_context)
{
    int v;

    /* DQ6 = toggle bit */
    v = flash800_context->program_byte;

    /* toggle the toggle bit(s) */
    /* FIXME better toggle bit II emulation */
    flash800_context->program_byte ^= flash_types[flash800_context->flash_type].status_toggle_bits;

    /* DQ3 = sector erase timer */
    if (flash800_context->flash_state != FLASH800_STATE_SECTOR_ERASE_TIMEOUT) {
        v |= 0x08;
    }

    return v;
}

/* -------------------------------------------------------------------------- */

static void erase_alarm_handler(CLOCK offset, void *data)
{
    unsigned int i, j;
    uint8_t m;
    flash800_context_t *flash800_context = (flash800_context_t *)data;

    alarm_unset(flash800_context->erase_alarm);

    FLASH_DEBUG(("Erase alarm, state %i", (int)flash800_context->flash_state));

    switch (flash800_context->flash_state) {
        case FLASH800_STATE_SECTOR_ERASE_TIMEOUT:
            alarm_set(flash800_context->erase_alarm, maincpu_clk + flash_types[flash800_context->flash_type].erase_sector_cycles);
            flash800_context->flash_state = FLASH800_STATE_SECTOR_ERASE;
            break;
        case FLASH800_STATE_SECTOR_ERASE:
            for (i = 0; i < (8 * FLASH800_ERASE_MASK_SIZE); ++i) {
                j = i >> 3;
                m = (uint8_t)(1 << (i & 0x7));
                if (flash800_context->erase_mask[j] & m) {
                    flash_erase_sector(flash800_context, i);
                    flash800_context->erase_mask[j] &= (uint8_t) ~m;
                    break;
                }
            }

            for (i = 0, m = 0; i < FLASH800_ERASE_MASK_SIZE; ++i) {
                m |= flash800_context->erase_mask[i];
            }

            if (m != 0) {
                alarm_set(flash800_context->erase_alarm, maincpu_clk + flash_types[flash800_context->flash_type].erase_sector_cycles);
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
            }
            break;

        case FLASH800_STATE_CHIP_ERASE:
            flash_erase_chip(flash800_context);
            flash800_context->flash_state = flash800_context->flash_base_state;
            break;

        default:
            FLASH_DEBUG(("Erase alarm - error, state %i unhandled!", (int)flash800_context->flash_state));
            break;
    }
}

/* -------------------------------------------------------------------------- */

static void flash800core_store_internal(flash800_context_t *flash800_context,
                                        unsigned int addr, uint8_t byte)
{
#ifdef FLASH_DEBUG_ENABLED
    flash800_state_t old_state = flash800_context->flash_state;
    flash800_state_t old_base_state = flash800_context->flash_base_state;
#endif

    switch (flash800_context->flash_state) {
        case FLASH800_STATE_READ:
            if (flash_magic_1(flash800_context, addr) && (byte == 0xaa)) {
                flash800_context->flash_state = FLASH800_STATE_MAGIC_1;
            }
            break;

        case FLASH800_STATE_MAGIC_1:
            if (flash_magic_2(flash800_context, addr) && (byte == 0x55)) {
                flash800_context->flash_state = FLASH800_STATE_MAGIC_2;
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
            }
            break;

        case FLASH800_STATE_MAGIC_2:
            if (flash_magic_1(flash800_context, addr)) {
                switch (byte) {
                    case 0x90:
                        flash800_context->flash_state = FLASH800_STATE_AUTOSELECT;
                        flash800_context->flash_base_state = FLASH800_STATE_AUTOSELECT;
                        break;
                    case 0xf0:
                        flash800_context->flash_state = FLASH800_STATE_READ;
                        flash800_context->flash_base_state = FLASH800_STATE_READ;
                        break;
                    case 0xa0:
                        flash800_context->flash_state = FLASH800_STATE_BYTE_PROGRAM;
                        break;
                    case 0x80:
                        flash800_context->flash_state = FLASH800_STATE_ERASE_MAGIC_1;
                        break;
                    default:
                        flash800_context->flash_state = flash800_context->flash_base_state;
                        break;
                }
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
            }
            break;

        case FLASH800_STATE_BYTE_PROGRAM:
            if (flash_program_byte(flash800_context, addr, byte)) {
                /* The byte program time is short enough to ignore */
                flash800_context->flash_state = flash800_context->flash_base_state;
            } else {
                flash800_context->flash_state = FLASH800_STATE_BYTE_PROGRAM_ERROR;
            }
            break;

        case FLASH800_STATE_ERASE_MAGIC_1:
            if (flash_magic_1(flash800_context, addr) && (byte == 0xaa)) {
                flash800_context->flash_state = FLASH800_STATE_ERASE_MAGIC_2;
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
            }
            break;

        case FLASH800_STATE_ERASE_MAGIC_2:
            if (flash_magic_2(flash800_context, addr) && (byte == 0x55)) {
                flash800_context->flash_state = FLASH800_STATE_ERASE_SELECT;
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
            }
            break;

        case FLASH800_STATE_ERASE_SELECT:
            if (flash_magic_1(flash800_context, addr) && (byte == 0x10)) {
                flash800_context->flash_state = FLASH800_STATE_CHIP_ERASE;
                flash800_context->program_byte = 0;
                alarm_set(flash800_context->erase_alarm, maincpu_clk + flash_types[flash800_context->flash_type].erase_chip_cycles);
            } else if (byte == 0x30) {
                flash_add_sector_to_erase_mask(flash800_context, addr);
                flash800_context->program_byte = 0;
                flash800_context->flash_state = FLASH800_STATE_SECTOR_ERASE_TIMEOUT;
                alarm_set(flash800_context->erase_alarm, maincpu_clk + flash_types[flash800_context->flash_type].erase_sector_timeout_cycles);
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
            }
            break;

        case FLASH800_STATE_SECTOR_ERASE_TIMEOUT:
            if (byte == 0x30) {
                flash_add_sector_to_erase_mask(flash800_context, addr);
            } else {
                flash800_context->flash_state = flash800_context->flash_base_state;
                flash_clear_erase_mask(flash800_context);
                alarm_unset(flash800_context->erase_alarm);
            }
            break;

        case FLASH800_STATE_SECTOR_ERASE:
            /* TODO not all models support suspending */
            if (byte == 0xb0) {
                flash800_context->flash_state = FLASH800_STATE_SECTOR_ERASE_SUSPEND;
                alarm_unset(flash800_context->erase_alarm);
            }
            break;

        case FLASH800_STATE_SECTOR_ERASE_SUSPEND:
            if (byte == 0x30) {
                flash800_context->flash_state = FLASH800_STATE_SECTOR_ERASE;
                alarm_set(flash800_context->erase_alarm, maincpu_clk + flash_types[flash800_context->flash_type].erase_sector_cycles);
            }
            break;

        case FLASH800_STATE_BYTE_PROGRAM_ERROR:
        case FLASH800_STATE_AUTOSELECT:
            if (flash_magic_1(flash800_context, addr) && (byte == 0xaa)) {
                flash800_context->flash_state = FLASH800_STATE_MAGIC_1;
            }
            if (byte == 0xf0) {
                flash800_context->flash_state = FLASH800_STATE_READ;
                flash800_context->flash_base_state = FLASH800_STATE_READ;
            }
            break;

        case FLASH800_STATE_CHIP_ERASE:
        default:
            break;
    }

    FLASH_DEBUG(("Write %02x to %05x, state %i->%i (base state %i->%i)", byte, addr, (int)old_state, (int)flash800_context->flash_state, (int)old_base_state, (int)flash800_context->flash_base_state));
}

/* -------------------------------------------------------------------------- */

void flash800core_store(flash800_context_t *flash800_context, unsigned int addr, uint8_t byte)
{
    if (maincpu_rmw_flag) {
        maincpu_clk--;
        flash800core_store_internal(flash800_context, addr, flash800_context->last_read);
        maincpu_clk++;
    }

    flash800core_store_internal(flash800_context, addr, byte);
}

uint8_t flash800core_read(flash800_context_t *flash800_context, unsigned int addr)
{
    uint8_t value;
#ifdef FLASH_DEBUG_ENABLED
    flash800_state_t old_state = flash800_context->flash_state;
#endif

    switch (flash800_context->flash_state) {
        case FLASH800_STATE_AUTOSELECT:

            if ((addr & 0xff) == 0) {
                value = flash_types[flash800_context->flash_type].manufacturer_ID;
            } else if ((addr & 0xff) == flash_types[flash800_context->flash_type].device_ID_addr) {
                value = flash_types[flash800_context->flash_type].device_ID;
            } else if ((addr & 0xff) == 2) {
                value = 0;
            } else {
                value = flash800_context->flash_data[addr];
            }
            break;

        case FLASH800_STATE_BYTE_PROGRAM_ERROR:
            value = flash_write_operation_status(flash800_context);
            break;

        case FLASH800_STATE_SECTOR_ERASE_SUSPEND:
        case FLASH800_STATE_CHIP_ERASE:
        case FLASH800_STATE_SECTOR_ERASE:
        case FLASH800_STATE_SECTOR_ERASE_TIMEOUT:
            value = flash_erase_operation_status(flash800_context);
            break;

        default:
        /* The state doesn't reset if a read occurs during a command sequence */
        /* fall through */
        case FLASH800_STATE_READ:
            value = flash800_context->flash_data[addr];
            break;
    }

#ifdef FLASH_DEBUG_ENABLED
    if (old_state != FLASH800_STATE_READ) {
        FLASH_DEBUG(("Read %02x from %05x, state %i->%i", value, addr, (int)old_state, (int)flash800_context->flash_state));
    }
#endif

    flash800_context->last_read = value;
    return value;
}

uint8_t flash800core_peek(flash800_context_t *flash800_context, unsigned int addr)
{
    return flash800_context->flash_data[addr];
}

void flash800core_reset(flash800_context_t *flash800_context)
{
    FLASH_DEBUG(("Reset"));
    flash800_context->flash_state = FLASH800_STATE_READ;
    flash800_context->flash_base_state = FLASH800_STATE_READ;
    flash800_context->program_byte = 0;
    flash_clear_erase_mask(flash800_context);
    alarm_unset(flash800_context->erase_alarm);
}

void flash800core_init(struct flash800_context_s *flash800_context,
                       struct alarm_context_s *alarm_context,
                       flash800_type_t type, uint8_t *data)
{
    FLASH_DEBUG(("Init"));
    flash800_context->flash_data = data;
    flash800_context->flash_type = type;
    flash800_context->flash_state = FLASH800_STATE_READ;
    flash800_context->flash_base_state = FLASH800_STATE_READ;
    flash800_context->program_byte = 0;
    flash_clear_erase_mask(flash800_context);
    flash800_context->flash_dirty = 0;
    flash800_context->erase_alarm = alarm_new(alarm_context, "Flash800Alarm", erase_alarm_handler, flash800_context);
}

void flash800core_shutdown(flash800_context_t *flash800_context)
{
    FLASH_DEBUG(("Shutdown"));
}

/* -------------------------------------------------------------------------- */

#define FLASH800_DUMP_VER_MAJOR   2
#define FLASH800_DUMP_VER_MINOR   0

int flash800core_snapshot_write_module(snapshot_t *s, flash800_context_t *flash800_context, const char *name)
{
    snapshot_module_t *m;
    uint8_t state, base_state;

    m = snapshot_module_create(s, name, FLASH800_DUMP_VER_MAJOR, FLASH800_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    state = (uint8_t)(flash800_context->flash_state);
    base_state = (uint8_t)(flash800_context->flash_base_state);

    if (0
        || (SMW_B(m, state) < 0)
        || (SMW_B(m, base_state) < 0)
        || (SMW_B(m, flash800_context->program_byte) < 0)
        || (SMW_BA(m, flash800_context->erase_mask, FLASH800_ERASE_MASK_SIZE) < 0)
        || (SMW_B(m, flash800_context->last_read) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int flash800core_snapshot_read_module(snapshot_t *s, flash800_context_t *flash800_context, const char *name)
{
    uint8_t vmajor, vminor, state, base_state;
    snapshot_module_t *m;

    m = snapshot_module_open(s, name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != FLASH800_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_B(m, &state) < 0)
        || (SMR_B(m, &base_state) < 0)
        || (SMR_B(m, &(flash800_context->program_byte)) < 0)
        || (SMR_BA(m, flash800_context->erase_mask, FLASH800_ERASE_MASK_SIZE) < 0)
        || (SMR_B(m, &(flash800_context->last_read)) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    flash800_context->flash_state = (flash800_state_t)state;
    flash800_context->flash_base_state = (flash800_state_t)base_state;

    /* Restore alarm if needed */
    switch (flash800_context->flash_state) {
        case FLASH800_STATE_SECTOR_ERASE_TIMEOUT:
        case FLASH800_STATE_SECTOR_ERASE:
        case FLASH800_STATE_CHIP_ERASE:
            /* the alarm timing is not saved, just use some value for now */
            alarm_set(flash800_context->erase_alarm, maincpu_clk + flash_types[flash800_context->flash_type].erase_sector_cycles);
            break;

        default:
            break;
    }

    return 0;
}
