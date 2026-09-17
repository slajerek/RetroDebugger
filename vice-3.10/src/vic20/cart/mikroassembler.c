/*
 * mikroassembler.c -- VIC20 Mikroassembler emulation.
 *
 * Written by
 *  groepaz
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

/* #define DEBUGCART */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mikroassembler.h"
#include "mem.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "zfile.h"

#ifdef DEBUGCART
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

/*
    Mikroassembler

    4KiB ROM in Block 3 (6000-6FFF)
    4KiB ROM in Block 5 (A000-AFFF)
    3KiB RAM in RAM123 (0400-0FFF)

*/


/* ------------------------------------------------------------------------- */

#define CART_RAM_SIZE (0x400 * 3)
static uint8_t *cart_ram = NULL;
#define CART_ROM_SIZE (0x400 * 8)
static uint8_t *cart_rom = NULL;

static log_t mikroassembler_log = LOG_DEFAULT;

static const export_resource_t export_res = {
    CARTRIDGE_VIC20_NAME_MIKRO_ASSEMBLER, 0, VIC_CART_RAM123 | VIC_CART_BLK3 | VIC_CART_BLK5, NULL, NULL, CARTRIDGE_VIC20_MIKRO_ASSEMBLER
};

/* ------------------------------------------------------------------------- */

/* read 0x0400-0x0fff (ram 0x0400 - 0x0fff) */
uint8_t mikroassembler_ram123_read(uint16_t addr)
{
    return cart_ram[(addr & 0x0fff) - 0x400];
}

/* store 0x0400-0x0fff (ram 0x0400 - 0x0fff) */
void mikroassembler_ram123_store(uint16_t addr, uint8_t value)
{
    cart_ram[(addr & 0x0fff) - 0x400] = value;
}

/* read 0x6000-0x6fff */
uint8_t mikroassembler_blk3_read(uint16_t addr)
{
    return cart_rom[addr & 0x0fff];
}

/* read 0xa000-0xafff */
uint8_t mikroassembler_blk5_read(uint16_t addr)
{
    return cart_rom[(addr & 0x0fff) + 0x1000];
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

/* ------------------------------------------------------------------------- */

/* FIXME: this still needs to be tweaked to match the hardware */
static RAMINITPARAM ramparam = {
    .start_value = 255,
    .value_invert = 2,
    .value_offset = 1,

    .pattern_invert = 0x100,
    .pattern_invert_value = 255,

    .random_start = 0,
    .random_repeat = 0,
    .random_chance = 0,
};

static void allocate_rom_ram(void)
{
    if (!cart_ram) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
    }
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }
}

static void clear_ram(void)
{
    if (cart_ram) {
        DBG(("clear_ram: cart_ram"));
        ram_init_with_pattern(cart_ram, CART_RAM_SIZE, &ramparam);
    }
}

void mikroassembler_init(void)
{
    if (mikroassembler_log == LOG_DEFAULT) {
        mikroassembler_log = log_open(CARTRIDGE_VIC20_NAME_MIKRO_ASSEMBLER);
    }
}

void mikroassembler_config_setup(uint8_t *rawcart)
{
}

int mikroassembler_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    allocate_rom_ram();
    clear_ram();

    /* first chip */
    if (crt_read_chip_header(&chip, fd)) {
        goto exiterror;
    }

    /* FIXME: first chip should go to 0x6000, second to 0xa000 */

    DBG(("chip %d at %02x len %02x", idx, chip.start, chip.size));
    if (chip.size != 0x1000) {
        goto exiterror;
    }

    if (crt_read_chip(&cart_rom[0], 0, &chip, fd)) {
        goto exiterror;
    }

    /* second chip */
    if (crt_read_chip_header(&chip, fd)) {
        goto exiterror;
    }

    DBG(("chip %d at %02x len %02x", idx, chip.start, chip.size));
    if (chip.size != 0x1000) {
        goto exiterror;
    }

    if (crt_read_chip(&cart_rom[0x1000], 0, &chip, fd)) {
        goto exiterror;
    }

    if (export_add(&export_res) < 0) {
        goto exiterror;
    }

    mem_cart_blocks = VIC_CART_RAM123 | VIC_CART_BLK3 | VIC_CART_BLK5;
    mem_initialize_memory();

    return CARTRIDGE_VIC20_MIKRO_ASSEMBLER;

exiterror:
    mikroassembler_detach();
    return -1;
}

int mikroassembler_bin_attach(const char *filename)
{
    allocate_rom_ram();
    clear_ram();

    if (zfile_load(filename, cart_rom, (size_t)CART_ROM_SIZE) < 0) {
        mikroassembler_detach();
        return -1;
    }

    if (export_add(&export_res) < 0) {
        return -1;
    }

    mem_cart_blocks = VIC_CART_RAM123 | VIC_CART_BLK3 | VIC_CART_BLK5;
    mem_initialize_memory();

    return 0;
}

void mikroassembler_detach(void)
{
    mem_cart_blocks = 0;
    mem_initialize_memory();
    lib_free(cart_ram);
    lib_free(cart_rom);
    cart_ram = NULL;
    cart_rom = NULL;

    export_remove(&export_res);
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   1
#define VIC20CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "MIKROASSEMBLER"

int mikroassembler_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, cart_ram, CART_RAM_SIZE) < 0)
        || (SMW_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int mikroassembler_snapshot_read_module(snapshot_t *s)
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

    if (!cart_ram) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
    }
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (0
        || (SMR_BA(m, cart_ram, CART_RAM_SIZE) < 0)
        || (SMR_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(cart_ram);
        lib_free(cart_rom);
        cart_ram = NULL;
        cart_rom = NULL;
        return -1;
    }

    snapshot_module_close(m);

    mem_cart_blocks = VIC_CART_RAM123 | VIC_CART_BLK3 | VIC_CART_BLK5;
    mem_initialize_memory();

    return 0;
}
