/*
 * magicdesk16.c - Cartridge handling, Magic Desk 16K cart.
 *
 * Original magicdesk.c Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>

 * 16K Mod Written by
 *  Salvo Cristaldi <crystal@unict.it>
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

/* #define MAGICDESK16_DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "magicdesk16.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef MAGICDESK16_DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    "Magic Desk 16K" Cartridge

    - supports all "Magic Desk Clone" homebrew cart with 16k game config, up to 2 MB

    - ROM is always mapped in at $8000-$BFFF (16k game).

    - 1 register at io1 / de00:

    bit 0-6   bank number
    bit 7     exrom (1 = cart disabled)
*/

#define MAXBANKS 128

static uint8_t regval = 0;
static uint8_t bankmask = 0x7f;

static void magicdesk16_io1_store(uint16_t addr, uint8_t value)
{
    regval = value & (0x80 | bankmask);
    cart_romhbank_set_slotmain(value & bankmask);
    cart_romlbank_set_slotmain(value & bankmask);
    cart_set_port_game_slotmain(0);
    if (value & 0x80) {
        /* turn off cart ROM */
        cart_set_port_exrom_slotmain(0);
        cart_set_port_game_slotmain(0);
    } else {
        cart_set_port_exrom_slotmain(1);
        cart_set_port_game_slotmain(1);
    }
    cart_port_config_changed_slotmain();
    DBG(("MAGICDESK16: Reg: %02x (Bank: %d of %d, %s)\n", regval, (regval & bankmask), bankmask + 1, (regval & 0x80) ? "disabled" : "enabled"));
}

static uint8_t magicdesk16_io1_peek(uint16_t addr)
{
    return regval;
}

static int magicdesk16_dump(void)
{
    mon_out("Reg: %02x (Bank: %d of %d, %s)\n", regval, (regval & bankmask), bankmask + 1, (regval & 0x80) ? "disabled" : "enabled");
    return 0;
}


/* ---------------------------------------------------------------------*/

static io_source_t magicdesk16_device = {
    CARTRIDGE_NAME_MAGIC_DESK_16, /* name of the device */
    IO_DETACH_CART,               /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,         /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                            /* read is never valid, reg is write only */
    magicdesk16_io1_store,        /* store function */
    NULL,                         /* NO poke function */
    NULL,                         /* read function */
    magicdesk16_io1_peek,         /* peek function */
    magicdesk16_dump,             /* device state information dump function */
    CARTRIDGE_MAGIC_DESK_16,      /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0,                            /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                /* NO mirroring */
};

static io_source_list_t *magicdesk16_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MAGIC_DESK_16, 1, 1, &magicdesk16_device, NULL, CARTRIDGE_MAGIC_DESK_16
};

/* ---------------------------------------------------------------------*/

void magicdesk16_config_init(void)
{
    magicdesk16_io1_store((uint16_t)0xde00, 0);
}

void magicdesk16_config_setup(uint8_t *rawcart)
{
    for (int i=0; i< MAXBANKS; i++) {
        memcpy(&roml_banks[i*0x2000], &rawcart[i*0x4000], 0x2000);
        memcpy(&romh_banks[i*0x2000], &rawcart[0x2000 + i*0x4000], 0x2000);
    }

}

/* ---------------------------------------------------------------------*/

static int magicdesk16_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    magicdesk16_list_item = io_source_register(&magicdesk16_device);
    return 0;
}

int magicdesk16_bin_attach(const char *filename, uint8_t *rawcart)
{
    bankmask = 0x7f;
    if (util_file_load(filename, rawcart, 0x200000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        bankmask = 0x3f;
        if (util_file_load(filename, rawcart, 0x100000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            bankmask = 0x1f;
            if (util_file_load(filename, rawcart, 0x80000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                bankmask = 0x0f;
                if (util_file_load(filename, rawcart, 0x40000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                    bankmask = 0x07;
                    if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                        bankmask = 0x03;
                        if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                            bankmask = 0x01;
                            if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                                return -1;
                            }
                        }
                    }
                }
            }
        }
    }
    return magicdesk16_common_attach();
}

int magicdesk16_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int lastbank = 0;
    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if ((chip.bank >= MAXBANKS) || ((chip.start != 0x8000) && (chip.start != 0xa000)) || (chip.size != 0x4000)) {
            return -1;
        }
        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
        if (chip.bank > lastbank) {
            lastbank = chip.bank;
        }
    }
    if (lastbank >= MAXBANKS) {
        /* more than 128 banks does not work */
        return -1;
    } else if (lastbank >= 64) {
        /* min 65, max 128 banks */
        bankmask = 0x7f;
    } else if (lastbank >= 32) {
        /* min 33, max 64 banks */
        bankmask = 0x3f;
    } else if (lastbank >= 16) {
        /* min 17, max 32 banks */
        bankmask = 0x1f;
    } else if (lastbank >= 8) {
        /* min 9, max 16 banks */
        bankmask = 0x0f;
    } else if (lastbank >= 4) {
        /* min 5, max 8 banks */
        bankmask = 0x07;
    } else if (lastbank >= 2) {
        /* min 3, max 4 banks */
        bankmask = 0x03;
    } else {
        /* max 2 banks */
        bankmask = 0x01;
    }

    return magicdesk16_common_attach();
}

void magicdesk16_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(magicdesk16_list_item);
    magicdesk16_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   2
#define SNAP_MODULE_NAME  "CARTMD16"

int magicdesk16_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)regval) < 0)
        || (SMW_B(m, (uint8_t)bankmask) < 0)
        || (SMW_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)
        || (SMW_BA(m, romh_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int magicdesk16_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_B(m, &regval) < 0)
        || (SMR_B(m, &bankmask) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)
        || (SMR_BA(m, romh_banks, 0x2000 * MAXBANKS) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (magicdesk16_common_attach() == -1) {
        return -1;
    }
    magicdesk16_io1_store(0xde00, regval);
    return 0;
}
