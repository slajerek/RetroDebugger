
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

/*
    Speedy Freezer Cartridge

    - 8KiB mapped to c1lo, mirrored twice so it fills the 16k
    - "Freeze" button, which will cause the following:
      - the circuit "waits" for a (TED-) interrupt to occur
      - now A9 is pulled low, which will cause the IRQ vector being fetched from
        offset 0x1dfe/0x1dff in the ROM
      - this results in the address 0xFD10
      - Code at 0xFD10 jumps to 0xFDF4, where it pages itself into c1l memory

 */

/* #define DEBUG_SPEEDY */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cartridge.h"
#include "cartio.h"
#include "crt.h"
#include "export.h"
#include "log.h"
#include "lib.h"
#include "interrupt.h"
#include "maincpu.h"
#include "monitor.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "snapshot.h"
#include "util.h"

#include "speedy.h"

#ifdef DEBUG_SPEEDY
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

#define SPEEDYROMSIZE    0x2000

static int freeze_active = 0;
static int freeze_active_q = 0;
static int speedy_filetype = 0;

static unsigned char *speedyrom = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_PLUS4_NAME_SPEEDY, 0, PLUS4_CART_C1LO, NULL, NULL, CARTRIDGE_PLUS4_SPEEDY
};

/* ------------------------------------------------------------------------- */

/* read 0x8000...0xbfff */
uint8_t speedy_c1lo_read(uint16_t addr)
{
    unsigned int offset = (addr & 0x1fff);

    /* DBG(("speedy_c1lo_read (freeze:%d) addr:%04x offs:%04x value: %02x",
         freeze_active, addr, offset, speedyrom[offset])); */

    if (freeze_active_q) {
        DBG(("speedy_c1lo_read disabling freeze at addr:%04x", addr));
        freeze_active_q = 0;
    }

    return speedyrom[offset];
}

int speedy_fd00_read(uint16_t addr, uint8_t *value)
{
    unsigned int offset;
    if (freeze_active_q) {
        offset = (addr & 0x1fff);
        offset &= ~0x0200;
        *value = speedyrom[offset];
        DBG(("speedy_fd00_read addr: 0x%04x offs: 0x%04x value:0x%02x", addr, offset, *value));
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

int speedy_fe00_read(uint16_t addr, uint8_t *value)
{
    unsigned int offset;
    if (freeze_active_q) {
        offset = (addr & 0x1fff);
        offset &= ~0x0200;
        *value = speedyrom[offset];
        DBG(("speedy_fe00_read addr: 0x%04x offs: 0x%04x value:0x%02x", addr, offset, *value));
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

/* read 0xc000...0xffff (kernal) */
int speedy_kernal_read(uint16_t addr, uint8_t *value)
{
    unsigned int offset;

    if (freeze_active) {
        if ((addr == 0xfffe) || (addr == 0xffff)) {
                DBG(("speedy_kernal_read set Q (addr: 0x%04x)", addr));
                freeze_active_q = 1;
                freeze_active = 0; /* release button */
        }
    }

    if (freeze_active_q) {
        offset = (addr & 0x1fff);
        offset &= ~0x0200;
        *value = speedyrom[offset];
        DBG(("speedy_kernal_read addr: 0x%04x offs: 0x%04x value:0x%02x", addr, offset, *value));
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

/*
  segment:
    bit 0-1:    2 upper bits of address
    bit 2:      ROM select (0: RAM, 1: ROM)
*/
uint8_t *speedy_get_tedmem_base(unsigned int segment)
{
    return speedyrom;
}

void speedy_reset(void)
{
    DBG(("speedy_reset"));
    freeze_active_q = 0;
    freeze_active = 0;
}

void speedy_freeze(void)
{
    DBG(("speedy_freeze"));
    /* the flipflop will clock in the state of the irq line */
    freeze_active_q = 0;
    freeze_active = 1;
}

void speedy_config_setup(uint8_t *rawcart)
{
    DBG(("speedy_config_setup"));
    memcpy(speedyrom, rawcart, SPEEDYROMSIZE);
}

static int speedy_common_attach(void)
{
    DBG(("speedy_common_attach"));

    if(!(speedyrom = lib_malloc(SPEEDYROMSIZE))) {
        return -1;
    }

    if (export_add(&export_res) < 0) {
        return -1;
    }

    return 0;
}

int speedy_bin_attach(const char *filename, uint8_t *rawcart)
{
    speedy_filetype = 0;

    DBG(("speedy_bin_attach '%s'", filename));

    /* we accept 8KiB images */
    if (util_file_load(filename, rawcart, SPEEDYROMSIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    speedy_filetype = CARTRIDGE_FILETYPE_BIN;
    return speedy_common_attach();
}

int speedy_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    DBG(("speedy_crt_attach"));

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if ((chip.bank >= 1) || (chip.size != 0x2000)) {
        return -1;
    }
    /* DBG(("bank: %d offset: %06x \n", chip.bank, chip.bank << 14)); */

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    speedy_filetype = CARTRIDGE_FILETYPE_CRT;
    return speedy_common_attach();
}

void speedy_detach(void)
{
    DBG(("speedy_detach"));
    export_remove(&export_res);
    lib_free(speedyrom);
    speedyrom = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTSPEEDY snapshot module format:

   type  | name              | version | description
   -------------------------------------------------
   BYTE  | freeze_active     |   0.1+  | was freeze pressed?
   BYTE  | freeze_active_q   |   0.1+  | was freeze activated?
   ARRAY | ROM               |   0.1+  | 8KiB of ROM data
 */

/* FIXME: since we cant actually make snapshots due to TED bugs, the following
          is completely untested */

static const char snap_module_name[] = "CARTSPEEDY";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int speedy_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    DBG(("speedy_snapshot_write_module"));

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)freeze_active) < 0
        || SMW_B(m, (uint8_t)freeze_active_q) < 0
        || SMW_BA(m, speedyrom, SPEEDYROMSIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return 0;
}

int speedy_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    DBG(("speedy_snapshot_read_module"));

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B_INT(m, &freeze_active) < 0
        || SMR_B_INT(m, &freeze_active_q) < 0
        || SMR_BA(m, speedyrom, SPEEDYROMSIZE) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    speedy_common_attach();

    /* set filetype to none */
    speedy_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}

