/*
 * uc1.c - Cartridge handling, Universal Cartridge 1.
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


/*
#define UC1_DEBUG
*/



#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "c64cart.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64pla.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "ram.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"
#include "vicii-phi1.h"
#include "uc1.h"



#ifdef UC1_DEBUG
#define DBG(x) log_printf x
#else
#define DBG(x)
#endif




#define MAXBANKS 8                     /* ROM banks (16K each) */
#define CART_RAM_SIZE (32 * 1024)      /* RAM size */



#define EXROM(regv)      ((regv & 0x80) ? 1:0)
#define GAME(regv)       ((regv & 0x40) ? 1:0)
#define RAMSEL(regv)     ((regv & 0x20) ? 1:0)
#define RAMWRE(regv)     ((regv & 0x10) ? 1:0)
#define IOENA(regv)      ((regv & 0x08) ? 0:1)




/*
    "UC-1" Cartridge

    - this cart comes in 3 ROM sizes, 32Kb (2 banks), 64Kb (4 banks) and 128Kb (8 banks)..
    - this cart comes with 32K SRAM

    - ROM is after reset mapped in at $8000-$BFFF (16k game).

    - 1 register at io1 / de00:

    Bit 0-2   bank number
    Bit 3 --- IO Register disable (1 - Register is invisible)
    Bit 4 --- SRAM write enable   (1 - SRAM is writable)
    Bit 5 --- SRAM select         (1 - RAM, 0 - EPROM)
    Bit 6 --- Signal /GAME        (C64 Cartridge Mode)
    Bit 7 --- Signal /EXROM       (C64 Cartridge Mode)
*/


#define CRT_OFF      0xC0
#define CRT_8K       0x40
#define CRT_16K      0x00
#define CRT_ULTI     0x80


/*-- UC register and bank mask -----*/
static uint8_t regval   = 0;
static uint8_t bankmask = 0x07;
static uint8_t cmode    = 0;

/*-- for performace reason pre calculated flags -----*/
static int write_ram    = 0;              /* from $4000 to $7FFF and $8000 to $BFFF */
static int read_ram     = 0;

/*-- FAKE ULTIMAX (only while mode write RAM) -----*/
static int fakeUlti     = 0;
static int read_l       = 0;
static int read_h       = 0;


/* 32 KB RAM */
static uint8_t *cart_ram = NULL;




/*
 *-- Set UC register at IO1 -----
 */
static void uc1_io1_storeReg(uint8_t value)
{
    unsigned int wflag = 0;
    uint8_t mode       = 0;
    uint8_t mode_phi1  = 0;
    uint8_t mode_phi2  = 0;

    regval = value;
    cmode  = value & 0xC0;

    if(cmode == CRT_8K) {
        /*-- 8K selected -----*/
        mode      = CMODE_8KGAME;
        read_l    = 1;
        read_h    = 0;
    } else if(cmode == CRT_16K) {
        /*-- 16K selected -----*/
        mode      = CMODE_16KGAME;
        read_l    = 1;
        read_h    = 1;
    } else if(cmode == CRT_ULTI) {
        /*-- UltiMax selected -----*/
        mode      = CMODE_ULTIMAX;
        read_l    = 1;
        read_h    = 1;
    } else {
        /*-- CRT_OFF -----*/
        mode      = CMODE_RAM;
        read_l    = 0;
        read_h    = 0;
    }
    mode_phi1  = mode;                  /* VIC */
    mode_phi2  = mode;                  /* CPU */

    if(RAMWRE(regval) && cmode != CRT_OFF && cart_ram != NULL) {
        /*-- RAM writable -----*/
        mode_phi2 = CMODE_ULTIMAX;      /* FAKE UltiMax */
        wflag    |= CMODE_WRITE;
        wflag    |= CMODE_EXPORT_RAM;
        write_ram = 1;
    } else {
        /*-- RAM readonly -----*/
        wflag    |= CMODE_READ;
        write_ram = 0;
    }

    if(RAMSEL(regval) && cart_ram != NULL) {
        /*-- RAM selected -----*/
        wflag |= CMODE_EXPORT_RAM;
        read_ram = 1;
    } else {
        /*-- ROM selected -----*/
        read_ram = 0;
    }

    cart_config_changed_slotmain(mode_phi1, mode_phi2, wflag);
    cart_romlbank_set_slotmain(value & bankmask);
    cart_romhbank_set_slotmain(value & bankmask);

    if(cmode == CRT_OFF) {
        cart_set_port_game_slotmain(0);
        cart_set_port_exrom_slotmain(0);
        cart_port_config_changed_slotmain();
        write_ram = 0;
        fakeUlti  = 0;
    } else if(write_ram == 0) {
        /*-- normal mode -----*/
        write_ram = 0;
        fakeUlti  = 0;
    } else {
        /*-- SRAM write - fake UltiMax -----*/
        DBG(("-FAKE UTLIMAX-"));
        fakeUlti  = 1;
        write_ram = 1;
    }
    export_ram = write_ram | read_ram;

    DBG(("UC1 reg: %02x bank: %d (%d banks), %s, %s, %s, %s, %s, %s",
        regval, (regval & bankmask), bankmask + 1,
        EXROM(regval) ? "EXROM" : "/EXROM",
        GAME(regval)  ? "GAME" : "/GAME",
        ( EXROM(regval) &&  GAME(regval)) ? "DISABLED" :
        (!EXROM(regval) &&  GAME(regval)) ? "8KB"      :
        (!EXROM(regval) && !GAME(regval)) ? "16KB"     : "Ultimax",
        RAMSEL(regval) ? "RAM"           : "ROM",
        RAMWRE(regval) ? "write enabled" : "write disabled",
        IOENA(regval)  ? "IO enabled"    : "IO disabled")
        );
}

static void resetRegister(void)
{
    uc1_io1_storeReg(0);
}

/*
 *-- Set UC register at IO1 -----
 */
static void uc1_io1_store(uint16_t addr, uint8_t value)
{
    if(IOENA(regval)) {
        uc1_io1_storeReg(value);
    }
}

static uint8_t uc1_io1_peek(uint16_t addr)
{
    return regval;
}

static uint8_t uc1_io1_read(uint16_t addr)
{
    return 0;
}

static int uc1_dump(void)
{
    mon_out("UC1 reg: %02x (bank: %d (%d banks), %s, %s, %s, %s, %s, %s)\n",
            regval, (regval & bankmask), bankmask + 1,
            EXROM(regval) ? "EXROM" : "/EXROM",
            GAME(regval)  ? "GAME" : "/GAME",
            ( EXROM(regval) &&  GAME(regval)) ? "DISABLED" :
            (!EXROM(regval) &&  GAME(regval)) ? "8KB"      :
            (!EXROM(regval) && !GAME(regval)) ? "16KB"     : "Ultimax",
            RAMSEL(regval) ? "RAM"           : "ROM",
            RAMWRE(regval) ? "write enabled" : "write disabled",
            IOENA(regval)  ? "IO enabled"    : "IO disabled"
      );
    return 0;
}

/* ---------------------------------------------------------------------*/


/*
 *-- get cart pointer l -----
 */
static inline uint8_t *get_mem_l(uint16_t addr, bool ram)
{
    if (ram) {
        return cart_ram + (addr & 0x1fff) + ((roml_bank & 1) << 14);
    }
    return roml_banks + ((addr & 0x1fff) + (roml_bank << 13));
}

/*
 *-- get cart pointer h -----
 */
static inline uint8_t *get_mem_h(uint16_t addr, bool ram)
{
    if (ram) {
        return cart_ram + (addr & 0x1fff) + 0x2000 + ((roml_bank & 1) << 14);
    }
    return romh_banks + ((addr & 0x1fff) + (roml_bank << 13));
}

/*
 *-- read ROM-L or RAM-L -----
 */
static inline uint8_t read_cart_l(uint16_t addr)
{
    return *(get_mem_l(addr, read_ram));
}

/*
 *-- read ROM-H or RAM-H -----
 */
static inline uint8_t read_cart_h(uint16_t addr)
{
    return *(get_mem_h(addr, read_ram));
}

/*
 *-- write ROM-L or RAM-L -----
 */
static inline void write_cart_l(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        *(get_mem_l(addr, 1)) = value;
    }
}

/*
 *-- write ROM-H or RAM-H -----
 */
static inline void write_cart_h(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        *(get_mem_h(addr, 1)) = value;
    }
}



/*
 *-- read ROM-L or RAM-L if active -----
 */
uint8_t uc1_roml_read(uint16_t addr)
{
    return read_cart_l(addr);
}

/*
 *-- write ROM-L or RAM-L if active -----
*/
void uc1_roml_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_l(addr, value);
    }
}

/*
 *-- write ROM-L or RAM-L if active -----
*/
int uc1_roml_no_ultimax_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_l(addr, value);
        return 1;
    }
    return 0;
}


/*
 *-- read ROM-H or RAM-H if active -----
 */
uint8_t uc1_romh_read(uint16_t addr)
{
    if(read_h) {
        return read_cart_h(addr);
    }
    return mem_read_without_ultimax(addr);
}

/*
 *-- write RAM-H if active -----
 */
void uc1_romh_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_h(addr, value);
    }
}


/* VIC reads */
int uc1_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    if(read_h) {
        /*-- UltiMax -----*/
        *value = read_cart_h(addr);
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH_NO_ULTIMAX;
}

/* CPU reads */
int uc1_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    if(read_h) {
        /*-- UltiMax -----*/
        *value = read_cart_h(addr);
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH_NO_ULTIMAX;
}


/*
    read from cart memory for monitor (without side effects)

    the majority of carts can use the generic fallback, custom functions
    must be provided by those carts where either:
    - the cart is not in "Main Slot"
    - "fake ultimax" mapping is used
    - memory can not be read without side effects
*/
int uc1_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if(addr >= 0xE000) {
        if(cmode == CRT_ULTI && read_h) {
            *value = uc1_romh_read(addr);
            return CART_READ_VALID;
        }
    } else if(addr >= 0xA000 && addr < 0xC000) {
        if(cmode == CRT_16K && read_h) {
            *value = uc1_romh_read(addr);
            return CART_READ_VALID;
        }
    } else if(addr >= 0x8000) {
        if(read_l) {
            *value = uc1_roml_read(addr);
            return CART_READ_VALID;
        }
    }

    DBG(("uc1_peek_mem(read_through): $%04X", addr));
    return CART_READ_THROUGH;
}

/*
 *-- FAKE ULTIMAX for write into 16K range: $4,$5,$6,$7 -----
 */
void uc1_1000_7fff_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
      if (addr >= 0x6000) {
          write_cart_h(addr, value);
      } else if (addr >= 0x4000) {
          write_cart_l(addr, value);
      }
      if (addr >= 0x8000) {
          DBG(("uc1_1000_7fff_store(): $%04X (%02X)", addr, value));
      }
      mem_store_without_ultimax(addr, value);
    }
}

/*
    $1000-$7fff in ultimax mode - this is always regular ram
*/
uint8_t uc1_1000_7fff_read(uint16_t addr)
{
    if (fakeUlti) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

/*
    $a000 in ultimax mode
*/
uint8_t uc1_a000_bfff_read(uint16_t addr)
{
    if (cmode == CRT_16K) {
        return read_cart_h(addr);
    }
    if (cmode == CRT_8K) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

void uc1_a000_bfff_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_h(addr, value);
    }
}

/*
    $c000 in ultimax mode - this is always regular ram
*/
uint8_t uc1_c000_cfff_read(uint16_t addr)
{
    if (fakeUlti) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

/* ---------------------------------------------------------------------*/

static io_source_t uc1_device = {
    CARTRIDGE_NAME_UC1,        /* name of the device */
    IO_DETACH_CART,            /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,      /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                         /* read is never valid, reg is write only */
    uc1_io1_store,             /* store function */
    NULL,                      /* NO poke function */
    uc1_io1_read,              /* read function */
    uc1_io1_peek,              /* peek function */
    uc1_dump,                  /* device state information dump function */
    CARTRIDGE_UC1,             /* cartridge ID */
    IO_PRIO_NORMAL,            /* normal priority, device read needs to be checked for collisions */
    0,                         /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE             /* NO mirroring */
};


static io_source_list_t *uc1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_UC1, 0, 1, &uc1_device, NULL, CARTRIDGE_UC1
};



/* ---------------------------------------------------------------------*/

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


/* ---------------------------------------------------------------------*/

void uc1_powerup(void)
{
    resetRegister();
}

void uc1_config_init(void)
{
/*
     wflag:
      bit 4  0x10   - trigger nmi after config changed
      bit 3  0x08   - export ram enabled
      bit 2  0x04   - vic phi2 mode (always sees ram if set)
      bit 1  0x02   - release freeze (stop asserting NMI)
      bit 0  0x01   - r/w flag
*/
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);

    resetRegister();
}

void uc1_reset(void)
{
    resetRegister();
}

void uc1_config_setup(uint8_t *rawcart)
{
    int i;

    for (i = 0; i < MAXBANKS; i++) { /* split interleaved low and high banks */
        memcpy(roml_banks + i * 0x2000, rawcart + i * 0x4000, 0x2000);
        memcpy(romh_banks + i * 0x2000, rawcart + i * 0x4000 + 0x2000, 0x2000);
    }
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int uc1_common_attach(void)
{
    uc1_io1_storeReg(regval);

    if (cart_ram == NULL) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
        ram_init_with_pattern(cart_ram, CART_RAM_SIZE, &ramparam);
    }

    if (export_add(&export_res) < 0) {
        return -1;
    }
    uc1_list_item = io_source_register(&uc1_device);
    return 0;
}

int uc1_bin_attach(const char *filename, uint8_t *rawcart)
{
    regval = 0;
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
    return uc1_common_attach();
}

int uc1_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    regval = 0;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if (chip.size == 0x2000) {
            if (chip.bank >= MAXBANKS || !(chip.start == 0x8000 || chip.start == 0xa000 || chip.start == 0xe000)) {
                return -1;
            }
            if (crt_read_chip(rawcart, (chip.bank << 14) | (chip.start & 0x2000), &chip, fd)) {
                return -1;
            }
        } else if (chip.size == 0x4000) {
            if (chip.bank >= MAXBANKS || chip.start != 0x8000) {
                return -1;
            }
            if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
                return -1;
            }
        } else {
            return -1;
        }
    }
    return uc1_common_attach();
}

void uc1_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(uc1_list_item);
    uc1_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   2
#define SNAP_MODULE_NAME  "CARTUC1"

int uc1_snapshot_write_module(snapshot_t *s)
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
        || (SMW_BA(m, romh_banks, 0x2000 * MAXBANKS) < 0)
        || (SMW_BA(m, cart_ram, CART_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int uc1_snapshot_read_module(snapshot_t *s)
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

    if (cart_ram == NULL) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
        if (cart_ram == NULL) {
            return -1;
        }
        ram_init_with_pattern(cart_ram, CART_RAM_SIZE, &ramparam);
    }

    if (0
        || (SMR_B(m, &regval) < 0)
        || (SMR_B(m, &bankmask) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)
        || (SMR_BA(m, romh_banks, 0x2000 * MAXBANKS) < 0)
        || (SMR_BA(m, cart_ram, CART_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (uc1_common_attach() == -1) {
        return -1;
    }
    uc1_io1_store(0xde00, regval);
    return 0;
}
