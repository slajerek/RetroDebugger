/*
 * c64memsc.c -- C64 memory handling for x64sc.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "SYS_Types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alarm.h"
#include "c64.h"
#include "c64-resources.h"
#include "c64_256k.h"
#include "c64cart.h"
#include "c64cia.h"
#include "c64mem.h"
#include "c64meminit.h"
#include "c64memlimit.h"
#include "c64memrom.h"
#include "c64pla.h"
#include "c64model.h"
#include "c64ui.h"
#include "lib.h"
#include "c64cartmem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "ViceWrapper.h"
#include "cpmcart.h"
#include "machine.h"
#include "mainc64cpu.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "plus256k.h"
#include "plus60k.h"
#include "ram.h"
#include "resources.h"
#include "reu.h"
#include "sid.h"
#include "tpi.h"
#include "vicii-cycle.h"
#include "vicii-mem.h"
#include "vicii-phi1.h"
#include "vicii.h"

#include "log.h"
#include "c64-generic.h"

/* Machine class (moved from c64.c to distinguish between x64 and x64sc) */
int machine_class = VICE_MACHINE_C64SC;

/* import from c64-resources.c - don't use the resource for performance reasons */
extern int board_type;

/* C64 memory-related resources.  */

/* ------------------------------------------------------------------------- */

/* Number of possible memory configurations.  */
#define NUM_CONFIGS     32

/* Number of possible video banks (16K each).  */
#define NUM_VBANKS      4

/* The C64 memory.  */
//uint8_t mem_ram[C64_RAM_SIZE];
uint8_t *mem_ram;

#ifdef USE_EMBEDDED
#include "c64chargen.h"
#else
uint8_t mem_chargen_rom[C64_CHARGEN_ROM_SIZE];
#endif

/* Internal color memory.  */
static uint8_t mem_color_ram[0x400];
uint8_t *mem_color_ram_cpu, *mem_color_ram_vicii;

/* Pointer to the chargen ROM.  */
uint8_t *mem_chargen_rom_ptr;

/* Pointers to the currently used memory read and write tables.  */
read_func_ptr_t *_mem_read_tab_ptr;
store_func_ptr_t *_mem_write_tab_ptr;
/* VICE 3.10: dummy access table pointers for watchpoint on dummy accesses */
read_func_ptr_t *_mem_read_tab_ptr_dummy;
store_func_ptr_t *_mem_write_tab_ptr_dummy;
static uint8_t **_mem_read_base_tab_ptr;
static uint32_t *mem_read_limit_tab_ptr;

/* Memory read and write tables.  */
static store_func_ptr_t mem_write_tab[NUM_CONFIGS][0x101];
static read_func_ptr_t mem_read_tab[NUM_CONFIGS][0x101];
static uint8_t *mem_read_base_tab[NUM_CONFIGS][0x101];
static uint32_t mem_read_limit_tab[NUM_CONFIGS][0x101];

static store_func_ptr_t mem_write_tab_watch[0x101];
static read_func_ptr_t mem_read_tab_watch[0x101];

/* Current video bank (0, 1, 2 or 3).  */
static int vbank;

/* Current memory configuration.  */
static int mem_config;

/* Tape sense status: 1 = some button pressed, 0 = no buttons pressed.  */
static int tape_sense = 0;

static int tape_write_in = 0;
static int tape_motor_in = 0;

/* Current watchpoint state. 1 = watchpoints active, 0 = no watchpoints */
static int watchpoints_active;

/* ------------------------------------------------------------------------- */

static uint8_t zero_read_watch(uint16_t addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][0](addr);
}

static void zero_store_watch(uint16_t addr, uint8_t value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[mem_config][0](addr, value);
}

static uint8_t read_watch(uint16_t addr)
{
    monitor_watch_push_load_addr(addr, e_comp_space);
    return mem_read_tab[mem_config][addr >> 8](addr);
}

static void store_watch(uint16_t addr, uint8_t value)
{
    monitor_watch_push_store_addr(addr, e_comp_space);
    mem_write_tab[mem_config][addr >> 8](addr, value);
}

/* called by mem_pla_config_changed(), mem_toggle_watchpoints() */
static void mem_update_tab_ptrs(int flag)
{
    if (flag) {
        _mem_read_tab_ptr = mem_read_tab_watch;
        _mem_write_tab_ptr = mem_write_tab_watch;
        if (flag > 1) {
            /* enable watchpoints on dummy accesses */
            _mem_read_tab_ptr_dummy = mem_read_tab_watch;
            _mem_write_tab_ptr_dummy = mem_write_tab_watch;
        } else {
            _mem_read_tab_ptr_dummy = mem_read_tab[mem_config];
            _mem_write_tab_ptr_dummy = mem_write_tab[mem_config];
        }
    } else {
        /* all watchpoints disabled */
        _mem_read_tab_ptr = mem_read_tab[mem_config];
        _mem_write_tab_ptr = mem_write_tab[mem_config];
        _mem_read_tab_ptr_dummy = mem_read_tab[mem_config];
        _mem_write_tab_ptr_dummy = mem_write_tab[mem_config];
    }
}

void mem_toggle_watchpoints(int flag, void *context)
{
    mem_update_tab_ptrs(flag);
    watchpoints_active = flag;
}

/* ------------------------------------------------------------------------- */

/* $00/$01 unused bits emulation

   - There are 2 different unused bits, 1) the output bits, 2) the input bits
   - The output bits can be (re)set when the data-direction is set to output
     for those bits and the output bits will not drop-off to 0.
   - When the data-direction for the unused bits is set to output then the
     unused input bits can be (re)set by writing to them, when set to 1 the
     drop-off timer will start which will cause the unused input bits to drop
     down to 0 in a certain amount of time.
   - When an unused input bit already had the drop-off timer running, and is
     set to 1 again, the drop-off timer will restart.
   - when a an unused bit changes from output to input, and the current output
     bit is 1, the drop-off timer will restart again

    see testprogs/CPU/cpuport for details and tests
*/

/* VICE 3.10: clk_overflow_callback removed — 64-bit CLOCK eliminates overflow */

void c64_mem_init(void)
{
    /* Initialize REU BA low interface (FIXME find a better place for this) */
    reu_ba_register(vicii_cycle_reu, vicii_steal_cycles, &maincpu_ba_low_flags, MAINCPU_BA_LOW_REU);

    /* Initialize CP/M cart BA low interface (FIXME find a better place for this) */
    cpmcart_ba_register(vicii_cycle, vicii_steal_cycles, &maincpu_ba_low_flags, MAINCPU_BA_LOW_VICII);
}

int mem_get_current_bank_config(void) {
    return mem_config;
}

/* RD-027: the banking state the C64 memory mapping provider resolves from,
   as VICE holds it -- never re-derived from the last byte written to $01.
   memConfig is VICE's PLA configuration 0..31 (effective 6510 port bits 0-2,
   export.exrom << 3, export.game << 4), the index of mem_read_tab; cartType
   is mem_cartridge_type (CARTRIDGE_NONE = -1); romlBank/romhBank/exportRam
   are the generic "Main Slot" bank registers (8 KB banks of roml_banks,
   romh_banks, export_ram0), which cartridge types without their own banking
   leave at 0. Any pointer may be NULL. */
extern int mem_cartridge_type;
extern int roml_bank, romh_bank, export_ram;

void c64d_get_banking_state(int *memConfig, int *cartType, int *exrom, int *game,
                            int *romlBank, int *romhBank, int *exportRam)
{
    if (memConfig) { *memConfig = mem_config; }
    if (cartType)  { *cartType = mem_cartridge_type; }
    if (exrom)     { *exrom = export.exrom; }
    if (game)      { *game = export.game; }
    if (romlBank)  { *romlBank = roml_bank; }
    if (romhBank)  { *romhBank = romh_bank; }
    if (exportRam) { *exportRam = export_ram; }
}

/* RD-028: which storage an access at ADDR reaches under VICE's CURRENT memory
   configuration, read from VICE's own per-configuration tables -- the routing
   the CPU itself uses -- so cartridge modes, Ultimax and board quirks are
   VICE's answer, not a re-derived PLA table. KIND is a C64D_ACCESS_* code
   (ViceWrapper.h), OFFSET the byte offset inside that storage. A handler this
   function does not know (memory expansions, cartridge types with their own
   banking) is C64D_ACCESS_UNKNOWN, never a guess.
   Must run on the emulation thread or with the emulation paused. */
static uint8_t void_read(uint16_t addr);
static void void_store(uint16_t addr, uint8_t value);
extern int cart_getid_slot0(void);
extern int cart_getid_slot1(void);

static int c64d_generic_cartridge_only(void)
{
    return (mem_cartridge_type == CARTRIDGE_GENERIC_8KB || mem_cartridge_type == CARTRIDGE_GENERIC_16KB
            || mem_cartridge_type == CARTRIDGE_ULTIMAX)
           && cart_getid_slot0() == CARTRIDGE_NONE && cart_getid_slot1() == CARTRIDGE_NONE;
}

/* RD-030: main-slot banked cartridges whose ROML reads come from roml_banks
   at roml_bank: Magic Desk and Ocean through c64cartmem.c's default
   generic_roml_read, EasyFlash through its flash040 core, whose data is
   roml_banks (easyflash_config_setup) */
static int c64d_banked_roml_cartridge(void)
{
    return (mem_cartridge_type == CARTRIDGE_MAGIC_DESK || mem_cartridge_type == CARTRIDGE_OCEAN
            || mem_cartridge_type == CARTRIDGE_EASYFLASH)
           && cart_getid_slot0() == CARTRIDGE_NONE && cart_getid_slot1() == CARTRIDGE_NONE;
}

static int c64d_main_slot_only(int cartType)
{
    return mem_cartridge_type == cartType
           && cart_getid_slot0() == CARTRIDGE_NONE && cart_getid_slot1() == CARTRIDGE_NONE;
}

/* RD-030: the .crt file offset of a cartridge ROM byte, OFFSET being the
   c64d_resolve_access offset (into roml_banks for C64D_ACCESS_ROML, romh_banks
   for C64D_ACCESS_ROMH). The raw image crt.c recorded is roml_banks as loaded
   for generic, Magic Desk and Ocean carts (romh_banks at $2000 for a generic
   16 KB one); EasyFlash interleaves it, bank N's ROML at N * $4000 and its
   ROMH $2000 above (easyflash_crt_attach). 0 when not known */
extern int c64d_crt_file_offset(uint32_t rawOffset, uint32_t *fileOffset);
int c64d_cartridge_roml_file_offset(uint32_t offset, uint32_t *fileOffset)
{
    if (!(c64d_generic_cartridge_only() || c64d_banked_roml_cartridge()) || export_ram) {
        return 0;
    }
    if (mem_cartridge_type == CARTRIDGE_EASYFLASH) {
        offset = ((offset >> 13) << 14) | (offset & 0x1fff);
    }
    return c64d_crt_file_offset(offset, fileOffset);
}

int c64d_cartridge_romh_file_offset(uint32_t offset, uint32_t *fileOffset)
{
    if (c64d_main_slot_only(CARTRIDGE_EASYFLASH)) {
        return c64d_crt_file_offset(((offset >> 13) << 14) | 0x2000 | (offset & 0x1fff), fileOffset);
    }
    if (c64d_main_slot_only(CARTRIDGE_GENERIC_16KB)) {
        return c64d_crt_file_offset(0x2000 + offset, fileOffset);
    }
    return 0;
}

void c64d_resolve_access(uint16_t addr, int isWrite, int *kind, uint32_t *offset)
{
    int page = addr >> 8;
    int k = C64D_ACCESS_UNKNOWN;
    uint32_t off = addr;

    if (addr <= 0x0001) {
        k = C64D_ACCESS_CPU_PORT;
    } else if (page == 0) {
        k = C64D_ACCESS_RAM;    /* zero_read / zero_store: RAM beyond the port */
    } else if (!isWrite) {
        read_func_ptr_t f = mem_read_tab[mem_config][page];
        if (f == ram_read) {
            k = C64D_ACCESS_RAM;
        } else if (f == c64memrom_basic64_read) {
            k = C64D_ACCESS_BASIC;
            off = addr - 0xa000;
        } else if (f == c64memrom_kernal64_read) {
            k = C64D_ACCESS_KERNAL;
            off = addr - 0xe000;
        } else if (f == chargen_read) {
            k = C64D_ACCESS_CHARGEN;
            off = addr & 0x0fff;
        } else if (f == colorram_read) {
            k = C64D_ACCESS_COLORRAM;
            off = addr & 0x03ff;
        } else if (f == c64io_d000_read || f == c64io_d100_read || f == c64io_d200_read || f == c64io_d300_read
                   || f == c64io_d400_read || f == c64io_d500_read || f == c64io_d600_read || f == c64io_d700_read
                   || f == cia1_read || f == cia2_read || f == c64io_de00_read || f == c64io_df00_read) {
            k = C64D_ACCESS_IO;
            off = addr - 0xd000;
        } else if (f == void_read) {
            k = C64D_ACCESS_UNMAPPED;
        } else if (f == roml_read && (c64d_generic_cartridge_only() || c64d_banked_roml_cartridge())) {
            k = export_ram ? C64D_ACCESS_CART_RAM : C64D_ACCESS_ROML;
            off = export_ram ? (uint32_t)(addr & 0x1fff) : (uint32_t)((roml_bank << 13) + (addr & 0x1fff));
        } else if ((f == romh_read || f == ultimax_romh_read_hirom)
                   && (c64d_generic_cartridge_only() || c64d_main_slot_only(CARTRIDGE_EASYFLASH))) {
            k = C64D_ACCESS_ROMH;
            off = (uint32_t)((romh_bank << 13) + (addr & 0x1fff));
        } else if ((f == ultimax_1000_7fff_read || f == ultimax_a000_bfff_read || f == ultimax_c000_cfff_read)
                   && (c64d_generic_cartridge_only() || c64d_banked_roml_cartridge())) {
            k = C64D_ACCESS_UNMAPPED;    /* Ultimax: for these carts the handler's default, open bus */
        } else if (f == ultimax_d000_dfff_read && (c64d_generic_cartridge_only() || c64d_banked_roml_cartridge())) {
            /* Ultimax: read_bank_io, the C64's I/O and color RAM whatever $01 says */
            if (page >= 0xd8 && page <= 0xdb) {
                k = C64D_ACCESS_COLORRAM;
                off = addr & 0x03ff;
            } else {
                k = C64D_ACCESS_IO;
                off = addr - 0xd000;
            }
        } else if (f == romh_read && c64d_main_slot_only(CARTRIDGE_OCEAN)) {
            k = C64D_ACCESS_ROML;    /* ocean_romh_read: $A000 reads the ROML bank too */
            off = (uint32_t)((roml_bank << 13) + (addr & 0x1fff));
        }
    } else {
        store_func_ptr_t s = mem_write_tab[mem_config][page];
        if (s == ram_store || s == ram_hi_store) {
            k = C64D_ACCESS_RAM;
        } else if ((s == raml_no_ultimax_store || s == ramh_no_ultimax_store
                    || s == roml_no_ultimax_store || s == romh_no_ultimax_store)
                   && (mem_cartridge_type == CARTRIDGE_NONE || c64d_generic_cartridge_only())) {
            k = C64D_ACCESS_RAM;    /* ROM overlays write through to the RAM underneath */
        } else if (s == colorram_store) {
            k = C64D_ACCESS_COLORRAM;
            off = addr & 0x03ff;
        } else if (s == c64io_d000_store || s == c64io_d100_store || s == c64io_d200_store || s == c64io_d300_store
                   || s == c64io_d400_store || s == c64io_d500_store || s == c64io_d600_store || s == c64io_d700_store
                   || s == cia1_store || s == cia2_store || s == c64io_de00_store || s == c64io_df00_store) {
            k = C64D_ACCESS_IO;
            off = addr - 0xd000;
        } else if (s == void_store) {
            k = C64D_ACCESS_UNMAPPED;
        }
    }

    if (kind) { *kind = k; }
    if (offset) { *offset = off; }
}

void mem_pla_config_changed(void)
{
    mem_config = (((~pport.dir | pport.data) & 0x7) | (export.exrom << 3) | (export.game << 4));

    /* NOTE: CPU port bits 3,4,5 are not connected on the SX64 board */
    if (board_type == BOARD_SX64) {
        c64pla_config_changed(1, 1, 1, 1, 0x07);
    } else {
        c64pla_config_changed(tape_sense, tape_write_in, tape_motor_in, 1, 0x17);
    }

    mem_update_tab_ptrs(watchpoints_active);

    _mem_read_base_tab_ptr = mem_read_base_tab[mem_config];
    mem_read_limit_tab_ptr = mem_read_limit_tab[mem_config];

    maincpu_resync_limits();
}

/* reads zeropage, 0/1 comes from RAM */
uint8_t zero_read_dma(uint16_t addr)
{
    addr &= 0xff;

    if (c64_256k_enabled) {
        return c64_256k_ram_segment0_read(addr);
    } else {
        if (plus256k_enabled) {
            return plus256k_ram_low_read(addr);
        } else {
            return mem_ram[addr & 0xff];
        }
    }
}

/* reads zeropage, 0/1 comes from CPU port */
uint8_t zero_read(uint16_t addr)
{
    uint8_t retval;

    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            return pport.dir_read;
        case 1:
            retval = pport.data_read;

            /* discharge the "capacitor" */

            /* FIXME: bits 3,4,5 are not connected on SX-64 boards - whether they
                      show similar behaviour needs to be tested */
            if (board_type == BOARD_SX64) {
                /* set real value of read bit 3 */
                if (pport.data_falloff_bit3 && (pport.data_set_clk_bit3 < maincpu_clk)) {
                    pport.data_falloff_bit3 = 0;
                    pport.data_set_bit3 = 0;
                }
                /* set real value of read bit 4 */
                if (pport.data_falloff_bit4 && (pport.data_set_clk_bit4 < maincpu_clk)) {
                    pport.data_falloff_bit4 = 0;
                    pport.data_set_bit4 = 0;
                }
                /* set real value of read bit 5 */
                if (pport.data_falloff_bit5 && (pport.data_set_clk_bit5 < maincpu_clk)) {
                    pport.data_falloff_bit5 = 0;
                    pport.data_set_bit5 = 0;
                }
            }

            /* set real value of read bit 6 */
            if (pport.data_falloff_bit6 && (pport.data_set_clk_bit6 < maincpu_clk)) {
                pport.data_falloff_bit6 = 0;
                pport.data_set_bit6 = 0;
            }

            /* set real value of read bit 7 */
            if (pport.data_falloff_bit7 && (pport.data_set_clk_bit7 < maincpu_clk)) {
                pport.data_falloff_bit7 = 0;
                pport.data_set_bit7 = 0;
            }

            /* for unused bits in input mode, the value comes from the "capacitor" */
            if (board_type == BOARD_SX64) {
                /* set real value of bit 3 */
                if (!(pport.dir_read & 0x08)) {
                    retval &= ~0x08;
                    retval |= pport.data_set_bit3;
                }
                /* set real value of bit 4 */
                if (!(pport.dir_read & 0x10)) {
                    retval &= ~0x10;
                    retval |= pport.data_set_bit4;
                }
                /* set real value of bit 5 */
                if (!(pport.dir_read & 0x20)) {
                    retval &= ~0x20;
                    retval |= pport.data_set_bit5;
                }
            }

            /* set real value of bit 6 */
            if (!(pport.dir_read & 0x40)) {
                retval &= ~0x40;
                retval |= pport.data_set_bit6;
            }

            /* set real value of bit 7 */
            if (!(pport.dir_read & 0x80)) {
                retval &= ~0x80;
                retval |= pport.data_set_bit7;
            }
            return retval;
    }

    return zero_read_dma(addr);
}

/* store zeropage, 0/1 goes to RAM */
void zero_store_dma(uint16_t addr, uint8_t value)
{
    addr &= 0xff;

    if (vbank == 0) {
        if (c64_256k_enabled) {
            c64_256k_ram_segment0_store(addr, value);
        } else {
            if (plus256k_enabled) {
                plus256k_ram_low_store(addr, value);
            } else {
                mem_ram[addr] = value;
            }
        }
    } else {
        mem_ram[addr] = value;
    }
}

#define FALLOFF_RANDOM (C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES / 5)
#define FALLOFF_RANDOM_SX (SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES / 5)

/* store zeropage, 0/1 goes to CPU port */
void zero_store(uint16_t addr, uint8_t value)
{
    addr &= 0xff;

    switch ((uint8_t)addr) {
        case 0:
            if (vbank == 0) {
                if (c64_256k_enabled) {
                    c64_256k_ram_segment0_store((uint16_t)0, vicii_read_phi1());
                } else {
                    if (plus256k_enabled) {
                        plus256k_ram_low_store((uint16_t)0, vicii_read_phi1());
                    } else {
                        mem_ram[0] = vicii_read_phi1();
                    }
                }
            } else {
                mem_ram[0] = vicii_read_phi1();
                machine_handle_pending_alarms(1);
            }
            /* when switching an unused bit from output (where it contained a
               stable value) to input mode (where the input is floating), some
               of the charge is transferred to the floating input */

            if (board_type == BOARD_SX64) {
                if ((pport.dir & 0x08)) {
                    if ((pport.dir ^ value) & 0x08) {
                        pport.data_set_clk_bit3 = maincpu_clk + SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM_SX);
                        pport.data_set_bit3 = pport.data & 0x08;
                        pport.data_falloff_bit3 = 1;
                    }
                }
                if ((pport.dir & 0x10)) {
                    if ((pport.dir ^ value) & 0x10) {
                        pport.data_set_clk_bit4 = maincpu_clk + SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM_SX);
                        pport.data_set_bit4 = pport.data & 0x10;
                        pport.data_falloff_bit4 = 1;
                    }
                }
                if ((pport.dir & 0x20)) {
                    if ((pport.dir ^ value) & 0x20) {
                        pport.data_set_clk_bit5 = maincpu_clk + SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM_SX);
                        pport.data_set_bit5 = pport.data & 0x20;
                        pport.data_falloff_bit5 = 1;
                    }
                }
            }

            /* check if bit 6 has flipped */
            if ((pport.dir & 0x40)) {
                if ((pport.dir ^ value) & 0x40) {
                    pport.data_set_clk_bit6 = maincpu_clk + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM);
                    pport.data_set_bit6 = pport.data & 0x40;
                    pport.data_falloff_bit6 = 1;
                }
            }

            /* check if bit 7 has flipped */
            if ((pport.dir & 0x80)) {
                if ((pport.dir ^ value) & 0x80) {
                    pport.data_set_clk_bit7 = maincpu_clk + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM);
                    pport.data_set_bit7 = pport.data & 0x80;
                    pport.data_falloff_bit7 = 1;
                }
            }

            if (pport.dir != value) {
                pport.dir = value;
                mem_pla_config_changed();
            }
            break;
        case 1:
            if (vbank == 0) {
                if (c64_256k_enabled) {
                    c64_256k_ram_segment0_store((uint16_t)1, vicii_read_phi1());
                } else {
                    if (plus256k_enabled) {
                        plus256k_ram_low_store((uint16_t)1, vicii_read_phi1());
                    } else {
                        mem_ram[1] = vicii_read_phi1();
                    }
                }
            } else {
                mem_ram[1] = vicii_read_phi1();
                machine_handle_pending_alarms(1);
            }

            /* when writing to an unused bit that is output, charge the "capacitor",
               otherwise don't touch it */
            if (pport.dir & 0x80) {
                pport.data_set_bit7 = value & 0x80;
                pport.data_set_clk_bit7 = maincpu_clk + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM);
                pport.data_falloff_bit7 = 1;
            }

            if (pport.dir & 0x40) {
                pport.data_set_bit6 = value & 0x40;
                pport.data_set_clk_bit6 = maincpu_clk + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM);
                pport.data_falloff_bit6 = 1;
            }

            if (board_type == BOARD_SX64) {
                if (pport.dir & 0x20) {
                    pport.data_set_bit5 = value & 0x20;
                    pport.data_set_clk_bit5 = maincpu_clk + SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM_SX);
                    pport.data_falloff_bit5 = 1;
                }
                if (pport.dir & 0x10) {
                    pport.data_set_bit4 = value & 0x10;
                    pport.data_set_clk_bit4 = maincpu_clk + SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM_SX);
                    pport.data_falloff_bit4 = 1;
                }
                if (pport.dir & 0x08) {
                    pport.data_set_bit3 = value & 0x08;
                    pport.data_set_clk_bit3 = maincpu_clk + SX64_CPU6510_DATA_PORT_FALL_OFF_CYCLES + lib_unsigned_rand(0, FALLOFF_RANDOM_SX);
                    pport.data_falloff_bit3 = 1;
                }
            }

            if (pport.data != value) {
                pport.data = value;
                mem_pla_config_changed();
            }
            break;
        default:
            zero_store_dma(addr, value);
            break;
    }
}

/* ------------------------------------------------------------------------- */

uint8_t chargen_read(uint16_t addr)
{
    return mem_chargen_rom[addr & 0xfff];
}

void chargen_store(uint16_t addr, uint8_t value)
{
    mem_chargen_rom[addr & 0xfff] = value;
}

uint8_t ram_read(uint16_t addr)
{
    return mem_ram[addr];
}

void ram_store(uint16_t addr, uint8_t value)
{
    mem_ram[addr] = value;
}

void ram_hi_store(uint16_t addr, uint8_t value)
{
    mem_ram[addr] = value;
}

/* unconnected memory space */
static uint8_t void_read(uint16_t addr)
{
    return vicii_read_phi1();
}

static void void_store(uint16_t addr, uint8_t value)
{
    return;
}

/* ------------------------------------------------------------------------- */

/* DMA memory access, this is the same as generic memory access, but needs to
   bypass the CPU port, so it accesses RAM at $00/$01 (from VICE 3.10) */

void mem_dma_store(uint16_t addr, uint8_t value)
{
    if ((addr & 0xff00) == 0) {
        /* exception: 0/1 accesses RAM! */
        zero_store_dma(addr, value);
    } else {
        _mem_write_tab_ptr[addr >> 8](addr, value);
    }
}

uint8_t mem_dma_read(uint16_t addr)
{
    if ((addr & 0xff00) == 0) {
        /* exception: 0/1 accesses RAM! */
        return zero_read_dma(addr);
    }
    return _mem_read_tab_ptr[addr >> 8](addr);
}

/* ------------------------------------------------------------------------- */

/* Generic memory access.  */

void mem_store(uint16_t addr, uint8_t value)
{
    _mem_write_tab_ptr[addr >> 8](addr, value);
}

uint8_t mem_read(uint16_t addr)
{
    return _mem_read_tab_ptr[addr >> 8](addr);
}

void mem_store_without_ultimax(uint16_t addr, uint8_t value)
{
    mem_write_tab[mem_config & 7][addr >> 8](addr, value);
}

uint8_t mem_read_without_ultimax(uint16_t addr)
{
    return mem_read_tab[mem_config & 7][addr >> 8](addr);
}

void mem_store_without_romlh(uint16_t addr, uint8_t value)
{
    mem_write_tab[0][addr >> 8](addr, value);
}


/* ------------------------------------------------------------------------- */

void colorram_store(uint16_t addr, uint8_t value)
{
    mem_color_ram[addr & 0x3ff] = value & 0xf;
}

uint8_t colorram_read(uint16_t addr)
{
    return mem_color_ram[addr & 0x3ff] | (vicii_read_phi1() & 0xf0);
}

uint8_t c64d_colorram_peek(uint16_t addr)
{
	return mem_color_ram[addr & 0x3ff] | (vicii_read_phi1() & 0xf0);
}

/* ------------------------------------------------------------------------- */

/* init 256k memory table changes */

static int check_256k_ram_write(int i, int j)
{
    if (mem_write_tab[i][j] == ram_hi_store || mem_write_tab[i][j] == ram_store) {
        return 1;
    } else {
        return 0;
    }
}

static void c64_256k_init_config(void)
{
    int i, j;

    if (c64_256k_enabled) {
        mem_limit_256k_init();
        for (i = 0; i < NUM_CONFIGS; i++) {
            for (j = 1; j <= 0xff; j++) {
                if (check_256k_ram_write(i, j) == 1) {
                    if (j < 0x40) {
                        mem_write_tab[i][j] = c64_256k_ram_segment0_store;
                    }
                    if (j > 0x3f && j < 0x80) {
                        mem_write_tab[i][j] = c64_256k_ram_segment1_store;
                    }
                    if (j > 0x7f && j < 0xc0) {
                        mem_write_tab[i][j] = c64_256k_ram_segment2_store;
                    }
                    if (j > 0xbf) {
                        mem_write_tab[i][j] = c64_256k_ram_segment3_store;
                    }
                }
                if (mem_read_tab[i][j] == ram_read) {
                    if (j < 0x40) {
                        mem_read_tab[i][j] = c64_256k_ram_segment0_read;
                    }
                    if (j > 0x3f && j < 0x80) {
                        mem_read_tab[i][j] = c64_256k_ram_segment1_read;
                    }
                    if (j > 0x7f && j < 0xc0) {
                        mem_read_tab[i][j] = c64_256k_ram_segment2_read;
                    }
                    if (j > 0xbf) {
                        mem_read_tab[i][j] = c64_256k_ram_segment3_read;
                    }
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

/* init plus256k memory table changes */

static void plus256k_init_config(void)
{
    int i, j;

    if (plus256k_enabled) {
        mem_limit_256k_init();
        for (i = 0; i < NUM_CONFIGS; i++) {
            for (j = 1; j <= 0xff; j++) {
                if (check_256k_ram_write(i, j) == 1) {
                    if (j < 0x10) {
                        mem_write_tab[i][j] = plus256k_ram_low_store;
                    } else {
                        mem_write_tab[i][j] = plus256k_ram_high_store;
                    }
                }

                if (mem_read_tab[i][j] == ram_read) {
                    if (j < 0x10) {
                        mem_read_tab[i][j] = plus256k_ram_low_read;
                    } else {
                        mem_read_tab[i][j] = plus256k_ram_high_read;
                    }
                }
            }
        }
    }
}

/* init plus60k memory table changes */

static void plus60k_init_config(void)
{
    int i, j;

    if (plus60k_enabled) {
        mem_limit_plus60k_init();
        for (i = 0; i < NUM_CONFIGS; i++) {
            for (j = 0x10; j <= 0xff; j++) {
                if (mem_write_tab[i][j] == ram_hi_store) {
                    mem_write_tab[i][j] = plus60k_ram_hi_store;
                }
                if (mem_write_tab[i][j] == ram_store) {
                    mem_write_tab[i][j] = plus60k_ram_store;
                }

                if (mem_read_tab[i][j] == ram_read) {
                    mem_read_tab[i][j] = plus60k_ram_read;
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

void mem_set_write_hook(int config, int page, store_func_t *f)
{
    mem_write_tab[config][page] = f;
}

void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func)
{
    mem_read_tab[base][index] = read_func;
}

void mem_read_base_set(unsigned int base, unsigned int index, uint8_t *mem_ptr)
{
    mem_read_base_tab[base][index] = mem_ptr;
}

/* add actual pointer (from VICE 3.10) */
void mem_read_addr_set(unsigned int base, unsigned int index, uintptr_t addr)
{
    mem_read_base_tab[base][index] += addr;
}

void mem_read_limit_set(unsigned int base, unsigned int index, uint32_t limit)
{
    mem_read_limit_tab[base][index] = limit;
}

void c64d_init_memory(uint8 *c64memory)
{
	// C64Debugger: set mem_ram by wrapper so we can have memory mapped file
	mem_ram = c64memory;
}

void mem_initialize_memory(void)
{
    int i, j;
    int board;

    mem_chargen_rom_ptr = mem_chargen_rom;
    mem_color_ram_cpu = mem_color_ram;
    mem_color_ram_vicii = mem_color_ram;

    mem_limit_init();

    /* setup watchpoint tables */
    mem_read_tab_watch[0] = zero_read_watch;
    mem_write_tab_watch[0] = zero_store_watch;
    for (i = 1; i <= 0x100; i++) {
        mem_read_tab_watch[i] = read_watch;
        mem_write_tab_watch[i] = store_watch;
    }

    resources_get_int("BoardType", &board);

    /* Default is RAM.  */
    for (i = 0; i < NUM_CONFIGS; i++) {
        mem_set_write_hook(i, 0, zero_store);
        mem_read_tab[i][0] = zero_read;
        mem_read_base_tab[i][0] = mem_ram;
        for (j = 1; j <= 0xfe; j++) {
            if (board == BOARD_MAX && j >= 0x08) {
                mem_read_tab[i][j] = void_read;
                mem_read_base_tab[i][j] = NULL;
                mem_set_write_hook(0, j, void_store);
                continue;
            }
            mem_read_tab[i][j] = ram_read;
            mem_read_base_tab[i][j] = mem_ram;
            mem_write_tab[i][j] = ram_store;
        }
        if (board == BOARD_MAX) {
            mem_read_tab[i][0xff] = void_read;
            mem_read_base_tab[i][0xff] = NULL;
            mem_set_write_hook(0, 0xff, void_store);
        } else {
            mem_read_tab[i][0xff] = ram_read;
            mem_read_base_tab[i][0xff] = mem_ram;

            /* REU $ff00 trigger is handled within `ram_hi_store()'.  */
            mem_set_write_hook(i, 0xff, ram_hi_store);
        }
    }

    /* Setup character generator ROM at $D000-$DFFF (memory configs 1, 2, 3, 9, 10, 11, 26, 27).  */
    for (i = 0xd0; i <= 0xdf; i++) {
        mem_read_tab[1][i] = chargen_read;
        mem_read_tab[2][i] = chargen_read;
        mem_read_tab[3][i] = chargen_read;
        mem_read_tab[9][i] = chargen_read;
        mem_read_tab[10][i] = chargen_read;
        mem_read_tab[11][i] = chargen_read;
        mem_read_tab[26][i] = chargen_read;
        mem_read_tab[27][i] = chargen_read;
        mem_read_base_tab[1][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[2][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[3][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[9][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[10][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[11][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[26][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
        mem_read_base_tab[27][i] = (uint8_t *)(mem_chargen_rom - (uint8_t *)0xd000);
    }

    c64meminit(0);

    for (i = 0; i < NUM_CONFIGS; i++) {
        mem_read_tab[i][0x100] = mem_read_tab[i][0];
        mem_write_tab[i][0x100] = mem_write_tab[i][0];
        mem_read_base_tab[i][0x100] = mem_read_base_tab[i][0];
    }

    _mem_read_tab_ptr = mem_read_tab[7];
    _mem_write_tab_ptr = mem_write_tab[7];
    _mem_read_base_tab_ptr = mem_read_base_tab[7];
    mem_read_limit_tab_ptr = mem_read_limit_tab[7];

    vicii_set_chargen_addr_options(0x7000, 0x1000);

    c64pla_pport_reset();
    export.exrom = 0;
    export.game = 0;

    /* Setup initial memory configuration.  */
    mem_pla_config_changed();
    cartridge_init_config();
    /* internal expansions, these may modify the above mappings and must take
       care of hooking up all callbacks correctly.
    */
    plus60k_init_config();
    plus256k_init_config();
    c64_256k_init_config();

    if (board == 1) {
        mem_limit_max_init();
    }
}

void mem_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    uint8_t *p = _mem_read_base_tab_ptr[addr >> 8];
    uint32_t limits;

    if (p != NULL && addr > 1) {
        *base = p;
        limits = mem_read_limit_tab_ptr[addr >> 8];
        *limit = limits & 0xffff;
        *start = limits >> 16;
    } else {
        cartridge_mmu_translate(addr, base, start, limit);
    }
}

/* ------------------------------------------------------------------------- */

/* Initialize RAM for power-up.  */
void mem_powerup(void)
{
    ram_init(mem_ram, 0x10000);
    /* cartridge_ram_init() removed in VICE 3.10 — cart RAM init handled by individual carts */
}

/* ------------------------------------------------------------------------- */

/* Change the current video bank.  Call this routine only when the vbank
   has really changed.  */
void mem_set_vbank(int new_vbank)
{
    vbank = new_vbank;
    vicii_set_vbank(new_vbank);
}

/* Set the tape sense status.  */
void mem_set_tape_sense(int sense)
{
    tape_sense = sense;
    mem_pla_config_changed();
}

/* Set the tape write in. */
void mem_set_tape_write_in(int val)
{
    tape_write_in = val;
    mem_pla_config_changed();
}

/* Set the tape motor in. */
void mem_set_tape_motor_in(int val)
{
    tape_motor_in = val;
    mem_pla_config_changed();
}

/* ------------------------------------------------------------------------- */

/* FIXME: this part needs to be checked.  */

void mem_get_basic_text(uint16_t *start, uint16_t *end)
{
    if (start != NULL) {
        *start = mem_ram[0x2b] | (mem_ram[0x2c] << 8);
    }
    if (end != NULL) {
        *end = mem_ram[0x2d] | (mem_ram[0x2e] << 8);
    }
}

void mem_set_basic_text(uint16_t start, uint16_t end)
{
    mem_ram[0x2b] = mem_ram[0xac] = start & 0xff;
    mem_ram[0x2c] = mem_ram[0xad] = start >> 8;
    mem_ram[0x2d] = mem_ram[0x2f] = mem_ram[0x31] = mem_ram[0xae] = end & 0xff;
    mem_ram[0x2e] = mem_ram[0x30] = mem_ram[0x32] = mem_ram[0xaf] = end >> 8;
}

void mem_inject(uint32_t addr, uint8_t value)
{
    /* could be made to handle various internal expansions in some sane way */
    mem_ram[addr & 0xffff] = value;
}

/* ------------------------------------------------------------------------- */

int mem_rom_trap_allowed(uint16_t addr)
{
    if (addr >= 0xe000) {
        switch (mem_config) {
            case 2:
            case 3:
            case 6:
            case 7:
            case 10:
            case 11:
            case 14:
            case 15:
            case 26:
            case 27:
            case 30:
            case 31:
                return 1;
            default:
                return 0;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Banked memory access functions for the monitor.  */

void store_bank_io(uint16_t addr, uint8_t byte)
{
    switch (addr & 0xff00) {
        case 0xd000:
            c64io_d000_store(addr, byte);
            break;
        case 0xd100:
            c64io_d100_store(addr, byte);
            break;
        case 0xd200:
            c64io_d200_store(addr, byte);
            break;
        case 0xd300:
            c64io_d300_store(addr, byte);
            break;
        case 0xd400:
            c64io_d400_store(addr, byte);
            break;
        case 0xd500:
            c64io_d500_store(addr, byte);
            break;
        case 0xd600:
            c64io_d600_store(addr, byte);
            break;
        case 0xd700:
            c64io_d700_store(addr, byte);
            break;
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            colorram_store(addr, byte);
            break;
        case 0xdc00:
            cia1_store(addr, byte);
            break;
        case 0xdd00:
            cia2_store(addr, byte);
            break;
        case 0xde00:
            c64io_de00_store(addr, byte);
            break;
        case 0xdf00:
            c64io_df00_store(addr, byte);
            break;
    }
    return;
}

uint8_t read_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
            return c64io_d000_read(addr);
        case 0xd100:
            return c64io_d100_read(addr);
        case 0xd200:
            return c64io_d200_read(addr);
        case 0xd300:
            return c64io_d300_read(addr);
        case 0xd400:
            return c64io_d400_read(addr);
        case 0xd500:
            return c64io_d500_read(addr);
        case 0xd600:
            return c64io_d600_read(addr);
        case 0xd700:
            return c64io_d700_read(addr);
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            return colorram_read(addr);
        case 0xdc00:
            return cia1_read(addr);
        case 0xdd00:
            return cia2_read(addr);
        case 0xde00:
            return c64io_de00_read(addr);
        case 0xdf00:
            return c64io_df00_read(addr);
    }
    return 0xff;
}

static uint8_t peek_bank_io(uint16_t addr)
{
    switch (addr & 0xff00) {
        case 0xd000:
            return c64io_d000_peek(addr);
        case 0xd100:
            return c64io_d100_peek(addr);
        case 0xd200:
            return c64io_d200_peek(addr);
        case 0xd300:
            return c64io_d300_peek(addr);
        case 0xd400:
            return c64io_d400_peek(addr);
        case 0xd500:
            return c64io_d500_peek(addr);
        case 0xd600:
            return c64io_d600_peek(addr);
        case 0xd700:
            return c64io_d700_peek(addr);
        case 0xd800:
        case 0xd900:
        case 0xda00:
        case 0xdb00:
            return colorram_read(addr);
        case 0xdc00:
            return cia1_peek(addr);
        case 0xdd00:
            return cia2_peek(addr);
        case 0xde00:
            return c64io_de00_peek(addr);
        case 0xdf00:
            return c64io_df00_peek(addr);
    }
    return 0xff;
}

/* ------------------------------------------------------------------------- */

/* Exported banked memory access functions for the monitor.  */

static const char *banknames[] = {
    "default",
    "cpu",
    "ram",
    "rom",
    "io",
    "cart",
    NULL
};

static const int banknums[] = { 1, 0, 1, 2, 3, 4 };

const char **mem_bank_list(void)
{
    return banknames;
}

int mem_bank_from_name(const char *name)
{
    int i = 0;

    while (banknames[i]) {
        if (!strcmp(name, banknames[i])) {
            return banknums[i];
        }
        i++;
    }
    return -1;
}

uint8_t mem_bank_read(int bank, uint16_t addr, void *context)
{
    switch (bank) {
        case 0:                   /* current */
            return mem_read(addr);
            break;
        case 3:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                return read_bank_io(addr);
            }
        case 4:                   /* cart */
            return cartridge_peek_mem(addr);
        case 2:                   /* rom */
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return c64memrom_basic64_rom[addr & 0x1fff];
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return mem_chargen_rom[addr & 0x0fff];
            }
            if (addr >= 0xe000) {
                return c64memrom_kernal64_rom[addr & 0x1fff];
            }
        case 1:                   /* ram */
            break;
    }
    return mem_ram[addr];
}

// Slaj: unfortunately mem_bank_peek DOES affect CIA I/O when run from async so can't be used here :(

uint8_t c64d_cia1_peek(uint16_t addr);
uint8_t c64d_cia2_peek(uint16_t addr);

uint8_t c64d_peek_io(uint16_t addr)
{
	switch ((addr >> 8) & 0x0F)
	{
		case 0x0:	// VIC
		case 0x1:
		case 0x2:
		case 0x3:
			return vicii_peek(addr);
		case 0x4:	// SID
		case 0x5:
		case 0x6:
		case 0x7:
			return sid_peek(addr);
		case 0x8:	// Color RAM
		case 0x9:
		case 0xa:
		case 0xb:
			return colorram_read(addr);
		case 0xc:	// CIA 1
			return c64d_cia1_peek(addr);
		case 0xd:	// CIA 2
			return c64d_cia2_peek(addr);
		case 0xe:	// TODO: REU/Open I/O
		case 0xf:
			return vicii_read_phi1();
	}
	abort();
	return 0;
}

/* from c64cart.c */
extern int mem_cartridge_type; /* Type of the cartridge attached. ("Main Slot") */
uint8_t roml_read_slotmain(uint16_t addr);
uint8_t romh_read_slotmain(uint16_t addr);
uint8_t cartridge_peek_mem_slotmain(uint16_t addr);

// https://www.c64-wiki.com/index.php/Bank_Switching

uint8_t c64d_peek_memory0001()
{
	return (pport.data_read & (0xff - (((!pport.data_set_bit6) << 6) + ((!pport.data_set_bit7) << 7))));
}

/* Cartridge windows of c64d_peek_c64: VICE's own side-effect-free monitor
   peek, which dispatches per slot and per cartridge type (EasyFlash, Action
   Replay, ... each have their own peek) and falls back to RAM where the
   cartridge does not answer, e.g. Ultimax $1000-$7FFF. This used to call
   generic_peek_mem for every cartridge type, and returned an uninitialized
   byte whenever that answered CART_READ_THROUGH. */
static uint8_t c64d_peek_cartridge(uint16_t addr)
{
	return cartridge_peek_mem(addr);
}

uint8_t c64d_peek_c64(uint16_t addr)
{
	int addrHi;
	uint8_t mem_config;
	int exrom;
	int game;
	uint8_t basic_in;
	uint8_t kernal_in;
	uint8_t char_in;
	uint8_t io_in;

	uint8_t charen;
	uint8_t hiram;
	uint8_t loram;
	
	uint8_t mem01;
	

	if (addr == 0x0000)
	{
		return pport.dir_read;
	}
	else if (addr == 0x0001)
	{
		return (pport.data_read & (0xff - (((!pport.data_set_bit6) << 6) + ((!pport.data_set_bit7) << 7))));
	}

	// TODO: change this into memory configs and func calls as it's done in VICE,
	//       note we need to peek into ROM directly to not trigger alarms

	mem_config = (((~pport.dir | pport.data) & 0x7) | (export.exrom << 3) | (export.game << 4));
//	LOGD("mem_config=%d", mem_config);
	//            return cartridge_peek_mem(addr);

	addrHi = addr >> 0x0c;
	
	// exrom, game high = 0
	//LOGD("exrom=%d game=%d", export.exrom, export.game);
	
	
	// exrom & game are set as here: https://www.c64-wiki.com/index.php/Bank_Switching
	exrom = !export.exrom;
	game = !export.game;
	
	mem01 = (~pport.dir | pport.data) & 0x07;
	
	charen = (mem01 & 0x04) == 0x04;
	hiram  = (mem01 & 0x02) == 0x02;
	loram  = (mem01 & 0x01) == 0x01;
	
	basic_in = (pport.data_read & 0x03) == 0x03;
	kernal_in = pport.data_read & 2;
	char_in = (pport.data_read & 3) && ~(pport.data_read & 4);
	io_in = (pport.data_read & 3) && (pport.data_read & 4);
	
	switch (addrHi)
	{
		case 0x00:
		{
			return mem_ram[addr];
		}
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else
			{
				return mem_ram[addr];
			}
		case 0x08:
		case 0x09:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else if (exrom == 0)
			{
				if (hiram == 1 && loram == 1)
				{
					return c64d_peek_cartridge(addr);
				}
				else
				{
					return mem_ram[addr];
				}
			}
			else
			{
				return mem_ram[addr];
			}
		case 0x0a:
		case 0x0b:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else if (game == 1)
			{
				if (basic_in)
				{
					return c64memrom_basic64_rom[addr & 0x1fff];
				}
				else
				{
					return mem_ram[addr];
				}
			}
			else
			{
				if (basic_in)
				{
					return c64d_peek_cartridge(addr);
				}
				else
				{
					return mem_ram[addr];
				}
			}
		case 0x0c:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else
			{
				return mem_ram[addr];
			}
		case 0x0d:
			{
				if (mem_cartridge_type != CARTRIDGE_NONE)
				{
					// TODO: check if peek does not create triggers/alarms...
					uint8 addrHi2 = (addr >> 8);
					if (addrHi2 == 0xDE)
					{
						return c64io_de00_peek(addr);
					}
					if (addrHi2 == 0xDF)
					{
						return c64io_df00_peek(addr);
					}
				}
				
				if (io_in)
				{
					return c64d_peek_io(addr);
				}
				else if (char_in)
				{
					return mem_chargen_rom[addr & 0x0fff];
				}
				else
				{
					return mem_ram[addr];
				}
			}
		case 0x0e:
		case 0x0f:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else if (kernal_in)
			{
				return c64memrom_kernal64_rom[addr & 0x1fff];
			}
			else
			{
				return mem_ram[addr];
			}
	}

	// shouldn't happen
	return mem_ram[addr];
}

uint8_t c64d_peek_c64_for_cycle(uint16_t addr, uint8_t memory0001, uint8_t exrom_state, uint8_t game_state)
{
	int addrHi;
	int exrom;
	int game;
	uint8_t basic_in;
	uint8_t kernal_in;
	uint8_t char_in;
	uint8_t io_in;
	uint8_t mem01;

	if (addr == 0x0000 || addr == 0x0001)
	{
		return mem_ram[addr];
	}

	addrHi = addr >> 0x0c;

	// exrom & game are inverted as in c64d_peek_c64
	exrom = !exrom_state;
	game = !game_state;

	mem01 = memory0001 & 0x07;

	basic_in = (memory0001 & 0x03) == 0x03;
	kernal_in = memory0001 & 2;
	char_in = (memory0001 & 3) && ~(memory0001 & 4);
	io_in = (memory0001 & 3) && (memory0001 & 4);

	switch (addrHi)
	{
		case 0x00:
			return mem_ram[addr];
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else
			{
				return mem_ram[addr];
			}
		case 0x08:
		case 0x09:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else if (exrom == 0)
			{
				if ((mem01 & 0x02) && (mem01 & 0x01))
				{
					return c64d_peek_cartridge(addr);
				}
				else
				{
					return mem_ram[addr];
				}
			}
			else
			{
				return mem_ram[addr];
			}
		case 0x0a:
		case 0x0b:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else if (game == 1)
			{
				if (basic_in)
				{
					return c64memrom_basic64_rom[addr & 0x1fff];
				}
				else
				{
					return mem_ram[addr];
				}
			}
			else
			{
				if (basic_in)
				{
					return c64d_peek_cartridge(addr);
				}
				else
				{
					return mem_ram[addr];
				}
			}
		case 0x0c:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else
			{
				return mem_ram[addr];
			}
		case 0x0d:
			{
				if (io_in)
				{
					return c64d_peek_io(addr);
				}
				else if (char_in)
				{
					return mem_chargen_rom[addr & 0x0fff];
				}
				else
				{
					return mem_ram[addr];
				}
			}
		case 0x0e:
		case 0x0f:
			if (exrom == 1 && game == 0)
			{
				return c64d_peek_cartridge(addr);
			}
			else if (kernal_in)
			{
				return c64memrom_kernal64_rom[addr & 0x1fff];
			}
			else
			{
				return mem_ram[addr];
			}
	}

	return mem_ram[addr];
}

void c64d_peek_memory_c64(uint8_t *buffer, int addrStart, int addrEnd)
{
	int addr;
	uint8_t *bufPtr = buffer + addrStart;
	for (addr = addrStart; addr < addrEnd; addr++)
	{
		*bufPtr++ = c64d_peek_c64(addr);
	}
}

void c64d_copy_ram_memory_c64(uint8_t *buffer, int addrStart, int addrEnd)
{
	int addr;
	uint8_t *bufPtr = buffer + addrStart;
	for (addr = addrStart; addr < addrEnd; addr++)
	{
		*bufPtr++ = mem_ram[addr];
	}
	
	if (addrStart == 0)
	{
		buffer[0x0000] = pport.dir_read;
		if (addrEnd != 1)
		{
			buffer[0x0001] = (pport.data_read & (0xff - (((!pport.data_set_bit6) << 6) + ((!pport.data_set_bit7) << 7))));
		}
	}
	else if (addrStart == 1)
	{
		buffer[0x0001] = (pport.data_read & (0xff - (((!pport.data_set_bit6) << 6) + ((!pport.data_set_bit7) << 7))));
	}
}


//
void c64d_peek_whole_map_c64(uint8_t *memoryBuffer)
{
	int exrom;
	int game;
	uint8_t basic_in;
	uint8_t kernal_in;
	uint8_t char_in;
	uint8_t io_in;
	
	uint8_t charen;
	uint8_t hiram;
	uint8_t loram;
	
	uint8_t mem01;
	
	
	uint32 addr;
	uint8 *bufPtr;

	memoryBuffer[0x0000] = pport.dir_read;
	memoryBuffer[0x0001] = (pport.data_read & (0xff - (((!pport.data_set_bit6) << 6) + ((!pport.data_set_bit7) << 7))));
	
	// TODO: change this into memory configs and func calls as it's done in VICE,
	//       note we need to peek into ROM directly to not trigger alarms
	
	
	// exrom & game are set as here: https://www.c64-wiki.com/index.php/Bank_Switching
	exrom = !export.exrom;
	game = !export.game;
	
	mem01 = (~pport.dir | pport.data) & 0x07;
	
	charen = (mem01 & 0x04) == 0x04;
	hiram  = (mem01 & 0x02) == 0x02;
	loram  = (mem01 & 0x01) == 0x01;
	
	basic_in = (pport.data_read & 0x03) == 0x03;
	kernal_in = pport.data_read & 2;
	char_in = (pport.data_read & 3) && ~(pport.data_read & 4);
	io_in = (pport.data_read & 3) && (pport.data_read & 4);
	
	bufPtr = memoryBuffer + 2;
	
	// 00
	for (addr = 0x0002; addr < 0x1000; addr++)
	{
		*bufPtr++ = mem_ram[addr];
	}

	// 10 to 7F
	if (exrom == 1 && game == 0)
	{
		for (addr = 0x1000; addr < 0x8000; addr++)
		{
			*bufPtr++ = c64d_peek_cartridge(addr);
		}
	}
	else
	{
		for (addr = 0x1000; addr < 0x8000; addr++)
		{
			*bufPtr++ = mem_ram[addr];
		}
	}
	
	// 80 to 9F
	if (exrom == 1 && game == 0)
	{
		for (addr = 0x8000; addr < 0xA000; addr++)
		{
			*bufPtr++ = c64d_peek_cartridge(addr);
		}
	}
	else if (exrom == 0)
	{
		if (hiram == 1 && loram == 1)
		{
			for (addr = 0x8000; addr < 0xA000; addr++)
			{
				*bufPtr++ = c64d_peek_cartridge(addr);
			}
		}
		else
		{
			for (addr = 0x8000; addr < 0xA000; addr++)
			{
				*bufPtr++ = mem_ram[addr];
			}
		}
	}
	else
	{
		for (addr = 0x8000; addr < 0xA000; addr++)
		{
			*bufPtr++ = mem_ram[addr];
		}
	}
	
	// A0 to BF
	if (exrom == 1 && game == 0)
	{
		for (addr = 0xA000; addr < 0xC000; addr++)
		{
			*bufPtr++ = c64d_peek_cartridge(addr);
		}
	}
	else if (game == 1)
	{
		if (basic_in)
		{
			for (addr = 0xA000; addr < 0xC000; addr++)
			{
				*bufPtr++ = c64memrom_basic64_rom[addr & 0x1fff];
			}
		}
		else
		{
			for (addr = 0xA000; addr < 0xC000; addr++)
			{
				*bufPtr++ = mem_ram[addr];
			}
		}
	}
	else
	{
		if (basic_in)
		{
			for (addr = 0xA000; addr < 0xC000; addr++)
			{
				*bufPtr++ = c64d_peek_cartridge(addr);
			}
		}
		else
		{
			for (addr = 0xA000; addr < 0xC000; addr++)
			{
				*bufPtr++ = mem_ram[addr];
			}
		}
	}
	
	// C0 - CF
	if (exrom == 1 && game == 0)
	{
		for (addr = 0xC000; addr < 0xD000; addr++)
		{
			*bufPtr++ = c64d_peek_cartridge(addr);
		}
	}
	else
	{
		for (addr = 0xC000; addr < 0xD000; addr++)
		{
			*bufPtr++ = mem_ram[addr];
		}
	}
	
	// D0 - DD
	if (io_in)
	{
		for (addr = 0xD000; addr < 0xDE00; addr++)
		{
			*bufPtr++ = c64d_peek_io(addr);
		}
	}
	else if (char_in)
	{
		for (addr = 0xD000; addr < 0xDE00; addr++)
		{
			*bufPtr++ = mem_chargen_rom[addr & 0x0fff];
		}
	}
	else
	{
		for (addr = 0xD000; addr < 0xDE00; addr++)
		{
			*bufPtr++ = mem_ram[addr];
		}
	}

	// DE - DF

	if (mem_cartridge_type != CARTRIDGE_NONE)
	{
		// TODO: check if peek does not create triggers/alarms...
		for (addr = 0xDE00; addr < 0xDF00; addr++)
		{
			*bufPtr++ = c64io_de00_peek(addr);
		}
		for (addr = 0xDF00; addr < 0xE000; addr++)
		{
			*bufPtr++ = c64io_df00_peek(addr);
		}
	}
	else
	{
		if (io_in)
		{
			for (addr = 0xDE00; addr < 0xE000; addr++)
			{
				*bufPtr++ = c64d_peek_io(addr);
			}
		}
		else if (char_in)
		{
			for (addr = 0xDE00; addr < 0xE000; addr++)
			{
				*bufPtr++ = mem_chargen_rom[addr & 0x0fff];
			}
		}
		else
		{
			for (addr = 0xDE00; addr < 0xE000; addr++)
			{
				*bufPtr++ = mem_ram[addr];
			}
		}
	}

	// E0-FF
	if (exrom == 1 && game == 0)
	{
		for (addr = 0xE000; addr < 0x10000; addr++)
		{
			*bufPtr++ = c64d_peek_cartridge(addr);
		}
	}
	else if (kernal_in)
	{
		for (addr = 0xE000; addr < 0x10000; addr++)
		{
			*bufPtr++ = c64memrom_kernal64_rom[addr & 0x1fff];
		}
	}
	else
	{
		for (addr = 0xE000; addr < 0x10000; addr++)
		{
			*bufPtr++ = mem_ram[addr];
		}
	}
}


void c64d_get_exrom_game(uint8_t *exrom, uint8_t *game)
{
	*exrom = !export.exrom;
	*game = !export.game;
}

void c64d_get_ultimax_phi(uint8_t *ultimax_phi1, uint8_t *ultimax_phi2)
{
	*ultimax_phi1 = export.ultimax_phi1;
	*ultimax_phi2 = export.ultimax_phi2;
}


void c64d_mem_ram_write_c64(uint16_t addr, uint8_t value)
{
	mem_ram[addr] = value;
}

void c64d_mem_ram_fill_c64(uint16_t addr, uint16_t size, uint8_t value)
{
	memset((mem_ram + addr), value, size);
}

uint8_t c64d_mem_ram_read_c64(uint16_t addr)
{
	return mem_ram[addr];
}

void c64d_copy_whole_mem_ram_c64(uint8_t *destBuf)
{
	memcpy(destBuf, mem_ram, 0x010000);
	
	destBuf[0x0000] = pport.dir_read;
	destBuf[0x0001] = (pport.data_read & (0xff - (((!pport.data_set_bit6) << 6) + ((!pport.data_set_bit7) << 7))));
}

int c64d_prev_val_of_FD84 = -1;
int c64d_prev_val_of_FD85 = -1;

void c64d_patch_kernal_fast_boot()
{
	c64d_prev_val_of_FD84 = c64memrom_rom64_read(0xFD84);
	c64d_prev_val_of_FD85 = c64memrom_rom64_read(0xFD85);
	
	c64memrom_rom64_store(0xFD84, 0xA0);
	c64memrom_rom64_store(0xFD85, 0x00);
}

void c64d_un_patch_kernal_fast_boot()
{
	if (c64d_prev_val_of_FD84 != -1)
	{
		c64memrom_rom64_store(0xFD84, c64d_prev_val_of_FD84);
		c64memrom_rom64_store(0xFD85, c64d_prev_val_of_FD85);
	}
}


// this modifies c64 state
//if (addr >= 0xd000 && addr < 0xe000)
//{
//	if (addr == 0xde00)
//	{
//		LOGD("addr=%4.4x", addr);
//		return peek_bank_io(addr);
//	}
//}


//d000
//vicii_read
//
//d100
//d200
//d300
//vicii_peek
//
//d400
//d500
//d600
//d700
//sid_peek
//
//case 0xd800:
//case 0xd900:
//case 0xda00:
//case 0xdb00:
//return colorram_read(addr);
//
//dc00
//cia1_peek(addr);
//
//dd00
//cia2_peek(addr);
//
//de00
//vicii_read_phi1(void)
//
//df00
//vicii_read_phi1(void)
//



// Slaj: unfortunately mem_bank_peek DOES affect I/O state :( :(
uint8_t mem_bank_peek(int bank, uint16_t addr, void *context)
{
    switch (bank) {
        case 0:                   /* current */
            /* we must check for which bank is currently active, and only use peek_bank_io
               when needed to avoid side effects */
            if (c64meminit_io_config[mem_config]) {
                if ((addr >= 0xd000) && (addr < 0xe000)) {
                    return peek_bank_io(addr);
                }
            }
            return mem_read(addr);
            break;
        case 3:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                return peek_bank_io(addr);
            }
        case 4:                   /* cart */
            return cartridge_peek_mem(addr);
    }
    return mem_bank_read(bank, addr, context);
}

void mem_bank_write(int bank, uint16_t addr, uint8_t byte, void *context)
{
    switch (bank) {
        case 0:                   /* current */
            mem_store(addr, byte);
            return;
        case 3:                   /* io */
            if (addr >= 0xd000 && addr < 0xe000) {
                store_bank_io(addr, byte);
                return;
            }
        case 2:                   /* rom */
            if (addr >= 0xa000 && addr <= 0xbfff) {
                return;
            }
            if (addr >= 0xd000 && addr <= 0xdfff) {
                return;
            }
            if (addr >= 0xe000) {
                return;
            }
        case 1:                   /* ram */
            break;
    }
    mem_ram[addr] = byte;
}

static int mem_dump_io(void *context, uint16_t addr)
{
    if ((addr >= 0xdc00) && (addr <= 0xdc3f)) {
        return ciacore_dump(machine_context.cia1);
    } else if ((addr >= 0xdd00) && (addr <= 0xdd3f)) {
        return ciacore_dump(machine_context.cia2);
    }
    return -1;
}

mem_ioreg_list_t *mem_ioreg_list_get(void *context)
{
    mem_ioreg_list_t *mem_ioreg_list = NULL;

    mon_ioreg_add_list(&mem_ioreg_list, "CIA1", 0xdc00, 0xdc0f, mem_dump_io, NULL, IO_MIRROR_NONE);
    mon_ioreg_add_list(&mem_ioreg_list, "CIA2", 0xdd00, 0xdd0f, mem_dump_io, NULL, IO_MIRROR_NONE);

    io_source_ioreg_add_list(&mem_ioreg_list);

    return mem_ioreg_list;
}

void mem_get_screen_parameter(uint16_t *base, uint8_t *rows, uint8_t *columns, int *bank)
{
    *base = ((vicii_peek(0xd018) & 0xf0) << 6) | ((~cia2_peek(0xdd00) & 0x03) << 14);
    *rows = 25;
    *columns = 40;
    *bank = 0;
}

/* ------------------------------------------------------------------------- */

void mem_color_ram_to_snapshot(uint8_t *color_ram)
{
    memcpy(color_ram, mem_color_ram, 0x400);
}

void mem_color_ram_from_snapshot(uint8_t *color_ram)
{
    memcpy(mem_color_ram, color_ram, 0x400);
}

/* ------------------------------------------------------------------------- */

/* UI functions (used to distinguish between x64 and x64sc) */
#ifdef USE_BEOS_UI
int c64_mem_ui_init_early(void)
{
    return c64scui_init_early();
}
#endif

int c64_mem_ui_init(void)
{
    return c64scui_init();
}

void c64_mem_ui_shutdown(void)
{
    c64scui_shutdown();
}
