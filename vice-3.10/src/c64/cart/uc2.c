/*
 * uc2.c - Cartridge handling, Universal Cartridge 2
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




/* #define UC2_DEBUG */




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
#include "uc2.h"



#ifdef UC2_DEBUG
#define DBG(x) log_printf x
#else
#define DBG(x)
#endif




#define MAXBANKS 32                    /* ROM banks (16K each, 512K) */
#define CART_RAM_SIZE (512 * 1024)     /* RAM size 512K */



#define EXROM(regv)      ((regv & 0x80) ? 1:0)
#define GAME(regv)       ((regv & 0x40) ? 1:0)
#define RAMSEL(regv)     ((regv & 0x20) ? 1:0)
#define RAMWRE(regv)     ((regv & 0x10) ? 1:0)
#define IOENA(regv)      ((regv & 0x08) ? 0:1)
#define MAXM(regv)       ((regv & 0x04) ? 1:0)




/*
    "UC-2" Cartridge

      this cart comes in 3 ROM sizes, 128Kb (8 banks), 256Kb (16 banks) and 512Kb (32 banks)..
    - this cart comes with 512K SRAM

    - ROM is after reset mapped in at $8000-$BFFF (16k game cart).

    - 2 register at io1 / De02,DE03.
      (register are write and readable)

    Register A (IO1:$DE02):
    Bit 0-4   bank number

    Register B (IO1:$DE03):
    Bit 2     MAX machine flag
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


/*-- UC register and bank mask ----- */
static uint8_t regA     = 0;
static uint8_t regB     = 0;
static uint8_t bankmask = 0x1F;
static uint8_t cmode    = 0;
static uint8_t ucmode   = 2;

/*-- for performace reason pre calculated flags ----- */
static int write_ram    = 0;              /* from $4000 to $7FFF and $8000 to $BFFF */
static int read_ram     = 0;

/*-- FAKE ULTIMAX (only while mode write RAM) ----- */
static int fakeUlti     = 0;
static int read_l       = 0;
static int read_h       = 0;


/* 512 KB RAM */
static uint8_t *cart_ram = NULL;




/*
 *-- Set UC register A -----
 */
static inline void uc2_io1_storeA(uint8_t value)
{
    regA  = value & 0x1F;
    cart_romlbank_set_slotmain(regA & bankmask);
    cart_romhbank_set_slotmain(regA & bankmask);

    DBG(("UC2 reg-A: %02x (bank: %d (%d banks))",
        regA, (regA & bankmask), bankmask + 1)
       );
}

/*
 *-- Set UC register B -----
 */
static inline void uc2_io1_storeB(uint8_t value)
{
    unsigned int wflag = 0;
    uint8_t mode       = 0;
    uint8_t mode_phi1  = 0;
    uint8_t mode_phi2  = 0;

    regB   = value;
    cmode  = value & 0xC0;

    if(cmode == CRT_8K) {
        /*-- 8K selected ----- */
        mode      = CMODE_8KGAME;
        read_l    = 1;
        read_h    = 0;
    } else if(cmode == CRT_16K) {
        /*-- 16K selected ----- */
        mode      = CMODE_16KGAME;
        read_l    = 1;
        read_h    = 1;
    } else if(cmode == CRT_ULTI) {
        /*-- UltiMax selected ----- */
        mode      = CMODE_ULTIMAX;
        read_l    = 1;
        read_h    = 1;
    } else {
        /*-- CRT_OFF ----- */
        mode      = CMODE_RAM;
        read_l    = 0;
        read_h    = 0;
    }
    mode_phi1  = mode;                  /* VIC */
    mode_phi2  = mode;                  /* CPU */

    if(RAMWRE(regB) && cmode != CRT_OFF && cart_ram != NULL) {
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
    if(RAMSEL(regB)  && cart_ram != NULL) {
        /*-- RAM selected -----*/
        wflag |= CMODE_EXPORT_RAM;
        read_ram = 1;
    } else {
        /*-- ROM selected -----*/
        read_ram = 0;
    }

    cart_config_changed_slotmain(mode_phi1, mode_phi2, wflag);
    cart_romlbank_set_slotmain(regA & bankmask);
    cart_romhbank_set_slotmain(regA & bankmask);

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

    DBG(("UC2 reg-B: %02x  %s, %s, %s, %s, %s, %s", regB,
        EXROM(regB) ? "EXROM" : "/EXROM",
        GAME(regB)  ? "GAME" : "/GAME",
        ( EXROM(regB) &&  GAME(regB)) ? "DISABLED" :
        (!EXROM(regB) &&  GAME(regB)) ? "8KB"      :
        (!EXROM(regB) && !GAME(regB)) ? "16KB"     : "Ultimax",
        RAMSEL(regB) ? "RAM"           : "ROM",
        RAMWRE(regB) ? "write enabled" : "write disabled",
        IOENA(regB)  ? "IO enabled"    : "IO disabled")
        );
}
static void resetRegister(void)
{
    uc2_io1_storeA(0);
    uc2_io1_storeB(0);
}

/*
 *-- Set UC register at IO1 -----
 */
static void uc2_io1_store(uint16_t addr, uint8_t value)
{
    if(IOENA(regB)) {
        switch(addr & 0x3) {
            case 2:         /* register A */
                uc2_io1_storeA(value);
                break;
            case 3:         /* register B */
                uc2_io1_storeB(value);
                break;
        }
    }
}

static uint8_t uc2_io1_peek(uint16_t addr)
{
    if(IOENA(regB)) {
        switch(addr & 0x3) {
            case 2:         /* register A */
                return(regA);
            case 3:         /* register B */
                return(regB);
        }
    }
    return 0xff;
}
static uint8_t uc15_io1_peek(uint16_t addr)
{
    return 0xff;
}

static uint8_t uc2_io1_read(uint16_t addr)
{
    return uc2_io1_peek(addr);
}
static uint8_t uc15_io1_read(uint16_t addr)
{
    return uc15_io1_peek(addr);
}

static int uc2_dump(void)
{
    mon_out("UC2 reg: %02x (bank: %d (%d banks), %s, %s, %s, %s, %s, %s)\n",
            regA, (regA & bankmask), bankmask + 1,
            EXROM(regB) ? "EXROM" : "/EXROM",
            GAME(regB)  ? "GAME" : "/GAME",
            ( EXROM(regB) &&  GAME(regB)) ? "DISABLED" :
            (!EXROM(regB) &&  GAME(regB)) ? "8KB"      :
            (!EXROM(regB) && !GAME(regB)) ? "16KB"     : "Ultimax",
            RAMSEL(regB) ? "RAM"           : "ROM",
            RAMWRE(regB) ? "write enabled" : "write disabled",
            IOENA(regB)  ? "IO enabled"    : "IO disabled"
      );
    return 0;
}

/* ---------------------------------------------------------------------*/


/*
 *-- get cart pointer l -----
 */
static inline uint8_t *get_mem_l(uint16_t addr, bool ram)
{
    if (ram && cart_ram != NULL) {
        return cart_ram + (addr & 0x1fff) + (regA << 14);
    }
    return roml_banks + ((addr & 0x1fff) + (regA << 13));
}

/*
 *-- get cart pointer h -----
 */
static inline uint8_t *get_mem_h(uint16_t addr, bool ram)
{
    if (ram && cart_ram != NULL) {
        return cart_ram + (addr & 0x1fff) + 0x2000 + (regA << 14);
    }
    return romh_banks + ((addr & 0x1fff) + (regA << 13));
}

/*
 *-- read ROM-L or RAM-L -----
 */
static inline uint8_t read_cart_l(uint16_t addr)
{
#ifdef UC2_DEBUG2
    if (read_ram) {
        if((addr & 0xFFF) == 0) {
            DBG(("read RAM-L: $%04X  (%02X)", addr, *(get_mem_l(addr, read_ram))));
        }
    }
#endif
    return *(get_mem_l(addr, read_ram));
}

/*
 *-- read ROM-H or RAM-H -----
 */
static inline uint8_t read_cart_h(uint16_t addr)
{
#ifdef UC2_DEBUG2
    if (read_ram) {
        if((addr & 0xFFF) == 0) {
            DBG(("read RAM-H: $%04X  (%02X)", addr, *(get_mem_h(addr, read_ram))));
        }
    }
#endif
    return *(get_mem_h(addr, read_ram));
}

/*
 *-- write ROM-L or RAM-L -----
 */
static inline void write_cart_l(uint16_t addr, uint8_t value)
{
    if (write_ram) {
#ifdef UC2_DEBUG
        if((addr & 0xFFF) == 0) {
            DBG(("write RAM-L: $%04X, %02X", addr, value));
        }
#endif
      *(get_mem_l(addr, 1)) = value;
    }
}

/*
 *-- write ROM-H or RAM-H -----
 */
static inline void write_cart_h(uint16_t addr, uint8_t value)
{
    if (write_ram) {
#ifdef UC2_DEBUG
        if((addr & 0xFFF) == 0) {
            DBG(("write RAM-H: $%04X, %02X", addr, value));
        }
#endif
      *(get_mem_h(addr, 1)) = value;
    }
}



/*
 *-- read ROM-L or RAM-L if active -----
 */
uint8_t uc2_roml_read(uint16_t addr)
{
    return read_cart_l(addr);
}

/*
 *-- write ROM-L or RAM-L if active -----
*/
void uc2_roml_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_l(addr, value);
    }
}

/*
 *-- write ROM-L or RAM-L if active -----
*/
int uc2_roml_no_ultimax_store(uint16_t addr, uint8_t value)
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
uint8_t uc2_romh_read(uint16_t addr)
{
    if(read_h) {
        return read_cart_h(addr);
    }
    return mem_read_without_ultimax(addr);
}

/*
 *-- write RAM-H if active -----
 */
void uc2_romh_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_h(addr, value);
    }
}


/* VIC reads */
int uc2_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    if(read_h) {
        /*-- UltiMax -----*/
        /*-- ROM-H: 2000-3FFF,6000-7FFF,A000-BFFF,E000-FFFF -----*/
        *value = read_cart_h(addr);
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH_NO_ULTIMAX;
}
/* CPU reads */
int uc2_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    if(read_h) {
        /*-- UltiMax -----*/
        /*-- ROM-H: 2000-3FFF,6000-7FFF,A000-BFFF,E000-FFFF -----*/
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
int uc2_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if(addr >= 0xE000) {
        /*-- Kernal: E000-FFFF -----*/
        if(cmode == CRT_ULTI && read_h) {
            *value = uc2_romh_read(addr);
            return CART_READ_VALID;
        }
    } else if(addr >= 0xA000 && addr < 0xC000) {
        /*-- ROM-H: A000-BFFF -----*/
        if(cmode == CRT_16K && read_h) {
            *value = uc2_romh_read(addr);
            return CART_READ_VALID;
        }
    } else if(addr >= 0x8000) {
        /*-- ROM-L: 8000-9FFF -----*/
        if(read_l) {
            *value = uc2_roml_read(addr);
            return CART_READ_VALID;
        }
    }

    DBG(("uc2_peek_mem(read_through): $%04X ", addr));
    return CART_READ_THROUGH;
}

/*
 *-- FAKE ULTIMAX for write into 16K range: $4,$5,$6,$7 -----
 */
void uc2_1000_7fff_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        if (addr >= 0x6000) {
            write_cart_h(addr, value);
        } else if (addr >= 0x4000) {
            write_cart_l(addr, value);
        }
        if (addr >= 0x8000) {
            DBG(("uc2_1000_7fff_store(): $%04X (%02X)", addr, value));
        }
        mem_store_without_ultimax(addr, value);
    }
}

/*
    $1000-$7fff in ultimax mode - this is always regular ram
*/
uint8_t uc2_1000_7fff_read(uint16_t addr)
{
    if (fakeUlti) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

/*
    $a000 in ultimax mode
*/
uint8_t uc2_a000_bfff_read(uint16_t addr)
{
    if (cmode == CRT_16K) {
        return read_cart_h(addr);
    }
    if (cmode == CRT_8K) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

void uc2_a000_bfff_store(uint16_t addr, uint8_t value)
{
    if (write_ram) {
        write_cart_h(addr, value);
    }
}

/*
    $c000 in ultimax mode - this is always regular ram
*/
uint8_t uc2_c000_cfff_read(uint16_t addr)
{
    if (fakeUlti) {
        return mem_read_without_ultimax(addr);
    }
    return vicii_read_phi1();
}

/* ---------------------------------------------------------------------*/

static io_source_t uc2_device = {
    CARTRIDGE_NAME_UC2,        /* name of the device */
    IO_DETACH_CART,            /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,      /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                         /* read is never valid, reg is write only */
    uc2_io1_store,             /* store function */
    NULL,                      /* NO poke function */
    uc2_io1_read,              /* read function */
    uc2_io1_peek,              /* peek function */
    uc2_dump,                  /* device state information dump function */
    CARTRIDGE_UC2,             /* cartridge ID */
    IO_PRIO_NORMAL,            /* normal priority, device read needs to be checked for collisions */
    0,                         /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE             /* NO mirroring */
};
static const export_resource_t export_res_uc2 = {
    CARTRIDGE_NAME_UC2, 0, 1, &uc2_device, NULL, CARTRIDGE_UC2
};

static io_source_t uc15_device = {
    CARTRIDGE_NAME_UC15,       /* name of the device */
    IO_DETACH_CART,            /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,      /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                         /* read is never valid, reg is write only */
    uc2_io1_store,             /* store function */
    NULL,                      /* NO poke function */
    uc15_io1_read,             /* read function */
    uc15_io1_peek,             /* peek function */
    uc2_dump,                  /* device state information dump function */
    CARTRIDGE_UC15,            /* cartridge ID */
    IO_PRIO_NORMAL,            /* normal priority, device read needs to be checked for collisions */
    0,                         /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE             /* NO mirroring */
};
static const export_resource_t export_res_uc15 = {
    CARTRIDGE_NAME_UC15, 0, 1, &uc15_device, NULL, CARTRIDGE_UC15
};


static io_source_list_t *uc_list_item = NULL;


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

void uc2_powerup(void)
{
    resetRegister();
}

void uc2_config_init(void)
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

void uc2_reset(void)
{
    resetRegister();
}

void uc2_config_setup(uint8_t *rawcart)
{
    int i;

    for (i = 0; i < MAXBANKS; i++) { /* split interleaved low and high banks */
        memcpy(roml_banks + i * 0x2000, rawcart + i * 0x4000, 0x2000);
        memcpy(romh_banks + i * 0x2000, rawcart + i * 0x4000 + 0x2000, 0x2000);
    }
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int uc2_common_attach(void)
{
    uc2_io1_storeA(regA);
    uc2_io1_storeB(regB);

    if (cart_ram == NULL) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
        ram_init_with_pattern(cart_ram, CART_RAM_SIZE, &ramparam);
    }

    if(ucmode == 2) {
        if (export_add(&export_res_uc2) < 0) {
            return -1;
        }
        uc_list_item = io_source_register(&uc2_device);
    } else {
        if (export_add(&export_res_uc15) < 0) {
            return -1;
        }
        uc_list_item = io_source_register(&uc15_device);
    }
    return 0;
}

static int bin_attach(const char *filename, uint8_t *rawcart)
{
    bankmask = 0x1f;
    if (util_file_load(filename, rawcart, 0x80000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        bankmask = 0x0f;
        if (util_file_load(filename, rawcart, 0x40000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            bankmask = 0x07;
            if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
        }
    }
    return uc2_common_attach();
}

static int crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

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
    return uc2_common_attach();
}


int uc2_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("UC2 attach BIN file"));
    ucmode = 2;
    return bin_attach(filename, rawcart);
}

int uc15_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("UC15 attach BIN file"));
    ucmode = 1;
    return bin_attach(filename, rawcart);
}

int uc2_crt_attach(FILE *fd, uint8_t *rawcart)
{
    DBG(("UC2 attach CRT file"));
    ucmode = 2;
    return crt_attach(fd, rawcart);
}

int uc15_crt_attach(FILE *fd, uint8_t *rawcart)
{
    DBG(("UC15 attach CRT file"));
    ucmode = 1;
    return crt_attach(fd, rawcart);
}

void uc2_detach(void)
{
    lib_free(cart_ram);
    cart_ram = NULL;


    if(ucmode == 2) {
        export_remove(&export_res_uc2);
    }
    else {
        export_remove(&export_res_uc15);
    }
    io_source_unregister(uc_list_item);
    uc_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   2
#define SNAP_MODULE_NAME  "CARTUC2"

int uc2_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)regA) < 0)
        || (SMW_B(m, (uint8_t)regB) < 0)
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

int uc2_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B(m, &regA) < 0)
        || (SMR_B(m, &regB) < 0)
        || (SMR_B(m, &bankmask) < 0)
        || (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0)
        || (SMR_BA(m, romh_banks, 0x2000 * MAXBANKS) < 0)
        || (SMR_BA(m, cart_ram, CART_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (uc2_common_attach() == -1) {
        return -1;
    }
    return 0;
}
