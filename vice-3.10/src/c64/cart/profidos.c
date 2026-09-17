/*
 * profidos.c - Cartridge handling, Profi-DOS cart.
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
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "cartio.h"
#include "cartridge.h"
#include "log.h"
#include "maincpu.h"
#include "monitor.h"
#include "profidos.h"
#include "export.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define DEBUG_PROFIDOS */

#ifdef DEBUG_PROFIDOS
#define DBG(x) log_printf x
#else
#define DBG(x)
#endif

/*
    Profi-DOS (REX Datentechnik)

    16K ROM (27128)

    The ROM is connected like this:

    Port    EPROM

      D7 => D6
      D6 => D4
      D5 => D7
      D4 => D5
      D3 => D0
      D2 => D3
      D1 => D1
      D0 => D2

      A0 => A0
      A1 => A1
      A2 => A2
      A3 => A3
      A4 => A4
      A5 => A5
      A6 => A6
      A7 => A7
      A8 => A12
      A9 => A10
     A10 => A11
     A11 => A9
     A12 => A8
     A13 => A13

    After reorganizing the ROM data as shown above, the first 8k bank of the ROM
    contains the code. The second bank is used as a lookup table, and wired so
    that when D0 becomes active (=0), then a counter is started which counts 4
    cycles. If during those 4 cycles D1 becomes active (=0), then the EPROM is
    enabled and the cartridge enables ultimax mode for ROMH. This way the cart
    can patch the kernal, without having an explicit enable mechanism - which
    makes it hard to dump (you'll have to know the addresses where this happens)

    Any access (read or write) to IO2 ($dfxx) will disable the cartridge again.
*/

static int enabled = 0;
static CLOCK check_cycles = 0;

static void profidos_io2_store(uint16_t addr, uint8_t value);
static uint8_t profidos_io2_read(uint16_t addr);
static int profidos_dump(void);

static io_source_t profidos_io2_device = {
    CARTRIDGE_NAME_PROFIDOS, /* name of the device */
    IO_DETACH_CART,          /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,   /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,    /* range for the device, regs:$df00-$dfff */
    0,                       /* read is never valid */
    profidos_io2_store,      /* store function */
    NULL,                    /* NO poke function */
    profidos_io2_read,       /* read function */
    NULL,                    /* NO peek function */
    profidos_dump,           /* device state information dump function */
    CARTRIDGE_PROFIDOS,      /* cartridge ID */
    IO_PRIO_NORMAL,          /* normal priority, device read needs to be checked for collisions */
    0,                       /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE           /* NO mirroring */
};

static const export_resource_t export_res = {
    CARTRIDGE_NAME_PROFIDOS, 0, 1, NULL, &profidos_io2_device, CARTRIDGE_PROFIDOS
};

static io_source_list_t *profidos_io2_list_item = NULL;

/* ---------------------------------------------------------------------*/

static int profidos_dump(void)
{
    mon_out("enabled: %s\n", enabled ? "yes" : "no");
    return 0;
}

static void profidos_io2_store(uint16_t addr, uint8_t value)
{
    DBG(("%08lx profidos_io2_store %04x %02x", maincpu_clk, addr, value));
    enabled = 0;
}

static uint8_t profidos_io2_read(uint16_t addr)
{
    DBG(("%08lx profidos_io2_read %04x", maincpu_clk, addr));
    enabled = 0;
    return 0;
}

uint8_t profidos_romh_read_hirom(uint16_t addr)
{
    static int lastenabled = -1;

    /* second ROM bank contains 0xfe (D0 active), 0xfd (D1 active)
       at the enable address

        e386: fe    e387: fd
        e5a8: fe    e5a9: fd
        e5ea: fe    e5eb: fd
        eb82: fe    eb83: fd
        eb8f: fe    eb90: fd
        eb91: fe    eb92: fd
        eb9f: fe    eba0: fd
        eba1: fe    eba2: fd
        f1cb: fe    f1cc: fd
        f3d5: fe    f3d6: fd
        f4c4: fe    f4c5: fd
        f615: fe    f616: fd
        fcea: fe    fceb: fd
        fd65: fe    fd66: fd
        fe73: fe    fe74: fd

     */

    if ((romh_banks[0x2000 + (addr & 0x1fff)] & 1) == 0) {
        /* D0 is active -> start the enable counter */
        check_cycles = maincpu_clk;
        DBG(("%08lx profidos_romh_read_hirom %04x enabled:%d %02x",
                   maincpu_clk, addr, enabled, romh_banks[0x2000 + (addr & 0x1fff)]));
    }
    if ((romh_banks[0x2000 + (addr & 0x1fff)] & 2) == 0) {
        /* D1 is active -> enable the ROM */
        if ((maincpu_clk - check_cycles) <= 4) {
            enabled = 1;
        }
        DBG(("%08lx profidos_romh_read_hirom %04x enabled:%d %02x",
                   maincpu_clk, addr, enabled, romh_banks[0x2000 + (addr & 0x1fff)]));
    }

    if (enabled != lastenabled) {
        DBG(("%08lx profidos_romh_read_hirom %04x enabled:%d %02x",
                   maincpu_clk, addr, enabled, romh_banks[0x2000 + (addr & 0x1fff)]));
        lastenabled = enabled;
    }

    if (enabled) {
        return romh_banks[(addr & 0x1fff)];
    }

    return mem_read_without_ultimax(addr);
}

int profidos_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    return CART_READ_C64MEM;
}

int profidos_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    return profidos_romh_phi1_read(addr, value);
}

int profidos_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if (addr >= 0xe000) {
        *value = romh_banks[addr & 0x1fff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

void profidos_config_init(void)
{
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
}

void profidos_reset(void)
{
    enabled = 0;
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

void profidos_config_setup(uint8_t *rawcart)
{
    memcpy(romh_banks, &rawcart[0], 0x4000);
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int profidos_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    profidos_io2_list_item = io_source_register(&profidos_io2_device);

    return 0;
}

int profidos_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return profidos_common_attach();
}

int profidos_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i, banks = 0;

    for (i = 0; i <= 2; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 2 || chip.size != 0x2000) {
            break;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            break;
        }
        ++banks;
    }

    if (banks != 2)  {
        return -1;
    }

    return profidos_common_attach();
}

void profidos_detach(void)
{
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

/* CARTPROFIDOS snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | ROMH | $4000 BYTES of ROMH data
 */

static const char snap_module_name[] = "CARTPROFIDOS";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int profidos_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, romh_banks, 0x4000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int profidos_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_BA(m, romh_banks, 0x4000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return profidos_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
