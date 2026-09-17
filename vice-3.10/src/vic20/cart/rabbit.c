/*
 * rabbit.c -- VIC20 "Rabbit Tape" Cartridge emulation.
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "rabbit.h"
#include "cartio.h"
#include "cartridge.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "zfile.h"

/* #define DEBUGRABBIT */

#ifdef DEBUGRABBIT
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*

    "Rabbit Tape"

    - 2KiB ROM mapped into IO2/IO3 ($9800-$9BFF, $9C00-9FFF)

    This is a tape fastloader. After SYS38912, the commands *L (load) and
    *S (save) work in direct mode, in the beginning of the command line.

*/

/* ------------------------------------------------------------------------- */


#define CART_ROM_SIZE (0x400 * 2)
static uint8_t *cart_rom = NULL;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t rabbit_io2_read(uint16_t addr);
static uint8_t rabbit_io3_read(uint16_t addr);
static int rabbit_mon_dump(void);

static io_source_t rabbit_io2_device = {
    CARTRIDGE_VIC20_NAME_RABBIT, /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0x9800, 0x9bff, 0x3ff,         /* range for the device */
    1,                             /* read is always valid */
    NULL,                          /* store function */
    NULL,                          /* NO poke function */
    rabbit_io2_read,               /* read function */
    NULL,                          /* NO peek function */
    rabbit_mon_dump,               /* device state information dump function */
    CARTRIDGE_VIC20_RABBIT,        /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0,                             /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                 /* NO mirroring */
};

static io_source_list_t *rabbit_io2_list_item = NULL;

static io_source_t rabbit_io3_device = {
    CARTRIDGE_VIC20_NAME_RABBIT, /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0x9c00, 0x9fff, 0x3ff,         /* range for the device, address is ignored, reg:$9c00, mirrors:$9c01-$9fff */
    1,                             /* read is always valid */
    NULL,            /* store function */
    NULL,                          /* NO poke function */
    rabbit_io3_read,                          /* read function */
    NULL,             /* NO peek function */
    rabbit_mon_dump,             /* device state information dump function */
    CARTRIDGE_VIC20_RABBIT,      /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0,                             /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                 /* NO mirroring */
};

static io_source_list_t *rabbit_io3_list_item = NULL;

static const export_resource_t export_res23 = {
    CARTRIDGE_VIC20_NAME_RABBIT, 0, 0, &rabbit_io2_device, &rabbit_io3_device, CARTRIDGE_VIC20_RABBIT
};

/* ------------------------------------------------------------------------- */

static uint8_t rabbit_io2_read(uint16_t addr)
{
    return cart_rom[0x000 + (addr & 0x3ff)];
}

static uint8_t rabbit_io3_read(uint16_t addr)
{
    return cart_rom[0x400 + (addr & 0x3ff)];
}

/* ------------------------------------------------------------------------- */

static int zfile_load(const char *filename, uint8_t *dest, size_t size)
{
    FILE *fd;
    off_t len;

    fd = zfile_fopen(filename, MODE_READ);
    if (!fd) {
        return -1;
    }
    len = archdep_file_size(fd);
    if (len < 0 || (size_t)len != size) {
        zfile_fclose(fd);
        return -1;
    }
    if (fread(dest, size, 1, fd) < 1) {
        zfile_fclose(fd);
        return -1;
    }
    zfile_fclose(fd);
    return 0;
}

int rabbit_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (crt_read_chip_header(&chip, fd)) {
        goto exiterror;
    }

    DBG(("chip %d at %02x len %02x\n", idx, chip.start, chip.size));
    if (chip.size != 0x0800) {
        goto exiterror;
    }

    if (crt_read_chip(&cart_rom[0], 0, &chip, fd)) {
        goto exiterror;
    }

    if (export_add(&export_res23) < 0) {
        goto exiterror;
    }

    mem_cart_blocks = VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    rabbit_io2_list_item = io_source_register(&rabbit_io2_device);
    rabbit_io3_list_item = io_source_register(&rabbit_io3_device);

    return CARTRIDGE_VIC20_RABBIT;

exiterror:
    rabbit_detach();
    return -1;
}

int rabbit_bin_attach(const char *filename)
{
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (zfile_load(filename, cart_rom, (size_t)CART_ROM_SIZE) < 0) {
        rabbit_detach();
        return -1;
    }

    if (export_add(&export_res23) < 0) {
        return -1;
    }

    mem_cart_blocks = VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    rabbit_io2_list_item = io_source_register(&rabbit_io2_device);
    rabbit_io3_list_item = io_source_register(&rabbit_io3_device);

    return 0;
}

void rabbit_detach(void)
{
    mem_cart_blocks = 0;
    mem_initialize_memory();
    lib_free(cart_rom);
    cart_rom = NULL;

    export_remove(&export_res23);
    if (rabbit_io2_list_item != NULL) {
        io_source_unregister(rabbit_io2_list_item);
        rabbit_io2_list_item = NULL;
    }

    if (rabbit_io3_list_item != NULL) {
        io_source_unregister(rabbit_io3_list_item);
        rabbit_io3_list_item = NULL;
    }
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   0
#define VIC20CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "RABBIT"

int rabbit_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int rabbit_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != VIC20CART_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (0
        || (SMR_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(cart_rom);
        cart_rom = NULL;
        return -1;
    }

    snapshot_module_close(m);

    mem_cart_blocks = VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    return 0;
}

/* ------------------------------------------------------------------------- */

static int rabbit_mon_dump(void)
{
    return 0;
}
