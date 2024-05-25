/*
 * c64cpusc.c - Emulation of the C64 6510 processor for x64sc.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#define USE_CHAMP_PROFILER

#include "vice.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "cpmcart.h"
#include "monitor.h"
#include "vicii-cycle.h"

#include "c64.h"
#include "cia.h"
#include "ViceWrapper.h"
#include "DebuggerDefs.h"

/* ------------------------------------------------------------------------- */

/* Global clock counter.  */
CLOCK maincpu_clk = 0L;
/* if != 0, exit when this many cycles have been executed */
CLOCK maincpu_clk_limit = 0L;

CLOCK c64d_maincpu_clk = 0L;
CLOCK c64d_maincpu_current_instruction_clk = 0L;

#define REWIND_FETCH_OPCODE(clock) /*clock-=2*/

/* Mask: BA low */
int maincpu_ba_low_flags = 0;

int c64d_c64_do_cycle();
int c64d_c64_instruction_cycle = 0;

//#define CLK_INC()                                  \
//    interrupt_delay();                             \
//    maincpu_clk++;                                 \
//    maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_VICII; \
//    maincpu_ba_low_flags |= vicii_cycle()

#define CLK_INC()                                  \
	interrupt_delay();                             \
	maincpu_clk++;                                 \
	c64d_maincpu_clk++;                            \
	maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_VICII; \
	maincpu_ba_low_flags |= c64d_c64_do_cycle()


/* Skip cycle implementation */

#define SKIP_CYCLE 0

/* Opcode info updated in FETCH_OPCODE.
   Needed for CLI/SEI detection in vicii_steal_cycles. */
#define OPCODE_UPDATE_IN_FETCH


/* opcode_t etc */

#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS

#define opcode_t DWORD

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 (opcode >> 8)

#define SET_OPCODE(o) (opcode) = o;

#else /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */

#define opcode_t         \
    struct {             \
        BYTE ins;        \
        union {          \
            BYTE op8[2]; \
            WORD op16;   \
        } op;            \
    }

#define p0 (opcode.ins)
#define p2 (opcode.op.op16)

#ifdef WORDS_BIGENDIAN

#define p1 (opcode.op.op8[1])

#define SET_OPCODE(o)                          \
    do {                                       \
        opcode.ins = (o) & 0xff;               \
        opcode.op.op8[1] = ((o) >> 8) & 0xff;  \
        opcode.op.op8[0] = ((o) >> 16) & 0xff; \
    } while (0)

#else /* !WORDS_BIGENDIAN */

#define p1 (opcode.op.op8[0])

#define SET_OPCODE(o)                          \
    do {                                       \
        opcode.ins = (o) & 0xff;               \
        opcode.op.op8[0] = ((o) >> 8) & 0xff;  \
        opcode.op.op8[1] = ((o) >> 16) & 0xff; \
    } while (0)

#endif

#endif /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */

/* HACK: memmap updates for the reg_pc < bank_limit case */
#ifdef FEATURE_CPUMEMHISTORY
#define MEMMAP_UPDATE(addr) memmap_mem_update(addr, 0)
#else
#define MEMMAP_UPDATE(addr)
#endif

/* FETCH_OPCODE implementation(s) */
#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS
#define FETCH_OPCODE(o)                                        \
    do {                                                       \
        if (((int)reg_pc) < bank_limit) {                      \
            check_ba();                                        \
            o = (*((DWORD *)(bank_base + reg_pc)) & 0xffffff); \
            MEMMAP_UPDATE(reg_pc);                             \
            SET_LAST_OPCODE(p0);                               \
            CLK_INC();                                         \
            check_ba();                                        \
            CLK_INC();                                         \
            if (fetch_tab[o & 0xff]) {                         \
                check_ba();                                    \
                CLK_INC();                                     \
            }                                                  \
        } else {                                               \
            o = LOAD(reg_pc);                                  \
            SET_LAST_OPCODE(p0);                               \
            CLK_INC();                                         \
            o |= LOAD(reg_pc + 1) << 8;                        \
            CLK_INC();                                         \
            if (fetch_tab[o & 0xff]) {                         \
                o |= (LOAD(reg_pc + 2) << 16);                 \
                CLK_INC();                                     \
            }                                                  \
        }                                                      \
    } while (0)

#else /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */
#define FETCH_OPCODE(o)                                          \
    do {                                                         \
        if (((int)reg_pc) < bank_limit) {                        \
            check_ba();                                          \
            (o).ins = *(bank_base + reg_pc);                     \
            MEMMAP_UPDATE(reg_pc);                               \
            SET_LAST_OPCODE(p0);                                 \
            CLK_INC();                                           \
            check_ba();                                          \
            (o).op.op16 = *(bank_base + reg_pc + 1);             \
            CLK_INC();                                           \
            if (fetch_tab[(o).ins]) {                            \
                check_ba();                                      \
                (o).op.op16 |= (*(bank_base + reg_pc + 2) << 8); \
                CLK_INC();                                       \
            }                                                    \
        } else {                                                 \
            (o).ins = LOAD(reg_pc);                              \
            SET_LAST_OPCODE(p0);                                 \
            CLK_INC();                                           \
            (o).op.op16 = LOAD(reg_pc + 1);                      \
            CLK_INC();                                           \
            if (fetch_tab[(o).ins]) {                            \
                (o).op.op16 |= (LOAD(reg_pc + 2) << 8);          \
                CLK_INC();                                       \
            }                                                    \
        }                                                        \
    } while (0)

#endif /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */

static void check_and_run_alternate_cpu(void)
{
    cpmcart_check_and_run_z80();
}

#define CHECK_AND_RUN_ALTERNATE_CPU check_and_run_alternate_cpu();

#define HAVE_Z80_REGS

//#include "../mainc64cpu.c"

///
/// "../mainc64cpu.c" starts here


/*
 * mainc64cpu.c - Emulation of the C64 6510 processor.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "6510core.h"
#include "alarm.h"

#ifdef FEATURE_CPUMEMHISTORY
#include "c64pla.h"
#endif

#include "clkguard.h"
#include "debug.h"
#include "interrupt.h"
#include "machine.h"
#include "mainc64cpu.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "mos6510.h"
#include "reu.h"
#include "snapshot.h"
#include "traps.h"
#include "vicetypes.h"

// c64d
#include "viciitypes.h"
//

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif


/* MACHINE_STUFF should define/undef
 
 - NEED_REG_PC
 
 */

/* ------------------------------------------------------------------------- */
#ifdef VICE_DEBUG
CLOCK debug_clk;
#endif

#define NEED_REG_PC

/* ------------------------------------------------------------------------- */

/* Implement the hack to make opcode fetches faster.  */
#define JUMP(addr)                                                                         \
do {                                                                                   \
reg_pc = (unsigned int)(addr);                                                     \
if (reg_pc >= (unsigned int)bank_limit || reg_pc < (unsigned int)bank_start) {     \
mem_mmu_translate((unsigned int)(addr), &bank_base, &bank_start, &bank_limit); \
}                                                                                  \
} while (0)

/* ------------------------------------------------------------------------- */

int check_ba_low = 0;

inline static void interrupt_delay(void)
{
	while (maincpu_clk >= alarm_context_next_pending_clk(maincpu_alarm_context)) {
		alarm_context_dispatch(maincpu_alarm_context, maincpu_clk);
	}
	
	if (maincpu_int_status->irq_clk <= maincpu_clk) {
		maincpu_int_status->irq_delay_cycles++;
	}
	
	if (maincpu_int_status->nmi_clk <= maincpu_clk) {
		maincpu_int_status->nmi_delay_cycles++;
	}
}

static void maincpu_steal_cycles(void)
{
	interrupt_cpu_status_t *cs = maincpu_int_status;
	BYTE opcode;
	
	if (maincpu_ba_low_flags & MAINCPU_BA_LOW_VICII) {
		vicii_steal_cycles();
		maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_VICII;
	}
	
	if (maincpu_ba_low_flags & MAINCPU_BA_LOW_REU) {
		reu_dma_start();
		maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_REU;
	}
	
	while (maincpu_clk >= alarm_context_next_pending_clk(maincpu_alarm_context)) {
		alarm_context_dispatch(maincpu_alarm_context, maincpu_clk);
	}
	
	/* special handling for steals during opcodes */
	opcode = OPINFO_NUMBER(*cs->last_opcode_info_ptr);
	switch (opcode) {
			/* SHA */
		case 0x93:
			if (check_ba_low) {
				OPINFO_SET_ENABLES_IRQ(*cs->last_opcode_info_ptr, 1);
			}
			break;
			
			/* SHS */
		case 0x9b:
			/* (fall through) */
			/* SHY */
		case 0x9c:
			/* (fall through) */
			/* SHX */
		case 0x9e:
			/* (fall through) */
			/* SHA */
		case 0x9f:
			/* this is a hacky way of signaling SET_ABS_SH_I() that
			 cycles were stolen before the write */
			if (check_ba_low) {
				OPINFO_SET_ENABLES_IRQ(*cs->last_opcode_info_ptr, 1);
			}
			break;
			
			/* ANE */
		case 0x8b:
			/* this is a hacky way of signaling ANE() that
			 cycles were stolen after the first fetch */
			/* (fall through) */
			
			/* CLI */
		case 0x58:
			/* this is a hacky way of signaling CLI() that it
			 shouldn't delay the interrupt */
			OPINFO_SET_ENABLES_IRQ(*cs->last_opcode_info_ptr, 1);
			break;
			
		default:
			break;
	}
	
	/* SEI: do not update interrupt delay counters */
	if (opcode != 0x78) {
		if (cs->irq_delay_cycles == 0 && cs->irq_clk < maincpu_clk) {
			cs->irq_delay_cycles++;
		}
	}
	
	if (cs->nmi_delay_cycles == 0 && cs->nmi_clk < maincpu_clk) {
		cs->nmi_delay_cycles++;
	}
}

inline static void check_ba(void)
{
	if (maincpu_ba_low_flags) {
#ifdef VICE_DEBUG
		CLOCK old_maincpu_clk = maincpu_clk;
#endif
		maincpu_steal_cycles();
#ifdef VICE_DEBUG
		if (debug_clk == old_maincpu_clk) {
			debug_clk = maincpu_clk;
		}
#endif
	}
}

#ifdef FEATURE_CPUMEMHISTORY

/* FIXME do proper ROM/RAM/IO tests */

inline static void memmap_mem_update(unsigned int addr, int write)
{
	unsigned int type = MEMMAP_RAM_R;
	
	if (write) {
		if ((addr >= 0xd000) && (addr <= 0xdfff)) {
			type = MEMMAP_I_O_W;
		} else {
			type = MEMMAP_RAM_W;
		}
	} else {
		switch (addr >> 12) {
			case 0xa:
			case 0xb:
			case 0xe:
			case 0xf:
				if (pport.data_read & (1 << ((addr >> 14) & 1))) {
					type = MEMMAP_ROM_R;
				} else {
					type = MEMMAP_RAM_R;
				}
				break;
			case 0xd:
				type = MEMMAP_I_O_R;
				break;
			default:
				type = MEMMAP_RAM_R;
				break;
		}
		if (memmap_state & MEMMAP_STATE_OPCODE) {
			/* HACK: transform R to X */
			type >>= 2;
			memmap_state &= ~(MEMMAP_STATE_OPCODE);
		} else if (memmap_state & MEMMAP_STATE_INSTR) {
			/* ignore operand reads */
			type = 0;
		}
	}
	monitor_memmap_store(addr, type);
}

void memmap_mem_store(unsigned int addr, unsigned int value)
{
	memmap_mem_update(addr, 1);
	
	c64d_mark_c64_cell_write(addr, value);

	(*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value));
}

/* read byte, check BA and mark as read */
BYTE memmap_mem_read(unsigned int addr)
{
	check_ba();
	
	memmap_mem_update(addr, 0);
	
	c64d_mark_c64_cell_read(addr);

	return (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr));
}

#ifndef STORE
#define STORE(addr, value) \
memmap_mem_store(addr, value)
#endif

#ifndef LOAD
#define LOAD(addr) \
memmap_mem_read(addr)
#endif

#ifndef LOAD_CHECK_BA_LOW
#define LOAD_CHECK_BA_LOW(addr) \
check_ba_low = 1;           \
memmap_mem_read(addr);      \
check_ba_low = 0
#endif

#ifndef STORE_ZERO
#define STORE_ZERO(addr, value) \
memmap_mem_store((addr) & 0xff, value)
#endif

#ifndef LOAD_ZERO
#define LOAD_ZERO(addr) \
memmap_mem_read((addr) & 0xff)
#endif

/* Route stack operations through memmap */

#define PUSH(val) memmap_mem_store((0x100 + (reg_sp--)), (BYTE)(val))
#define PULL()    memmap_mem_read(0x100 + (++reg_sp))
#define STACK_PEEK()  memmap_mem_read(0x100 + reg_sp)

#endif /* FEATURE_CPUMEMHISTORY */

inline static BYTE mem_read_check_ba(unsigned int addr)
{
	//LOGD("mem_read_check_ba: %4.4x", addr);
	
	check_ba();
	
	c64d_mark_c64_cell_read(addr);
	
	return (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr));
}

inline static void c64d_mem_store(unsigned int addr, unsigned char value)
{
	//LOGD("c64d_mem_store: %4.4x %2.2x", addr, value);
	
	c64d_mark_c64_cell_write(addr, value);
	
	(*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value));
}

inline static void c64d_mem_store_zero(unsigned int addr, unsigned char value)
{
	//LOGD("c64d_mem_store_zero: %4.4x %2.2x", addr, value);
	
	c64d_mark_c64_cell_write(addr, value);
	
	(*_mem_write_tab_ptr[0])((WORD)(addr), (BYTE)(value));
}

void c64d_mem_write_c64(unsigned int addr, unsigned char value)
{
	c64d_mem_store(addr, value);
}

void c64d_mem_write_c64_no_mark(unsigned int addr, unsigned char value)
{
	(*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value));
}

#ifndef STORE
#define STORE(addr, value) \
c64d_mem_store((WORD)(addr), (BYTE)(value));
#endif

#ifndef LOAD
#define LOAD(addr) \
mem_read_check_ba(addr)
#endif

#ifndef LOAD_CHECK_BA_LOW
#define LOAD_CHECK_BA_LOW(addr) \
check_ba_low = 1;           \
mem_read_check_ba(addr);    \
check_ba_low = 0
#endif

#ifndef STORE_ZERO
#define STORE_ZERO(addr, value) \
c64d_mem_store_zero(addr, value);
#endif

#ifndef LOAD_ZERO
#define LOAD_ZERO(addr) \
mem_read_check_ba((addr) & 0xff)
#endif

/* Route stack operations through read/write handlers */

#ifndef PUSH
#define PUSH(val) (*_mem_write_tab_ptr[0x01])((WORD)(0x100 + (reg_sp--)), (BYTE)(val))
#endif

#ifndef PULL
#define PULL()    mem_read_check_ba(0x100 + (++reg_sp))
#endif

#ifndef STACK_PEEK
#define STACK_PEEK()  mem_read_check_ba(0x100 + reg_sp)
#endif

#ifndef DMA_FUNC
static void maincpu_generic_dma(void)
{
	/* Generic DMA hosts can be implemented here.
	 For example a very accurate REU emulation. */
}
#define DMA_FUNC maincpu_generic_dma()
#endif

#ifndef DMA_ON_RESET
#define DMA_ON_RESET
#endif

#ifndef CPU_ADDITIONAL_RESET
#define CPU_ADDITIONAL_RESET()
#endif

#ifndef CPU_ADDITIONAL_INIT
#define CPU_ADDITIONAL_INIT()
#endif

/* ------------------------------------------------------------------------- */

struct interrupt_cpu_status_s *maincpu_int_status = NULL;
alarm_context_t *maincpu_alarm_context = NULL;
clk_guard_t *maincpu_clk_guard = NULL;
monitor_interface_t *maincpu_monitor_interface = NULL;

/* This flag is an obsolete optimization. It's always 0 for the x64sc CPU,
 but has to be kept for the common code. */
int maincpu_rmw_flag = 0;

/* Information about the last executed opcode.  This is used to know the
 number of write cycles in the last executed opcode and to delay interrupts
 by one more cycle if necessary, as happens with conditional branch opcodes
 when the branch is taken.  */
unsigned int last_opcode_info;

/* Address of the last executed opcode. This is used by watchpoints. */
unsigned int last_opcode_addr;

/* Number of write cycles for each 6510 opcode.  */
const CLOCK maincpu_opcode_write_cycles[] = {
	/* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
	/* $00 */  3, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, /* $00 */
	/* $10 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $10 */
	/* $20 */  2, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $20 */
	/* $30 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $30 */
	/* $40 */  0, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, /* $40 */
	/* $50 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $50 */
	/* $60 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $60 */
	/* $70 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $70 */
	/* $80 */  0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, /* $80 */
	/* $90 */  0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, /* $90 */
	/* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $A0 */
	/* $B0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $B0 */
	/* $C0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $C0 */
	/* $D0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $D0 */
	/* $E0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $E0 */
	/* $F0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2  /* $F0 */
	/* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
};

/* Public copy of the CPU registers.  As putting the registers into the
 function makes it faster, you have to generate a `TRAP' interrupt to have
 the values copied into this struct.  */
mos6510_regs_t maincpu_regs;

/* ------------------------------------------------------------------------- */

monitor_interface_t *maincpu_monitor_interface_get(void)
{
	maincpu_monitor_interface->cpu_regs = &maincpu_regs;
	maincpu_monitor_interface->cpu_R65C02_regs = NULL;
	maincpu_monitor_interface->dtv_cpu_regs = NULL;
	maincpu_monitor_interface->z80_cpu_regs = NULL;
	maincpu_monitor_interface->h6809_cpu_regs = NULL;
	
	maincpu_monitor_interface->int_status = maincpu_int_status;
	
	maincpu_monitor_interface->clk = &maincpu_clk;
	
	maincpu_monitor_interface->current_bank = 0;
	maincpu_monitor_interface->mem_bank_list = mem_bank_list;
	maincpu_monitor_interface->mem_bank_from_name = mem_bank_from_name;
	maincpu_monitor_interface->mem_bank_read = mem_bank_read;
	maincpu_monitor_interface->mem_bank_peek = mem_bank_peek;
	maincpu_monitor_interface->mem_bank_write = mem_bank_write;
	
	maincpu_monitor_interface->mem_ioreg_list_get = mem_ioreg_list_get;
	
	maincpu_monitor_interface->toggle_watchpoints_func = mem_toggle_watchpoints;
	
	maincpu_monitor_interface->set_bank_base = NULL;
	maincpu_monitor_interface->get_line_cycle = machine_get_line_cycle;
	
	return maincpu_monitor_interface;
}

/* ------------------------------------------------------------------------- */

void maincpu_early_init(void)
{
	maincpu_int_status = interrupt_cpu_status_new();
}

void maincpu_init(void)
{
	interrupt_cpu_status_init(maincpu_int_status, &last_opcode_info);
	
	/* cpu specifix additional init routine */
	CPU_ADDITIONAL_INIT();
}

void maincpu_shutdown(void)
{
	interrupt_cpu_status_destroy(maincpu_int_status);
}

static void cpu_reset(void)
{
	int preserve_monitor;
	
	preserve_monitor = maincpu_int_status->global_pending_int & IK_MONITOR;
	
	interrupt_cpu_status_reset(maincpu_int_status);
	
	if (preserve_monitor) {
		interrupt_monitor_trap_on(maincpu_int_status);
	}
	
	maincpu_clk = 6; /* # of clock cycles needed for RESET.  */
	c64d_maincpu_clk = 6;
	
	/* CPU specific extra reset routine, currently only used
	 for 8502 fast mode refresh cycle. */
	CPU_ADDITIONAL_RESET();
	
	/* Do machine-specific initialization.  */
	machine_reset();
}

void maincpu_reset(void)
{
	cpu_reset();
}

/* ------------------------------------------------------------------------- */

/* Return nonzero if a pending NMI should be dispatched now.  This takes
 account for the internal delays of the 6510, but does not actually check
 the status of the NMI line.  */
inline static int interrupt_check_nmi_delay(interrupt_cpu_status_t *cs,
											CLOCK cpu_clk)
{
	unsigned int delay_cycles = INTERRUPT_DELAY;
	
	/* BRK (0x00) delays the NMI by one opcode.  */
	/* TODO DO_INTERRUPT sets last opcode to 0: can NMI occur right after IRQ? */
	if (OPINFO_NUMBER(*cs->last_opcode_info_ptr) == 0x00) {
		return 0;
	}
	
	/* Branch instructions delay IRQs and NMI by one cycle if branch
	 is taken with no page boundary crossing.  */
	if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr)) {
		delay_cycles++;
	}
	
	if (cs->nmi_delay_cycles >= delay_cycles) {
		return 1;
	}
	
	return 0;
}

/* Return nonzero if a pending IRQ should be dispatched now.  This takes
 account for the internal delays of the 6510, but does not actually check
 the status of the IRQ line.  */
inline static int interrupt_check_irq_delay(interrupt_cpu_status_t *cs,
											CLOCK cpu_clk)
{
	unsigned int delay_cycles = INTERRUPT_DELAY;
	
	/* Branch instructions delay IRQs and NMI by one cycle if branch
	 is taken with no page boundary crossing.  */
	if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr)) {
		delay_cycles++;
	}
	
	if (cs->irq_delay_cycles >= delay_cycles) {
		if (!OPINFO_ENABLES_IRQ(*cs->last_opcode_info_ptr)) {
			return 1;
		} else {
			cs->global_pending_int |= IK_IRQPEND;
		}
	}
	
	return 0;
}

/* ------------------------------------------------------------------------- */

#ifdef NEED_REG_PC
unsigned int reg_pc;
#endif

static BYTE **o_bank_base;
static int *o_bank_start;
static int *o_bank_limit;

void maincpu_resync_limits(void)
{
	if (o_bank_base) {
		mem_mmu_translate(reg_pc, o_bank_base, o_bank_start, o_bank_limit);
	}
}

BYTE reg_a = 0;
BYTE reg_x = 0;
BYTE reg_y = 0;
BYTE reg_p = 0;
BYTE reg_sp = 0;
BYTE flag_n = 0;
BYTE flag_z = 0;
#ifndef NEED_REG_PC
unsigned int reg_pc;
#endif
BYTE *bank_base;
int bank_start = 0;
int bank_limit = 0;

//
// proof of concept of the profiler compatible with Champ, this needs to be moved
// and generalised to be used also with other platforms not only C64
//
#if defined(USE_CHAMP_PROFILER)

void c64d_profiler_init();
void c64d_set_debug_mode(int newMode);

typedef struct {
	uint32_t index;
	uint16_t pc;
	uint8_t post;
	enum {
		dataType_u8, dataType_s8, dataType_u16, dataType_s16
	} data_type;
	enum
	{
		MEMORY,
		REGISTER_A,
		REGISTER_X,
		REGISTER_Y
	} type;
	uint16_t memory_address;
} r_watch;

r_watch *c64d_profiler_watches = 0;
uint8_t c64d_profiler_watches_allocated = 0;
size_t c64d_profiler_watch_count = 0;
int32_t c64d_profiler_watch_offset_for_pc_and_post[0x20000];

uint16_t c64d_profiler_old_pc = 0x0000;
uint8_t  c64d_profiler_trace_stack[0x100];
uint8_t  c64d_profiler_trace_stack_pointer = 0xff;
uint16_t c64d_profiler_trace_stack_function[0x100];
uint64_t c64d_profiler_cycles_per_function[0x10000];
uint64_t c64d_profiler_calls_per_function[0x10000];
uint64_t c64d_profiler_cpu_total_cycles;
int c64d_profiler_last_cycles = -1;

int c64d_profiler_is_active = 0;
FILE *c64d_profiler_file_out = NULL;
int profiler_run_for_cycles = -1;

int profiler_pause_cpu_when_finished = 1;

// if numCycles == -1 run until stopped
void c64d_profiler_activate(char *fileName, int runForNumCycles, int pauseCpuWhenFinished)
{
	LOGD("c64d_activate_profiler: fileName=%s runForNumCycles=%d", (fileName == NULL ? "NULL" : fileName), runForNumCycles);
	if (fileName != NULL)
	{
		c64d_profiler_file_out = fopen(fileName, "wb");
	}
	profiler_run_for_cycles = runForNumCycles;
	profiler_pause_cpu_when_finished = pauseCpuWhenFinished;
	
	c64d_profiler_init();
}

void c64d_profiler_deactivate()
{
	if (c64d_profiler_file_out)
	{
		fclose(c64d_profiler_file_out);
		c64d_profiler_file_out = NULL;
		profiler_run_for_cycles = -1;
	}
	
	c64d_profiler_is_active = 0;
	
	if (profiler_pause_cpu_when_finished)
	{
		c64d_set_debug_mode(DEBUGGER_MODE_PAUSED);
	}
}

void c64d_profiler_init()
{
	LOGD("c64d_profiler_init");
	c64d_profiler_cpu_total_cycles = 0;
	c64d_profiler_last_cycles = -1;
	
	memset(c64d_profiler_cycles_per_function, 0, sizeof(c64d_profiler_cycles_per_function));
	memset(c64d_profiler_calls_per_function, 0, sizeof(c64d_profiler_calls_per_function));

	c64d_profiler_is_active = 1;
}

void c64d_profiler_jsr(uint16_t target_address)
{
	//	LOGD("c64d_profiler_jsr: addr pc=%x", reg_pc);
	if (c64d_profiler_is_active)
	{
		// push PC - 1 because target address has already been read
		c64d_profiler_trace_stack_function[c64d_profiler_trace_stack_pointer] = target_address;
		c64d_profiler_trace_stack[c64d_profiler_trace_stack_pointer] = reg_sp;
		c64d_profiler_trace_stack_pointer--;
		c64d_profiler_calls_per_function[target_address]++;
		
		if (c64d_profiler_file_out)
		{
			fprintf(c64d_profiler_file_out, "jsr 0x%04x %llu\n", target_address, c64d_profiler_cpu_total_cycles);
			fflush(c64d_profiler_file_out);
		}
	}
}

void c64d_profiler_rts()
{
	//	LOGD("c64d_profiler_rts: addr pc=%x", reg_pc);
	
	if (c64d_profiler_is_active)
	{
		if (c64d_profiler_trace_stack[c64d_profiler_trace_stack_pointer + 1] == reg_sp - 2)
		{

			if (c64d_profiler_file_out)
			{
				fprintf(c64d_profiler_file_out, "rts %llu\n", c64d_profiler_cpu_total_cycles);
				fflush(c64d_profiler_file_out);
			}
			c64d_profiler_trace_stack_pointer++;
		}
	}
}

void c64d_profiler_start_handle_cpu_instruction()
{
	if (c64d_profiler_is_active)
	{
		c64d_profiler_old_pc = reg_pc;
	}
}

void c64d_profiler_end_cpu_instruction()
{
//	LOGD("c64d_c64_instruction_cycle=%d", c64d_c64_instruction_cycle);
	if (c64d_profiler_is_active)
	{
		c64d_profiler_cpu_total_cycles += c64d_c64_instruction_cycle;
		if (c64d_profiler_trace_stack_pointer < 0xff)
		{
			c64d_profiler_cycles_per_function[c64d_profiler_trace_stack_function[c64d_profiler_trace_stack_pointer + 1]] += c64d_c64_instruction_cycle;
		}
		
		if (c64d_profiler_file_out)
		{
			int raster_line = vicii.raster_line;
			int raster_cycle = vicii.raster_cycle;
			//int raster_irq_line = vicii.raster_irq_line;
			int bad_line = vicii.bad_line;
			

			unsigned int frameNum = c64d_get_frame_num();
			fprintf(c64d_profiler_file_out, "cpu %u %u %04x %02x %02x %02x %04x %02x %02x %llu %d %d %d\n",
					c64d_maincpu_clk,
					frameNum,
				   c64d_profiler_old_pc, reg_a, reg_x, reg_y, reg_pc, reg_sp,
					reg_p | (flag_n & 0x80) | P_UNUSED | ( (!(flag_z)) ? P_ZERO : 0),
					c64d_profiler_cpu_total_cycles,
					raster_line, raster_cycle, bad_line);
			if (c64d_profiler_cpu_total_cycles / 100000 != c64d_profiler_last_cycles)
			{
				c64d_profiler_last_cycles = c64d_profiler_cpu_total_cycles / 100000;
				fprintf(c64d_profiler_file_out, "cycles %d\n", c64d_profiler_last_cycles * 100000);
			}
			fflush(c64d_profiler_file_out);
		}
		
		if (profiler_run_for_cycles != -1)
		{
			if (c64d_profiler_cpu_total_cycles > profiler_run_for_cycles)
			{
				c64d_profiler_deactivate();
			}
		}
	}
}

#else

void c64d_profiler_init() {}
void c64d_profiler_activate(char *fileName, int runForNumCycles, int pauseCpuWhenFinished) {}
void c64d_profiler_deactivate() {}
void c64d_profiler_jsr() {}
void c64d_profiler_rts() {}
void c64d_profiler_start_handle_cpu_instruction() {}
void c64d_profiler_end_cpu_instruction() {}

#endif

void c64d_get_maincpu_regs(uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp, uint16 *pc,
						   uint8 *instructionCycle)
{
	*a = reg_a;
	*x = reg_x;
	*y = reg_y;

	// NV-BDIZC
	// 01111101
	*p = reg_p | (flag_n & 0x80) | P_UNUSED | ( (!(flag_z)) ? P_ZERO : 0);
	
	*sp = reg_sp;
	*pc = viceCurrentC64PC;
	*instructionCycle = c64d_c64_instruction_cycle;
}

void c64d_set_maincpu_regs_no_trap(uint8 a, uint8 x, uint8 y, uint8 p, uint8 sp)
{
	reg_a = a;
	reg_x = x;
	reg_y = y;
	
	reg_p = p;
	
	reg_sp = sp;
}

unsigned int c64d_get_maincpu_clock()
{
	return c64d_maincpu_clk;
}

void interrupt_maincpu_trigger_trap(void (*trap_func)(WORD, void *data),
									void *data);

int _c64d_new_pc = -1;

void _c64d_set_c64_pc_trap(WORD addr, void *data)
{
	WORD *newpc = data;
	maincpu_regs.pc = *newpc;
	
	_c64d_new_pc = -1;
}

void c64d_set_c64_pc(uint16 pc)
{
	viceCurrentC64PC = pc;
	_c64d_new_pc = pc;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_pc_trap, (void*)&_c64d_new_pc);
}

////
uint8 _c64d_maincpu_set_a; uint8 _c64d_maincpu_set_x; uint8 _c64d_maincpu_set_y; uint8 _c64d_maincpu_set_p; uint8 _c64d_maincpu_set_sp;

void _c64d_set_c64_maincpu_set_sp_trap(WORD addr, void *data)
{
	LOGD("_c64d_set_c64_maincpu_set_sp_trap: %x", _c64d_maincpu_set_sp);
	maincpu_regs.sp = _c64d_maincpu_set_sp;
}

void c64d_set_maincpu_set_sp(uint8 *sp)
{
	LOGD("c64d_set_maincpu_set_sp");
	_c64d_maincpu_set_sp = *sp;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_maincpu_set_sp_trap, NULL);
}

void _c64d_set_c64_maincpu_set_a_trap(WORD addr, void *data)
{
	LOGD("_c64d_set_c64_maincpu_set_a_trap: %x", _c64d_maincpu_set_a);
	maincpu_regs.a = _c64d_maincpu_set_a;
}

void c64d_set_maincpu_set_a(uint8 *a)
{
	LOGD("c64d_set_maincpu_set_a");
	_c64d_maincpu_set_a = *a;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_maincpu_set_a_trap, NULL);
}

void _c64d_set_c64_maincpu_set_x_trap(WORD addr, void *data)
{
	LOGD("_c64d_set_c64_maincpu_set_x_trap: %x", _c64d_maincpu_set_x);
	maincpu_regs.x = _c64d_maincpu_set_x;
	
}

void c64d_set_maincpu_set_x(uint8 *x)
{
	LOGD("c64d_set_maincpu_set_x");
	_c64d_maincpu_set_x = *x;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_maincpu_set_x_trap, NULL);
}

void _c64d_set_c64_maincpu_set_y_trap(WORD addr, void *data)
{
	LOGD("_c64d_set_c64_maincpu_set_y_trap: %x", _c64d_maincpu_set_y);
	maincpu_regs.y = _c64d_maincpu_set_y;
	
}

void c64d_set_maincpu_set_y(uint8 *y)
{
	LOGD("c64d_set_maincpu_set_y");
	_c64d_maincpu_set_y = *y;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_maincpu_set_y_trap, NULL);
}

void _c64d_set_c64_maincpu_set_p_trap(WORD addr, void *data)
{
	LOGD("_c64d_set_c64_maincpu_set_p_trap: %x", _c64d_maincpu_set_p);
	maincpu_regs.p = _c64d_maincpu_set_p;
}

void c64d_set_maincpu_set_p(uint8 *p)
{
	LOGD("c64d_set_maincpu_set_p");
	_c64d_maincpu_set_p = *p;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_maincpu_set_p_trap, NULL);
}

void _c64d_set_c64_maincpu_regs_trap(WORD addr, void *data)
{
	LOGD("_c64d_set_c64_maincpu_regs_trap: %x %x %x %x %x", _c64d_maincpu_set_a, _c64d_maincpu_set_x, _c64d_maincpu_set_y, _c64d_maincpu_set_p, _c64d_maincpu_set_sp);
	maincpu_regs.a  = _c64d_maincpu_set_a;
	maincpu_regs.x  = _c64d_maincpu_set_x;
	maincpu_regs.y  = _c64d_maincpu_set_y;
	maincpu_regs.p  = _c64d_maincpu_set_p;
	maincpu_regs.sp = _c64d_maincpu_set_sp;

}

void c64d_set_maincpu_regs(uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp)
{
	LOGD("c64d_set_maincpu_regs");
	_c64d_maincpu_set_a = *a;
	_c64d_maincpu_set_x = *x;
	_c64d_maincpu_set_y = *y;
	_c64d_maincpu_set_p = *p;
	_c64d_maincpu_set_sp = *sp;
	interrupt_maincpu_trigger_trap(_c64d_set_c64_maincpu_regs_trap, NULL);
}

void _c64d_update_roms_trap(WORD addr, void *data);

void c64d_trigger_update_roms()
{
	LOGD("c64d_update_roms");
	interrupt_maincpu_trigger_trap(_c64d_update_roms_trap, NULL);
}

void _c64d_maincpu_make_basic_run_trap(WORD addr, void *data)
{
	LOGD("_c64d_maincpu_make_basic_run_trap");
	
	// cursor off
	c64d_mem_write_c64_no_mark(0x00CC, 0xFF);
	
	// push jsr a659, jsr a533, jmp a7ae to stack
	// ae a7 33 a5
	c64d_mem_write_c64_no_mark(0x01FC, 0xAD);	// ret-1
	c64d_mem_write_c64_no_mark(0x01FD, 0xA7);
	c64d_mem_write_c64_no_mark(0x01FE, 0x32);		// ret-1
	c64d_mem_write_c64_no_mark(0x01FF, 0xA5);

	maincpu_regs.sp = 0xFB;
	maincpu_regs.pc = 0xA659;
	_c64d_new_pc = -1;
	
	c64d_reset_counters();

	LOGD("_c64d_maincpu_make_basic_run_trap done");
}

void c64d_maincpu_make_basic_run(uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp)
{
	LOGD("c64d_maincpu_make_basic_run");
	interrupt_maincpu_trigger_trap(_c64d_maincpu_make_basic_run_trap, NULL);
}

int debug_iterations_after_restore = 0;
int debug_prev_iteration_pc = 0;

volatile unsigned char c64d_vice_run_emulation = 1;
void c64d_shutdown_vice()
{
	c64d_vice_run_emulation = 0;
}

void maincpu_mainloop(void)
{
	LOGD("maincpu_mainloop");

	/* Notice that using a struct for these would make it a lot slower (at
	 least, on gcc 2.7.2.x).  */
	//	BYTE reg_a = 0;
	//	BYTE reg_x = 0;
	//	BYTE reg_y = 0;
	//	BYTE reg_p = 0;
	//	BYTE reg_sp = 0;
	//	BYTE flag_n = 0;
	//	BYTE flag_z = 0;
	//#ifndef NEED_REG_PC
	//	unsigned int reg_pc;
	//#endif
	//	BYTE *bank_base;
	//	int bank_start = 0;
	//	int bank_limit = 0;

	o_bank_base = &bank_base;
	o_bank_start = &bank_start;
	o_bank_limit = &bank_limit;
	
	machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
	
	while (c64d_vice_run_emulation) {
#define CLK maincpu_clk
#define RMW_FLAG maincpu_rmw_flag
#define LAST_OPCODE_INFO last_opcode_info
#define LAST_OPCODE_ADDR last_opcode_addr
#define TRACEFLG debug.maincpu_traceflg
		
#define CPU_INT_STATUS maincpu_int_status
		
#define ALARM_CONTEXT maincpu_alarm_context
		
#define CHECK_PENDING_ALARM() (clk >= next_alarm_clk(maincpu_int_status))
		
#define CHECK_PENDING_INTERRUPT() check_pending_interrupt(maincpu_int_status)
		
#define TRAP(addr) maincpu_int_status->trap_func(addr);
		
#define ROM_TRAP_HANDLER() traps_handler()
		
#define JAM()                                                         \
do {                                                              \
unsigned int tmp;                                             \
\
EXPORT_REGISTERS();                                           \
LOGError("CPU JAM: PC=%04x opcode=%x LAST_OPCODE_ADDR=%04x debug_iterations_after_restore=%d debug_prev_iteration_pc=%04x", reg_pc, opcode, LAST_OPCODE_ADDR, debug_iterations_after_restore, debug_prev_iteration_pc);									\
tmp = machine_jam("   " CPU_STR ": JAM at $%04X   ", reg_pc); \
switch (tmp) {                                                \
case JAM_RESET:                                           \
DO_INTERRUPT(IK_RESET);                               \
break;                                                \
case JAM_HARD_RESET:                                      \
mem_powerup();                                        \
DO_INTERRUPT(IK_RESET);                               \
break;                                                \
case JAM_MONITOR:                                         \
monitor_startup(e_comp_space);                        \
IMPORT_REGISTERS();                                   \
break;                                                \
default:                                                  \
CLK_INC();                                            \
}                                                             \
} while (0)
		
#define CALLER e_comp_space
		
#define ROM_TRAP_ALLOWED() mem_rom_trap_allowed((WORD)reg_pc)
		
#define GLOBAL_REGS maincpu_regs
		
//#include "6510dtvcore.c"
		
		/*
		 * 6510dtvcore.c - Cycle based 6510 emulation core.
		 *
		 * Written by
		 *  Ettore Perazzoli <ettore@comm2000.it>
		 *  Andreas Boose <viceteam@t-online.de>
		 *
		 * DTV sections written by
		 *  M.Kiesel <mayne@users.sourceforge.net>
		 *  Hannu Nuotio <hannu.nuotio@tut.fi>
		 *
		 * Cycle based rewrite by
		 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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
		
		/* This file is included by (some) CPU definition files */
		/* (mainc64cpu.c, mainviccpu.c) */
		
#define CPU_STR "Main CPU"
		
#include "traps.h"
		
#ifndef C64DTV
		/* The C64DTV can use different shadow registers for accu read/write. */
		/* For standard 6510, this is not the case. */
#define reg_a_write reg_a
#define reg_a_read  reg_a
#endif
		
		/* ------------------------------------------------------------------------- */
		
#define LOCAL_SET_NZ(val)        (flag_z = flag_n = (val))
		
#define LOCAL_SET_OVERFLOW(val)   \
do {                          \
if (val) {                \
reg_p |= P_OVERFLOW;  \
} else {                  \
reg_p &= ~P_OVERFLOW; \
}                         \
} while (0)
		
#define LOCAL_SET_BREAK(val)   \
do {                       \
if (val) {             \
reg_p |= P_BREAK;  \
} else {               \
reg_p &= ~P_BREAK; \
}                      \
} while (0)
		
#define LOCAL_SET_DECIMAL(val)   \
do {                         \
if (val) {               \
reg_p |= P_DECIMAL;  \
} else {                 \
reg_p &= ~P_DECIMAL; \
}                        \
} while (0)
		
#define LOCAL_SET_INTERRUPT(val)   \
do {                           \
if (val) {                 \
reg_p |= P_INTERRUPT;  \
} else {                   \
reg_p &= ~P_INTERRUPT; \
}                          \
} while (0)
		
#define LOCAL_SET_CARRY(val)   \
do {                       \
if (val) {             \
reg_p |= P_CARRY;  \
} else {               \
reg_p &= ~P_CARRY; \
}                      \
} while (0)
		
#define LOCAL_SET_SIGN(val)      (flag_n = (val) ? 0x80 : 0)
#define LOCAL_SET_ZERO(val)      (flag_z = !(val))
#define LOCAL_SET_STATUS(val)    (reg_p = ((val) & ~(P_ZERO | P_SIGN)), \
LOCAL_SET_ZERO((val) & P_ZERO),       \
flag_n = (val))
		
#define LOCAL_OVERFLOW()         (reg_p & P_OVERFLOW)
#define LOCAL_BREAK()            (reg_p & P_BREAK)
#define LOCAL_DECIMAL()          (reg_p & P_DECIMAL)
#define LOCAL_INTERRUPT()        (reg_p & P_INTERRUPT)
#define LOCAL_CARRY()            (reg_p & P_CARRY)
#define LOCAL_SIGN()             (flag_n & 0x80)
#define LOCAL_ZERO()             (!flag_z)
#define LOCAL_STATUS()           (reg_p | (flag_n & 0x80) | P_UNUSED    \
| (LOCAL_ZERO() ? P_ZERO : 0))
		
#ifdef LAST_OPCODE_INFO
		
		/* If requested, gather some info about the last executed opcode for timing
   purposes.  */
		
		/* Remember the number of the last opcode.  By default, the opcode does not
   delay interrupt and does not change the I flag.  */
#define SET_LAST_OPCODE(x) OPINFO_SET(LAST_OPCODE_INFO, (x), 0, 0, 0)
		
		/* Remember that the last opcode delayed a pending IRQ or NMI by one cycle.  */
#define OPCODE_DELAYS_INTERRUPT() OPINFO_SET_DELAYS_INTERRUPT(LAST_OPCODE_INFO, 1)
		
		/* Remember that the last opcode changed the I flag from 0 to 1, so we have
   to dispatch an IRQ even if the I flag is 0 when we check it.  */
#define OPCODE_DISABLES_IRQ() OPINFO_SET_DISABLES_IRQ(LAST_OPCODE_INFO, 1)
		
		/* Remember that the last opcode changed the I flag from 1 to 0, so we must
   not dispatch an IRQ even if the I flag is 1 when we check it.  */
#define OPCODE_ENABLES_IRQ() OPINFO_SET_ENABLES_IRQ(LAST_OPCODE_INFO, 1)
		
#else
		
		/* Info about the last opcode is not needed.  */
#define SET_LAST_OPCODE(x)
#define OPCODE_DELAYS_INTERRUPT()
#define OPCODE_DISABLES_IRQ()
#define OPCODE_ENABLES_IRQ()
		
#endif
		
#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif
		
#ifndef C64DTV
		/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()          \
do {                            \
GLOBAL_REGS.pc = reg_pc ;   \
GLOBAL_REGS.a = reg_a_read; \
GLOBAL_REGS.x = reg_x;      \
GLOBAL_REGS.y = reg_y;      \
GLOBAL_REGS.sp = reg_sp;    \
GLOBAL_REGS.p = reg_p;      \
GLOBAL_REGS.n = flag_n;     \
GLOBAL_REGS.z = flag_z;     \
} while (0)
		
		/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()                                 \
do {                                                   \
reg_a_write = GLOBAL_REGS.a;                       \
reg_x = GLOBAL_REGS.x;                             \
reg_y = GLOBAL_REGS.y;                             \
reg_sp = GLOBAL_REGS.sp;                           \
reg_p = GLOBAL_REGS.p;                             \
flag_n = GLOBAL_REGS.n;                            \
flag_z = GLOBAL_REGS.z;                            \
bank_start = bank_limit = 0; /* prevent caching */ \
JUMP(GLOBAL_REGS.pc);                              \
} while (0)
		
#else  /* C64DTV */
		
		/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()                                           \
do {                                                             \
GLOBAL_REGS.pc = reg_pc;                                     \
GLOBAL_REGS.a = dtv_registers[0];                            \
GLOBAL_REGS.x = dtv_registers[2];                            \
GLOBAL_REGS.y = dtv_registers[1];                            \
GLOBAL_REGS.sp = reg_sp;                                     \
GLOBAL_REGS.p = reg_p;                                       \
GLOBAL_REGS.n = flag_n;                                      \
GLOBAL_REGS.z = flag_z;                                      \
GLOBAL_REGS.r3 = dtv_registers[3];                           \
GLOBAL_REGS.r4 = dtv_registers[4];                           \
GLOBAL_REGS.r5 = dtv_registers[5];                           \
GLOBAL_REGS.r6 = dtv_registers[6];                           \
GLOBAL_REGS.r7 = dtv_registers[7];                           \
GLOBAL_REGS.r8 = dtv_registers[8];                           \
GLOBAL_REGS.r9 = dtv_registers[9];                           \
GLOBAL_REGS.r10 = dtv_registers[10];                         \
GLOBAL_REGS.r11 = dtv_registers[11];                         \
GLOBAL_REGS.r12 = dtv_registers[12];                         \
GLOBAL_REGS.r13 = dtv_registers[13];                         \
GLOBAL_REGS.r14 = dtv_registers[14];                         \
GLOBAL_REGS.r15 = dtv_registers[15];                         \
GLOBAL_REGS.acm = (reg_a_write_idx << 4) | (reg_a_read_idx); \
GLOBAL_REGS.yxm = (reg_y_idx << 4) | (reg_x_idx);            \
} while (0)
		
		/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()                                 \
do {                                                   \
dtv_registers[0] = GLOBAL_REGS.a;                  \
dtv_registers[2] = GLOBAL_REGS.x;                  \
dtv_registers[1] = GLOBAL_REGS.y;                  \
reg_sp = GLOBAL_REGS.sp;                           \
reg_p = GLOBAL_REGS.p;                             \
flag_n = GLOBAL_REGS.n;                            \
flag_z = GLOBAL_REGS.z;                            \
dtv_registers[3] = GLOBAL_REGS.r3;                 \
dtv_registers[4] = GLOBAL_REGS.r4;                 \
dtv_registers[5] = GLOBAL_REGS.r5;                 \
dtv_registers[6] = GLOBAL_REGS.r6;                 \
dtv_registers[7] = GLOBAL_REGS.r7;                 \
dtv_registers[8] = GLOBAL_REGS.r8;                 \
dtv_registers[9] = GLOBAL_REGS.r9;                 \
dtv_registers[10] = GLOBAL_REGS.r10;               \
dtv_registers[11] = GLOBAL_REGS.r11;               \
dtv_registers[12] = GLOBAL_REGS.r12;               \
dtv_registers[13] = GLOBAL_REGS.r13;               \
dtv_registers[14] = GLOBAL_REGS.r14;               \
dtv_registers[15] = GLOBAL_REGS.r15;               \
reg_a_write_idx = GLOBAL_REGS.acm >> 4;            \
reg_a_read_idx = GLOBAL_REGS.acm & 0xf;            \
reg_y_idx = GLOBAL_REGS.yxm >> 4;                  \
reg_x_idx = GLOBAL_REGS.yxm & 0xf;                 \
bank_start = bank_limit = 0; /* prevent caching */ \
JUMP(GLOBAL_REGS.pc);                              \
} while (0)
		
#endif /* C64DTV */
		
#ifdef VICE_DEBUG
#define TRACE_NMI()                         \
do {                                    \
if (TRACEFLG) {                     \
debug_nmi(CPU_INT_STATUS, CLK); \
}                                   \
} while (0)
		
#define TRACE_IRQ()                         \
do {                                    \
if (TRACEFLG) {                     \
debug_irq(CPU_INT_STATUS, CLK); \
}                                   \
} while (0)
		
#define TRACE_BRK()                \
do {                           \
if (TRACEFLG) {            \
debug_text("*** BRK"); \
}                          \
} while (0)
#else
#define TRACE_NMI()
#define TRACE_IRQ()
#define TRACE_BRK()
#endif
		
		/* Do the IRQ/BRK sequence, including NMI transformation. */
#define DO_IRQBRK()                                                                                                   \
do {                                                                                                              \
/* Interrupt vector to use. Assume regular IRQ/BRK. */                                                        \
WORD handler_vector = 0xfffe;                                                                                 \
\
PUSH(reg_pc >> 8);                                                                                            \
CLK_INC();                                                                                                    \
PUSH(reg_pc & 0xff);                                                                                          \
CLK_INC();                                                                                                    \
PUSH(LOCAL_STATUS());                                                                                         \
CLK_INC();                                                                                                    \
\
/* Process alarms up to this point to get nmi_clk updated. */                                                 \
while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {                                                \
alarm_context_dispatch(ALARM_CONTEXT, CLK);                                                               \
}                                                                                                             \
\
/* If an NMI would occur at this cycle... */                                                                  \
if ((CPU_INT_STATUS->global_pending_int & IK_NMI) && (CLK >= (CPU_INT_STATUS->nmi_clk + INTERRUPT_DELAY))) {  \
/* Transform the IRQ/BRK into an NMI. */                                                                  \
handler_vector = 0xfffa;                                                                                  \
TRACE_NMI();                                                                                              \
if (monitor_mask[CALLER] & (MI_STEP)) {                                                                   \
monitor_check_icount_interrupt();                                                                     \
}                                                                                                         \
interrupt_ack_nmi(CPU_INT_STATUS);                                                                        \
}                                                                                                             \
\
LOCAL_SET_INTERRUPT(1);                                                                                       \
addr = LOAD(handler_vector);                                                                                  \
CLK_INC();                                                                                                    \
addr |= (LOAD(handler_vector + 1) << 8);                                                                      \
CLK_INC();                                                                                                    \
JUMP(addr);                                                                                                   \
} while (0)
		
		/* Perform the interrupts in `int_kind'.  If we have both NMI and IRQ,
   execute NMI.  */
		/* FIXME: LOCAL_STATUS() should check byte ready first.  */
#define DO_INTERRUPT(int_kind)                                                 \
	do {                                                                       \
		BYTE ik = (int_kind);                                                  \
		WORD addr;                                                             \
		\
		if (ik & (IK_IRQ | IK_IRQPEND | IK_NMI)) {                             \
			if ((ik & IK_NMI)                                                  \
				&& interrupt_check_nmi_delay(CPU_INT_STATUS, CLK)) {           \
				TRACE_NMI();                                                   \
				if (monitor_mask[CALLER] & (MI_STEP)) {                        \
					monitor_check_icount_interrupt();                          \
				}                                                              \
				interrupt_ack_nmi(CPU_INT_STATUS);                             \
				if (!SKIP_CYCLE) {                                             \
					LOAD(reg_pc);                                              \
					CLK_INC();                                                 \
					LOAD(reg_pc);                                              \
					CLK_INC();                                                 \
				}                                                              \
				LOCAL_SET_BREAK(0);                                            \
				PUSH(reg_pc >> 8);                                             \
				CLK_INC();                                                     \
				PUSH(reg_pc & 0xff);                                           \
				CLK_INC();                                                     \
				PUSH(LOCAL_STATUS());                                          \
				CLK_INC();                                                     \
				addr = LOAD(0xfffa);                                           \
				CLK_INC();                                                     \
				addr |= (LOAD(0xfffb) << 8);                                   \
				CLK_INC();                                                     \
				LOCAL_SET_INTERRUPT(1);                                        \
				JUMP(addr);                                                    \
				SET_LAST_OPCODE(0);                                            \
			} else if ((ik & (IK_IRQ | IK_IRQPEND))                            \
					&& (!LOCAL_INTERRUPT()                                    \
						|| OPINFO_DISABLES_IRQ(LAST_OPCODE_INFO))             \
					&& interrupt_check_irq_delay(CPU_INT_STATUS, CLK)) {      \
				TRACE_IRQ();                                                   \
				if (monitor_mask[CALLER] & (MI_STEP)) {                        \
					monitor_check_icount_interrupt();                          \
				}                                                              \
				interrupt_ack_irq(CPU_INT_STATUS);                             \
				if (!SKIP_CYCLE) {                                             \
					LOAD(reg_pc);                                              \
					CLK_INC();                                                 \
					LOAD(reg_pc);                                              \
					CLK_INC();                                                 \
				}                                                              \
				LOCAL_SET_BREAK(0);                                            \
				DO_IRQBRK();                                                   \
				SET_LAST_OPCODE(0);                                            \
			}                                                                  \
		}                                                                      \
		if (ik & (IK_TRAP | IK_RESET)) {                                       \
			if (ik & IK_TRAP) {                                                \
				EXPORT_REGISTERS();                                            \
				interrupt_do_trap(CPU_INT_STATUS, (WORD)reg_pc);               \
				IMPORT_REGISTERS();                                            \
				if (CPU_INT_STATUS->global_pending_int & IK_RESET) {           \
					ik |= IK_RESET;                                            \
				}                                                              \
			}                                                                  \
			if (ik & IK_RESET) {                                               \
				interrupt_ack_reset(CPU_INT_STATUS);                           \
				cpu_reset();                                                   \
				addr = LOAD(0xfffc);                                           \
				addr |= (LOAD(0xfffd) << 8);                                   \
				bank_start = bank_limit = 0; /* prevent caching */             \
				JUMP(addr);                                                    \
				DMA_ON_RESET;                                                  \
			}                                                                  \
		}                                                                      \
		if (ik & (IK_MONITOR | IK_DMA)) {                                      \
			if (ik & IK_MONITOR) {                                             \
				if (monitor_force_import(CALLER)) {                            \
					IMPORT_REGISTERS();                                        \
				}                                                              \
				if (monitor_mask[CALLER]) {                                    \
					EXPORT_REGISTERS();                                        \
				}                                                              \
				if (monitor_mask[CALLER] & (MI_STEP)) {                        \
					monitor_check_icount((WORD)reg_pc);                        \
					IMPORT_REGISTERS();                                        \
				}                                                              \
				if (monitor_mask[CALLER] & (MI_BREAK)) {                       \
					if (monitor_check_breakpoints(CALLER, (WORD)reg_pc)) {     \
						monitor_startup(CALLER);                               \
						IMPORT_REGISTERS();                                    \
					}                                                          \
				}                                                              \
				if (monitor_mask[CALLER] & (MI_WATCH)) {                       \
					monitor_check_watchpoints(LAST_OPCODE_ADDR, (WORD)reg_pc); \
					IMPORT_REGISTERS();                                        \
				}                                                              \
			}                                                                  \
			if (ik & IK_DMA) {                                                 \
				EXPORT_REGISTERS();                                            \
				DMA_FUNC;                                                      \
				interrupt_ack_dma(CPU_INT_STATUS);                             \
				IMPORT_REGISTERS();                                            \
			}                                                                  \
		}                                                                      \
} while (0)
		
		/* ------------------------------------------------------------------------- */
		
		/* Addressing modes.  For convenience, page boundary crossing cycles and
   ``idle'' memory reads are handled here as well. */
		
#define GET_TEMP(dest) dest = new_value;
		
#define GET_IMM(dest) dest = (BYTE)(p1);
		/* same as above, for NOOP */
#define GET_IMM_DUMMY()
		
#define GET_ABS(dest)        \
dest = (BYTE)(LOAD(p2)); \
CLK_INC();
		
		/* same as above, for NOOP */
#define GET_ABS_DUMMY()      \
LOAD(p2);                \
CLK_INC();
		
#define SET_ABS(value) \
STORE(p2, value);  \
CLK_INC();
		
#define SET_ABS_RMW(old_value, new_value) \
if (!SKIP_CYCLE) {                    \
STORE(p2, old_value);             \
CLK_INC();                        \
}                                     \
STORE(p2, new_value);                 \
CLK_INC();
		
#define INT_ABS_I_R(reg_i)                                 \
if (!SKIP_CYCLE && ((((p2) & 0xff) + reg_i) > 0xff)) { \
LOAD((((p2) + reg_i) & 0xff) | ((p2) & 0xff00));   \
CLK_INC();                                         \
}
		
#define INT_ABS_I_W(reg_i)                               \
if (!SKIP_CYCLE) {                                   \
LOAD((((p2) + reg_i) & 0xff) | ((p2) & 0xff00)); \
CLK_INC();                                       \
}
		
#define GET_ABS_X(dest)        \
INT_ABS_I_R(reg_x)         \
dest = LOAD((p2) + reg_x); \
CLK_INC();
		/* same as above, for NOOP */
#define GET_ABS_X_DUMMY()      \
INT_ABS_I_R(reg_x)         \
LOAD((p2) + reg_x);        \
CLK_INC();
		
#define GET_ABS_Y(dest)        \
INT_ABS_I_R(reg_y)         \
dest = LOAD((p2) + reg_y); \
CLK_INC();
		
#define GET_ABS_X_RMW(dest)    \
INT_ABS_I_W(reg_x)         \
dest = LOAD((p2) + reg_x); \
CLK_INC();
		
#define GET_ABS_Y_RMW(dest)    \
INT_ABS_I_W(reg_y)         \
dest = LOAD((p2) + reg_y); \
CLK_INC();
		
#define SET_ABS_X(value)      \
INT_ABS_I_W(reg_x)        \
STORE(p2 + reg_x, value); \
CLK_INC();
		
#define SET_ABS_Y(value)      \
INT_ABS_I_W(reg_y)        \
STORE(p2 + reg_y, value); \
CLK_INC();
		
#define SET_ABS_I_RMW(reg_i, old_value, new_value) \
if (!SKIP_CYCLE) {                             \
STORE(p2 + reg_i, old_value);              \
CLK_INC();                                 \
}                                              \
STORE(p2 + reg_i, new_value);                  \
CLK_INC();
		
#define SET_ABS_X_RMW(old_value, new_value) SET_ABS_I_RMW(reg_x, old_value, new_value)
		
#define SET_ABS_Y_RMW(old_value, new_value) SET_ABS_I_RMW(reg_y, old_value, new_value)
		
#define GET_ZERO(dest)    \
dest = LOAD_ZERO(p1); \
CLK_INC();
		
		/* same as above, for NOOP */
#define GET_ZERO_DUMMY()    \
LOAD_ZERO(p1); \
CLK_INC();
		
#define SET_ZERO(value)    \
STORE_ZERO(p1, value); \
CLK_INC();
		
#define SET_ZERO_RMW(old_value, new_value) \
if (!SKIP_CYCLE) {                     \
STORE_ZERO(p1, old_value);         \
CLK_INC();                         \
}                                      \
STORE_ZERO(p1, new_value);             \
CLK_INC();
		
#define INT_ZERO_I      \
if (!SKIP_CYCLE) {  \
LOAD_ZERO(p1);  \
CLK_INC();      \
}
		
#define GET_ZERO_X(dest)          \
INT_ZERO_I                    \
dest = LOAD_ZERO(p1 + reg_x); \
CLK_INC();
		/* same as above, for NOOP */
#define GET_ZERO_X_DUMMY()        \
INT_ZERO_I                    \
LOAD_ZERO(p1 + reg_x);        \
CLK_INC();
		
#define GET_ZERO_Y(dest)          \
INT_ZERO_I                    \
dest = LOAD_ZERO(p1 + reg_y); \
CLK_INC();
		
#define SET_ZERO_X(value)          \
INT_ZERO_I                     \
STORE_ZERO(p1 + reg_x, value); \
CLK_INC();
		
#define SET_ZERO_Y(value)          \
INT_ZERO_I                     \
STORE_ZERO(p1 + reg_y, value); \
CLK_INC();
		
#define SET_ZERO_I_RMW(reg_i, old_value, new_value) \
if (!SKIP_CYCLE) {                              \
STORE_ZERO(p1 + reg_i, old_value);          \
CLK_INC();                                  \
}                                               \
STORE_ZERO(p1 + reg_i, new_value);              \
CLK_INC();
		
#define SET_ZERO_X_RMW(old_value, new_value) SET_ZERO_I_RMW(reg_x, old_value, new_value)
		
#define SET_ZERO_Y_RMW(old_value, new_value) SET_ZERO_I_RMW(reg_y, old_value, new_value)
		
#define INT_IND_X                   \
unsigned int tmpa, addr;        \
LOAD_ZERO(p1);                  \
CLK_INC();                      \
tmpa = (p1 + reg_x) & 0xff;     \
addr = LOAD_ZERO(tmpa);         \
CLK_INC();                      \
tmpa = (tmpa + 1) & 0xff;       \
addr |= (LOAD_ZERO(tmpa) << 8); \
CLK_INC();
		
#define GET_IND_X(dest) \
INT_IND_X           \
dest = LOAD(addr);  \
CLK_INC();
		
#define SET_IND_X(value)    \
{                       \
INT_IND_X           \
STORE(addr, value); \
CLK_INC();          \
}
		
#define INT_IND_Y_R()                                        \
unsigned int tmpa, addr;                                 \
tmpa = LOAD_ZERO(p1);                                    \
CLK_INC();                                               \
tmpa |= (LOAD_ZERO(p1 + 1) << 8);                        \
CLK_INC();                                               \
if (!SKIP_CYCLE && ((((tmpa) & 0xff) + reg_y) > 0xff)) { \
LOAD((tmpa & 0xff00) | ((tmpa + reg_y) & 0xff));     \
CLK_INC();                                           \
}                                                        \
addr = (tmpa + reg_y) & 0xffff;                          \

#define INT_IND_Y_W()                                    \
unsigned int tmpa, addr;                             \
tmpa = LOAD_ZERO(p1);                                \
CLK_INC();                                           \
tmpa |= (LOAD_ZERO(p1 + 1) << 8);                    \
CLK_INC();                                           \
if (!SKIP_CYCLE) {                                   \
LOAD((tmpa & 0xff00) | ((tmpa + reg_y) & 0xff)); \
CLK_INC();                                       \
}                                                    \
addr = (tmpa + reg_y) & 0xffff;
		/* like above, for SHA_IND_Y */
#define INT_IND_Y_W_NOADDR()                                          \
unsigned int tmpa;                                                \
tmpa = LOAD_ZERO(p1);                                             \
CLK_INC();                                                        \
tmpa |= (LOAD_ZERO(p1 + 1) << 8);                                 \
CLK_INC();                                                        \
if (!SKIP_CYCLE) {                                                \
LOAD_CHECK_BA_LOW((tmpa & 0xff00) | ((tmpa + reg_y) & 0xff)); \
CLK_INC();                                                    \
}
		
#define GET_IND_Y(dest) \
INT_IND_Y_R()       \
dest = LOAD(addr);  \
CLK_INC();
		
#define GET_IND_Y_RMW(dest) \
INT_IND_Y_W()           \
dest = LOAD(addr);      \
CLK_INC();
		
#define SET_IND_Y(value)    \
{                       \
INT_IND_Y_W()       \
STORE(addr, value); \
CLK_INC();          \
}
		
#define SET_IND_RMW(old_value, new_value) \
if (!SKIP_CYCLE) {                    \
STORE(addr, old_value);           \
CLK_INC();                        \
}                                     \
STORE(addr, new_value);               \
CLK_INC();
		
#define SET_ABS_SH_I(addr, reg_and, reg_i)                      \
do {                                                        \
unsigned int tmp2, tmp3, value;                         \
\
tmp3 = reg_and & (((addr) >> 8) + 1);                   \
/* Set by main cpu to signal steal after above fetch */ \
if (OPINFO_ENABLES_IRQ(LAST_OPCODE_INFO)) {             \
/* Remove the signal */                             \
LAST_OPCODE_INFO &= ~OPINFO_ENABLES_IRQ_MSK;        \
/* Value is not ANDed */                            \
value = reg_and;                                    \
} else {                                                \
value = tmp3;                                       \
}                                                       \
tmp2 = addr + reg_i;                                    \
if ((((addr) & 0xff) + reg_i) > 0xff) {                 \
tmp2 = (tmp2 & 0xff) | ((tmp3) << 8);               \
}                                                       \
STORE(tmp2, (value));                                   \
CLK_INC();                                              \
} while (0)
		
#define INC_PC(value) (reg_pc += (value))
		
		/* ------------------------------------------------------------------------- */
		
		/* Opcodes.  */
		
		/*
   A couple of caveats about PC:
		 
   - the VIC-II emulation requires PC to be incremented before the first
		 write access (this is not (very) important when writing to the zero
		 page);
		 
   - `p0', `p1' and `p2' can only be used *before* incrementing PC: some
		 machines (eg. the C128) might depend on this.
		 */
		
#define ADC(get_func, pc_inc)                                                                      \
do {                                                                                           \
unsigned int tmp_value;                                                                    \
unsigned int tmp;                                                                          \
\
get_func(tmp_value);                                                                       \
\
if (LOCAL_DECIMAL()) {                                                                     \
tmp = (reg_a_read & 0xf) + (tmp_value & 0xf) + (reg_p & 0x1);                          \
if (tmp > 0x9) {                                                                       \
tmp += 0x6;                                                                        \
}                                                                                      \
if (tmp <= 0x0f) {                                                                     \
tmp = (tmp & 0xf) + (reg_a_read & 0xf0) + (tmp_value & 0xf0);                      \
} else {                                                                               \
tmp = (tmp & 0xf) + (reg_a_read & 0xf0) + (tmp_value & 0xf0) + 0x10;               \
}                                                                                      \
LOCAL_SET_ZERO(!((reg_a_read + tmp_value + (reg_p & 0x1)) & 0xff));                    \
LOCAL_SET_SIGN(tmp & 0x80);                                                            \
LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80) && !((reg_a_read ^ tmp_value) & 0x80)); \
if ((tmp & 0x1f0) > 0x90) {                                                            \
tmp += 0x60;                                                                       \
}                                                                                      \
LOCAL_SET_CARRY((tmp & 0xff0) > 0xf0);                                                 \
} else {                                                                                   \
tmp = tmp_value + reg_a_read + (reg_p & P_CARRY);                                      \
LOCAL_SET_NZ(tmp & 0xff);                                                              \
LOCAL_SET_OVERFLOW(!((reg_a_read ^ tmp_value) & 0x80) && ((reg_a_read ^ tmp) & 0x80)); \
LOCAL_SET_CARRY(tmp > 0xff);                                                           \
}                                                                                          \
reg_a_write = tmp;                                                                         \
INC_PC(pc_inc);                                                                            \
} while (0)
		
#define ANC()                                  \
do {                                       \
reg_a_write = (BYTE)(reg_a_read & p1); \
LOCAL_SET_NZ(reg_a_read);              \
LOCAL_SET_CARRY(LOCAL_SIGN());         \
INC_PC(2);                             \
} while (0)
		
#define AND(get_func, pc_inc)                     \
do {                                          \
unsigned int value;                       \
get_func(value)                           \
reg_a_write = (BYTE)(reg_a_read & value); \
LOCAL_SET_NZ(reg_a_read);                 \
INC_PC(pc_inc);                           \
} while (0)
		
		/*
		 The result of the ANE opcode is A = ((A | CONST) & X & IMM), with CONST apparently
		 being both chip- and temperature dependent.
		 
		 The commonly used value for CONST in various documents is 0xee, which is however
		 not to be taken for granted (as it is unstable). see here:
		 http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)
		 
		 as seen in the list, there are several possible values, and its origin is still
		 kinda unknown. instead of the commonly used 0xee we use 0xff here, since this
		 will make the only known occurance of this opcode in actual code work. see here:
		 https://sourceforge.net/tracker/?func=detail&aid=2110948&group_id=223021&atid=1057617
		 
		 FIXME: in the unlikely event that other code surfaces that depends on another
		 CONST value, it probably has to be made configureable somehow if no value can
		 be found that works for both.
		 */
		
#define ANE()                                                       \
do {                                                            \
/* Set by main-cpu to signal steal after first fetch */     \
if (OPINFO_ENABLES_IRQ(LAST_OPCODE_INFO)) {                 \
/* Remove the signal */                                 \
LAST_OPCODE_INFO &= ~OPINFO_ENABLES_IRQ_MSK;            \
/* TODO emulate the different behaviour */              \
reg_a_write = (BYTE)((reg_a_read | 0xff) & reg_x & p1); \
} else {                                                    \
reg_a_write = (BYTE)((reg_a_read | 0xff) & reg_x & p1); \
}                                                           \
LOCAL_SET_NZ(reg_a_read);                                   \
INC_PC(2);                                                  \
/* Pretend to be NOP #$nn to not trigger the special case   \
when cycles are stolen after the second fetch */         \
SET_LAST_OPCODE(0x80);                                      \
} while (0)
		
		/* The fanciest opcode ever... ARR! */
#define ARR()                                                       \
do {                                                            \
unsigned int tmp;                                           \
\
tmp = reg_a_read & (p1);                                    \
if (LOCAL_DECIMAL()) {                                      \
int tmp_2 = tmp;                                        \
tmp_2 |= (reg_p & P_CARRY) << 8;                        \
tmp_2 >>= 1;                                            \
LOCAL_SET_SIGN(LOCAL_CARRY());                          \
LOCAL_SET_ZERO(!tmp_2);                                 \
LOCAL_SET_OVERFLOW((tmp_2 ^ tmp) & 0x40);               \
if (((tmp & 0xf) + (tmp & 0x1)) > 0x5) {                \
tmp_2 = (tmp_2 & 0xf0) | ((tmp_2 + 0x6) & 0xf);     \
}                                                       \
if (((tmp & 0xf0) + (tmp & 0x10)) > 0x50) {             \
tmp_2 = (tmp_2 & 0x0f) | ((tmp_2 + 0x60) & 0xf0);   \
LOCAL_SET_CARRY(1);                                 \
} else {                                                \
LOCAL_SET_CARRY(0);                                 \
}                                                       \
reg_a_write = tmp_2;                                    \
} else {                                                    \
tmp |= (reg_p & P_CARRY) << 8;                          \
tmp >>= 1;                                              \
LOCAL_SET_NZ(tmp);                                      \
LOCAL_SET_CARRY(tmp & 0x40);                            \
LOCAL_SET_OVERFLOW((tmp & 0x40) ^ ((tmp & 0x20) << 1)); \
reg_a_write = tmp;                                      \
}                                                           \
INC_PC(2);                                                  \
} while (0)
		
#define ASL(pc_inc, get_func, set_func)      \
do {                                     \
unsigned int old_value, new_value;   \
get_func(old_value)                  \
LOCAL_SET_CARRY(old_value & 0x80);   \
new_value = (old_value << 1) & 0xff; \
LOCAL_SET_NZ(new_value);             \
INC_PC(pc_inc);                      \
set_func(old_value, new_value);      \
} while (0)
		
#define ASL_A()                             \
do {                                    \
LOCAL_SET_CARRY(reg_a_read & 0x80); \
reg_a_write = reg_a_read << 1;      \
LOCAL_SET_NZ(reg_a_read);           \
INC_PC(1);                          \
} while (0)
		
#define ASR()                        \
do {                             \
unsigned int tmp;            \
\
tmp = reg_a_read & (p1);     \
LOCAL_SET_CARRY(tmp & 0x01); \
reg_a_write = tmp >> 1;      \
LOCAL_SET_NZ(reg_a_read);    \
INC_PC(2);                   \
} while (0)
		
#define BIT(get_func, pc_inc)                \
do {                                     \
unsigned int tmp;                    \
get_func(tmp)                        \
LOCAL_SET_SIGN(tmp & 0x80);          \
LOCAL_SET_OVERFLOW(tmp & 0x40);      \
LOCAL_SET_ZERO(!(tmp & reg_a_read)); \
INC_PC(pc_inc);                      \
} while (0)
		
#ifdef C64DTV
		
#define BRANCH(cond)                                              \
do {                                                          \
INC_PC(2);                                                \
\
if (cond) {                                               \
unsigned int dest_addr;                               \
\
burst_broken = 1;                                     \
dest_addr = reg_pc + (signed char)(p1);               \
\
if (!SKIP_CYCLE) {                                    \
LOAD(reg_pc);                                     \
CLK_INC();                                        \
if ((reg_pc ^ dest_addr) & 0xff00) {              \
LOAD((reg_pc & 0xff00) | (dest_addr & 0xff)); \
CLK_INC();                                    \
} else {                                          \
OPCODE_DELAYS_INTERRUPT();                    \
}                                                 \
}                                                     \
JUMP(dest_addr & 0xffff);                             \
}                                                         \
} while (0)
		
#else /* !C64DTV */
		
#define BRANCH(cond)                                          \
do {                                                      \
INC_PC(2);                                            \
\
if (cond) {                                           \
unsigned int dest_addr;                           \
\
dest_addr = reg_pc + (signed char)(p1);           \
\
LOAD(reg_pc);                                     \
CLK_INC();                                        \
if ((reg_pc ^ dest_addr) & 0xff00) {              \
LOAD((reg_pc & 0xff00) | (dest_addr & 0xff)); \
CLK_INC();                                    \
} else {                                          \
OPCODE_DELAYS_INTERRUPT();                    \
}                                                 \
JUMP(dest_addr & 0xffff);                         \
}                                                     \
} while (0)
		
#endif
		
#define BRK() \
do { \
WORD addr;          \
EXPORT_REGISTERS(); \
TRACE_BRK();        \
INC_PC(2);          \
LOCAL_SET_BREAK(1); \
DO_IRQBRK();        \
} while (0)
		
		/* The JAM (0x02) opcode is also used to patch the ROM.  The function trap_handler()
   returns nonzero if this is not a patch, but a `real' JAM instruction. */
		
#define JAM_02()                                                                      \
do {                                                                              \
DWORD trap_result;                                                            \
EXPORT_REGISTERS();                                                           \
if (!ROM_TRAP_ALLOWED() || (trap_result = ROM_TRAP_HANDLER()) == (DWORD)-1) { \
REWIND_FETCH_OPCODE(CLK);                                                 \
JAM();                                                                    \
} else {                                                                      \
if (trap_result) {                                                        \
REWIND_FETCH_OPCODE(CLK);                                             \
SET_OPCODE(trap_result);                                              \
IMPORT_REGISTERS();                                                   \
goto trap_skipped;                                                    \
} else {                                                                  \
IMPORT_REGISTERS();                                                   \
}                                                                         \
}                                                                             \
} while (0)
		
#define CLC()               \
do {                    \
INC_PC(1);          \
LOCAL_SET_CARRY(0); \
} while (0)
		
#define CLD()                 \
do {                      \
INC_PC(1);            \
LOCAL_SET_DECIMAL(0); \
} while (0)
		
#ifdef OPCODE_UPDATE_IN_FETCH
		
#define CLI()                                             \
do {                                                  \
INC_PC(1);                                        \
/* Set by main-cpu to signal steal during CLI */  \
if (!OPINFO_ENABLES_IRQ(LAST_OPCODE_INFO)) {      \
if (LOCAL_INTERRUPT()) {                      \
OPCODE_ENABLES_IRQ();                     \
}                                             \
} else {                                          \
/* Remove the signal and the related delay */ \
LAST_OPCODE_INFO &= ~OPINFO_ENABLES_IRQ_MSK;  \
}                                                 \
LOCAL_SET_INTERRUPT(0);                           \
} while (0)
		
#else /* !OPCODE_UPDATE_IN_FETCH */
		
#define CLI()                     \
do {                          \
INC_PC(1);                \
if (LOCAL_INTERRUPT()) {  \
OPCODE_ENABLES_IRQ(); \
}                         \
LOCAL_SET_INTERRUPT(0);   \
} while (0)
		
#endif
		
#define CLV()                  \
do {                       \
INC_PC(1);             \
LOCAL_SET_OVERFLOW(0); \
} while (0)
		
#define CP(reg, get_func, pc_inc)     \
do {                              \
unsigned int tmp;             \
BYTE value;                   \
get_func(value)               \
tmp = reg - value;            \
LOCAL_SET_CARRY(tmp < 0x100); \
LOCAL_SET_NZ(tmp & 0xff);     \
INC_PC(pc_inc);               \
} while (0)
		
#define DCP(pc_inc, get_func, set_func)           \
do {                                          \
unsigned int old_value, new_value;        \
get_func(old_value)                       \
new_value = (old_value - 1) & 0xff;       \
LOCAL_SET_CARRY(reg_a_read >= new_value); \
LOCAL_SET_NZ((reg_a_read - new_value));   \
INC_PC(pc_inc);                           \
set_func(old_value, new_value)            \
} while (0)
		
#define DEC(pc_inc, get_func, set_func)     \
do {                                    \
unsigned int old_value, new_value;  \
get_func(old_value)                 \
new_value = (old_value - 1) & 0xff; \
LOCAL_SET_NZ(new_value);            \
INC_PC(pc_inc);                     \
set_func(old_value, new_value)      \
} while (0)
		
#define DEX()                \
do {                     \
reg_x--;             \
LOCAL_SET_NZ(reg_x); \
INC_PC(1);           \
} while (0)
		
#define DEY()                \
do {                     \
reg_y--;             \
LOCAL_SET_NZ(reg_y); \
INC_PC(1);           \
} while (0)
		
#define EOR(get_func, pc_inc)                       \
do {                                            \
unsigned int value;                         \
get_func(value)                             \
reg_a_write = (BYTE)(reg_a_read ^ (value)); \
LOCAL_SET_NZ(reg_a_read);                   \
INC_PC(pc_inc);                             \
} while (0)
		
#define INC(pc_inc, get_func, set_func)     \
do {                                    \
unsigned int old_value, new_value;  \
get_func(old_value)                 \
new_value = (old_value + 1) & 0xff; \
LOCAL_SET_NZ(new_value);            \
INC_PC(pc_inc);                     \
set_func(old_value, new_value)      \
} while (0)
		
#define INX()                \
do {                     \
reg_x++;             \
LOCAL_SET_NZ(reg_x); \
INC_PC(1);           \
} while (0)
		
#define INY()                \
do {                     \
reg_y++;             \
LOCAL_SET_NZ(reg_y); \
INC_PC(1);           \
} while (0)
		
#define ISB(pc_inc, get_func, set_func)     \
do {                                    \
unsigned int old_value, new_value;  \
get_func(old_value)                 \
new_value = (old_value + 1) & 0xff; \
SBC(GET_TEMP, 0);                   \
INC_PC(pc_inc);                     \
set_func(old_value, new_value)      \
} while (0)
		
#define JMP(addr)   \
do {            \
JUMP(addr); \
} while (0)
		
#define JMP_IND()                                                    \
do {                                                             \
WORD dest_addr;                                              \
dest_addr = LOAD(p2);                                        \
CLK_INC();                                                   \
dest_addr |= (LOAD((p2 & 0xff00) | ((p2 + 1) & 0xff)) << 8); \
CLK_INC();                                                   \
JUMP(dest_addr);                                             \
} while (0)
		
		/* HACK: fix JSR MSB in monitor CPU history */
#ifdef FEATURE_CPUMEMHISTORY
#define JSR_FIXUP_MSB(x)    monitor_cpuhistory_fix_p2(x)
#else
#define JSR_FIXUP_MSB(x)
#endif
		
#define JSR()                                     \
do {                                          \
BYTE addr_msb;                            \
WORD dest_addr;                           \
if (!SKIP_CYCLE) {                        \
STACK_PEEK();                         \
CLK_INC();                            \
}                                         \
INC_PC(2);                                \
PUSH(((reg_pc) >> 8) & 0xff);             \
CLK_INC();                                \
PUSH((reg_pc) & 0xff);                    \
CLK_INC();                                \
addr_msb = LOAD(reg_pc);                  \
JSR_FIXUP_MSB(addr_msb);                  \
dest_addr = (WORD)(p1 | (addr_msb << 8)); \
CLK_INC();                                \
c64d_profiler_jsr(dest_addr);								\
JUMP(dest_addr);                          \
} while (0)
		
#define LAS()                                          \
do {                                               \
unsigned int value;                            \
GET_ABS_Y(value)                               \
reg_a_write = reg_x = reg_sp = reg_sp & value; \
LOCAL_SET_NZ(reg_a_read);                      \
INC_PC(3);                                     \
} while (0)
		
#define LAX(get_func, pc_inc)     \
do {                          \
get_func(reg_x);          \
reg_a_write = reg_x;      \
LOCAL_SET_NZ(reg_a_read); \
INC_PC(pc_inc);           \
} while (0)
		
#define LD(dest, get_func, pc_inc) \
do {                           \
get_func(dest);            \
LOCAL_SET_NZ(dest);        \
INC_PC(pc_inc);            \
} while (0)
		
#define LSR(pc_inc, get_func, set_func)    \
do {                                   \
unsigned int old_value, new_value; \
get_func(old_value)                \
LOCAL_SET_CARRY(old_value & 0x01); \
new_value = old_value >> 1;        \
LOCAL_SET_NZ(new_value);           \
INC_PC(pc_inc);                    \
set_func(old_value, new_value)     \
} while (0)
		
#define LSR_A()                             \
do {                                    \
LOCAL_SET_CARRY(reg_a_read & 0x01); \
reg_a_write = reg_a_read >> 1;      \
LOCAL_SET_NZ(reg_a_read);           \
INC_PC(1);                          \
} while (0)
		
		/* Note: this is not always exact, as this opcode can be quite unstable!
   Moreover, the behavior is different from the one described in 64doc. */
#define LXA(value, pc_inc)                                             \
do {                                                               \
reg_a_write = reg_x = ((reg_a_read | 0xee) & ((BYTE)(value))); \
LOCAL_SET_NZ(reg_a_read);                                      \
INC_PC(pc_inc);                                                \
} while (0)
		
#define ORA(get_func, pc_inc)                       \
do {                                            \
unsigned int value;                         \
get_func(value)                             \
reg_a_write = (BYTE)(reg_a_read | (value)); \
LOCAL_SET_NZ(reg_a_write);                  \
INC_PC(pc_inc);                             \
} while (0)
		
#define NOOP(get_func, pc_inc) \
do {                       \
get_func()             \
INC_PC(pc_inc);        \
} while (0)
		
#define PHA()             \
do {                  \
PUSH(reg_a_read); \
CLK_INC();        \
INC_PC(1);        \
} while (0)
		
#define PHP()                           \
do {                                \
PUSH(LOCAL_STATUS() | P_BREAK); \
CLK_INC();                      \
INC_PC(1);                      \
} while (0)
		
#define PLA()                     \
do {                          \
if (!SKIP_CYCLE) {        \
STACK_PEEK();         \
CLK_INC();            \
}                         \
reg_a_write = PULL();     \
CLK_INC();                \
LOCAL_SET_NZ(reg_a_read); \
INC_PC(1);                \
} while (0)
		
#define PLP()                                                 \
do {                                                      \
BYTE s;                                               \
if (!SKIP_CYCLE) {                                    \
STACK_PEEK();                                     \
CLK_INC();                                        \
}                                                     \
s = PULL();                                           \
CLK_INC();                                            \
if (!(s & P_INTERRUPT) && LOCAL_INTERRUPT()) {        \
OPCODE_ENABLES_IRQ();                             \
} else if ((s & P_INTERRUPT) && !LOCAL_INTERRUPT()) { \
OPCODE_DISABLES_IRQ();                            \
}                                                     \
LOCAL_SET_STATUS(s);                                  \
INC_PC(1);                                            \
} while (0)
		
#define RLA(pc_inc, get_func, set_func)                   \
do {                                                  \
unsigned int old_value, new_value;                \
get_func(old_value)                               \
new_value = (old_value << 1) | (reg_p & P_CARRY); \
LOCAL_SET_CARRY(new_value & 0x100);               \
reg_a_write = reg_a_read & new_value;             \
LOCAL_SET_NZ(reg_a_read);                         \
INC_PC(pc_inc);                                   \
set_func(old_value, new_value);                   \
} while (0)
		
#define ROL(pc_inc, get_func, set_func)                   \
do {                                                  \
unsigned int old_value, new_value;                \
get_func(old_value)                               \
new_value = (old_value << 1) | (reg_p & P_CARRY); \
LOCAL_SET_CARRY(new_value & 0x100);               \
LOCAL_SET_NZ(new_value & 0xff);                   \
INC_PC(pc_inc);                                   \
set_func(old_value, new_value);                   \
} while (0)
		
#define ROL_A()                                \
do {                                       \
unsigned int tmp = reg_a_read << 1;    \
\
reg_a_write = tmp | (reg_p & P_CARRY); \
LOCAL_SET_CARRY(tmp & 0x100);          \
LOCAL_SET_NZ(reg_a_read);              \
INC_PC(1);                             \
} while (0)
		
#define ROR(pc_inc, get_func, set_func)    \
do {                                   \
unsigned int old_value, new_value; \
get_func(old_value)                \
new_value = old_value;             \
if (reg_p & P_CARRY) {             \
new_value |= 0x100;            \
}                                  \
LOCAL_SET_CARRY(new_value & 0x01); \
new_value >>= 1;                   \
LOCAL_SET_NZ(new_value);           \
INC_PC(pc_inc);                    \
set_func(old_value, new_value)     \
} while (0)
		
#define ROR_A()                                         \
do {                                                \
BYTE tmp = reg_a_read;                          \
\
reg_a_write = (reg_a_read >> 1) | (reg_p << 7); \
LOCAL_SET_CARRY(tmp & 0x01);                    \
LOCAL_SET_NZ(reg_a_read);                       \
INC_PC(1);                                      \
} while (0)
		
#define RRA(pc_inc, get_func, set_func)    \
do {                                   \
unsigned int old_value, new_value; \
get_func(old_value)                \
new_value = old_value;             \
if (reg_p & P_CARRY) {             \
new_value |= 0x100;            \
}                                  \
LOCAL_SET_CARRY(new_value & 0x01); \
new_value >>= 1;                   \
LOCAL_SET_NZ(new_value);           \
INC_PC(pc_inc);                    \
ADC(GET_TEMP, 0);                  \
set_func(old_value, new_value)     \
} while (0)
		
		/* RTI does must not use `OPCODE_ENABLES_IRQ()' even if the I flag changes
   from 1 to 0 because the value of I is set 3 cycles before the end of the
   opcode, and thus the 6510 has enough time to call the interrupt routine as
   soon as the opcode ends, if necessary.  */
#define RTI()                        \
do {                             \
WORD tmp;                    \
if (!SKIP_CYCLE) {           \
STACK_PEEK();            \
CLK_INC();               \
}                            \
tmp = (WORD)PULL();          \
CLK_INC();                   \
LOCAL_SET_STATUS((BYTE)tmp); \
tmp = (WORD)PULL();          \
CLK_INC();                   \
tmp |= (WORD)PULL() << 8;    \
CLK_INC();                   \
JUMP(tmp);                   \
} while (0)
		
#define RTS()                 \
do {                      \
WORD tmp;             \
if (!SKIP_CYCLE) {    \
STACK_PEEK();     \
CLK_INC();        \
}                     \
tmp = PULL();         \
CLK_INC();            \
tmp |= (PULL() << 8); \
CLK_INC();            \
LOAD(tmp);            \
CLK_INC();            \
tmp++;                \
c64d_profiler_rts();			  \
JUMP(tmp);            \
} while (0)
		
#define SAC()                       \
do {                            \
reg_a_write_idx = p1 >> 4;  \
reg_a_read_idx = p1 & 0x0f; \
INC_PC(2);                  \
} while (0)
		
#define SBC(get_func, pc_inc)                                                               \
do {                                                                                    \
WORD src, tmp;                                                                      \
\
get_func(src)                                                                       \
tmp = reg_a_read - src - ((reg_p & P_CARRY) ? 0 : 1);                               \
if (reg_p & P_DECIMAL) {                                                            \
unsigned int tmp_a;                                                             \
tmp_a = (reg_a_read & 0xf) - (src & 0xf) - ((reg_p & P_CARRY) ? 0 : 1);         \
if (tmp_a & 0x10) {                                                             \
tmp_a = ((tmp_a - 6) & 0xf) | ((reg_a_read & 0xf0) - (src & 0xf0) - 0x10);  \
} else {                                                                        \
tmp_a = (tmp_a & 0xf) | ((reg_a_read & 0xf0) - (src & 0xf0));               \
}                                                                               \
if (tmp_a & 0x100) {                                                            \
tmp_a -= 0x60;                                                              \
}                                                                               \
LOCAL_SET_CARRY(tmp < 0x100);                                                   \
LOCAL_SET_NZ(tmp & 0xff);                                                       \
LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80) && ((reg_a_read ^ src) & 0x80)); \
reg_a_write = (BYTE) tmp_a;                                                     \
} else {                                                                            \
LOCAL_SET_NZ(tmp & 0xff);                                                       \
LOCAL_SET_CARRY(tmp < 0x100);                                                   \
LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80) && ((reg_a_read ^ src) & 0x80)); \
reg_a_write = (BYTE) tmp;                                                       \
}                                                                                   \
INC_PC(pc_inc);                                                                     \
} while (0)
		
#define SBX()                            \
do {                                 \
unsigned int tmp;                \
INC_PC(2);                       \
tmp = (reg_a_read & reg_x) - p1; \
LOCAL_SET_CARRY(tmp < 0x100);    \
reg_x = tmp & 0xff;              \
LOCAL_SET_NZ(reg_x);             \
} while (0)
		
#undef SEC    /* defined in time.h on SunOS. */
#define SEC()               \
do {                    \
LOCAL_SET_CARRY(1); \
INC_PC(1);          \
} while (0)
		
#define SED()                 \
do {                      \
LOCAL_SET_DECIMAL(1); \
INC_PC(1);            \
} while (0)
		
#define SEI()                      \
do {                           \
if (!LOCAL_INTERRUPT()) {  \
OPCODE_DISABLES_IRQ(); \
}                          \
LOCAL_SET_INTERRUPT(1);    \
INC_PC(1);                 \
} while (0)
		
#define SHA_IND_Y()                                    \
do {                                               \
INT_IND_Y_W_NOADDR();                          \
SET_ABS_SH_I(tmpa, reg_a_read & reg_x, reg_y); \
INC_PC(2);                                     \
} while (0)
		
#define SH_ABS_I(reg_and, reg_i)                                        \
do {                                                                \
if (!SKIP_CYCLE) {                                              \
LOAD_CHECK_BA_LOW(((p2 + reg_i) & 0xff) | ((p2) & 0xff00)); \
CLK_INC();                                                  \
}                                                               \
SET_ABS_SH_I(p2, reg_and, reg_i);                               \
INC_PC(3);                                                      \
} while (0)
		
#define SHS_ABS_Y()                          \
do {                                     \
SH_ABS_I(reg_a_read & reg_x, reg_y); \
reg_sp = reg_a_read & reg_x;         \
} while (0)
		
#define SIR()                  \
do {                       \
reg_y_idx = p1 >> 4;   \
reg_x_idx = p1 & 0x0f; \
INC_PC(2);             \
} while (0)
		
#define SLO(pc_inc, get_func, set_func)       \
do {                                      \
unsigned int old_value, new_value;    \
get_func(old_value)                   \
new_value = old_value;                \
LOCAL_SET_CARRY(old_value & 0x80);    \
new_value <<= 1;                      \
reg_a_write = reg_a_read | new_value; \
LOCAL_SET_NZ(reg_a_read);             \
INC_PC(pc_inc);                       \
set_func(old_value, new_value)        \
} while (0)
		
#define SRE(pc_inc, get_func, set_func)       \
do {                                      \
unsigned int old_value, new_value;    \
get_func(old_value)                   \
new_value = old_value;                \
LOCAL_SET_CARRY(old_value & 0x01);    \
new_value >>= 1;                      \
reg_a_write = reg_a_read ^ new_value; \
LOCAL_SET_NZ(reg_a_read);             \
INC_PC(pc_inc);                       \
set_func(old_value, new_value)        \
} while (0)
		
#define ST(value, set_func, pc_inc) \
do {                            \
INC_PC(pc_inc);             \
set_func(value);            \
} while (0)
		
#define TAX()                     \
do {                          \
reg_x = reg_a_read;       \
LOCAL_SET_NZ(reg_a_read); \
INC_PC(1);                \
} while (0)
		
#define TAY()                     \
do {                          \
reg_y = reg_a_read;       \
LOCAL_SET_NZ(reg_a_read); \
INC_PC(1);                \
} while (0)
		
#define TSX()                 \
do {                      \
reg_x = reg_sp;       \
LOCAL_SET_NZ(reg_sp); \
INC_PC(1);            \
} while (0)
		
#define TXA()                     \
do {                          \
reg_a_write = reg_x;      \
LOCAL_SET_NZ(reg_a_read); \
INC_PC(1);                \
} while (0)
		
#define TXS()           \
do {                \
reg_sp = reg_x; \
INC_PC(1);      \
} while (0)
		
#define TYA()                     \
do {                          \
reg_a_write = reg_y;      \
LOCAL_SET_NZ(reg_a_read); \
INC_PC(1);                \
} while (0)
		
		
		/* ------------------------------------------------------------------------- */
		
		static const BYTE fetch_tab[] = {
			/* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
			/* $00 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $00 */
			/* $10 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $10 */
			/* $20 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $20 */
			/* $30 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $30 */
			/* $40 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $40 */
			/* $50 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $50 */
			/* $60 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $60 */
			/* $70 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $70 */
			/* $80 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $80 */
			/* $90 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $90 */
			/* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $A0 */
			/* $B0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $B0 */
			/* $C0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $C0 */
			/* $D0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $D0 */
			/* $E0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $E0 */
			/* $F0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1  /* $F0 */
		};
		
		
		/* ------------------------------------------------------------------------ */
		
		/* Here, the CPU is emulated. */
		
		{
			//LOGD("reg_pc=%4.4x", reg_pc);
			if (_c64d_new_pc == -1)
			{
				viceCurrentC64PC = reg_pc;
			}
			else
			{
				viceCurrentC64PC = _c64d_new_pc;
			}
			
#ifdef CHECK_AND_RUN_ALTERNATE_CPU
			CHECK_AND_RUN_ALTERNATE_CPU
#endif
			
			while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
				alarm_context_dispatch(ALARM_CONTEXT, CLK);
			}
			
			{
				enum cpu_int pending_interrupt;
				
				if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
					&& (CPU_INT_STATUS->global_pending_int & IK_IRQPEND)
					&& CPU_INT_STATUS->irq_pending_clk <= CLK) {
					interrupt_ack_irq(CPU_INT_STATUS);
				}
				
				c64d_check_cpu_snapshot_manager_store();
				
				pending_interrupt = CPU_INT_STATUS->global_pending_int;
				if (pending_interrupt != IK_NONE)
				{
					DO_INTERRUPT(pending_interrupt);
					
					if ((pending_interrupt & IK_NMI)
						&& (CLK >= (CPU_INT_STATUS->nmi_clk + INTERRUPT_DELAY)))
					{
						c64d_c64_check_irqnmi_breakpoint();
						viceCurrentC64PC = reg_pc;
					}
					
					// c64 debugger - check interrupt breakpoints when irq is ack'ed
					if ((pending_interrupt & IK_IRQ) && ((reg_p & P_INTERRUPT) == P_INTERRUPT))
					{
						if (c64d_c64_is_checking_irq_breakpoints_enabled() == 1)
						{
							if (vicii.c64d_irq_flag == 1)
							{
								c64d_c64_check_irqvic_breakpoint();
								vicii.c64d_irq_flag = 0;
								viceCurrentC64PC = reg_pc;
							}
							
							if (machine_context.cia1->irq_enabled)
							{
								if (machine_context.cia1->c64d_irq_flag == 1)
								{
									c64d_c64_check_irqcia_breakpoint(1);
									machine_context.cia1->c64d_irq_flag = 0;
									viceCurrentC64PC = reg_pc;
								}
							}
							
							if (machine_context.cia2->irq_enabled)
							{
								if (machine_context.cia2->c64d_irq_flag == 1)
								{
									c64d_c64_check_irqcia_breakpoint(2);
									machine_context.cia2->c64d_irq_flag = 0;
									viceCurrentC64PC = reg_pc;
								}
							}
						}
					}
					/////
					
					if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
						&& CPU_INT_STATUS->global_pending_int & IK_IRQPEND) {
						CPU_INT_STATUS->global_pending_int &= ~IK_IRQPEND;
					}
					while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
						alarm_context_dispatch(ALARM_CONTEXT, CLK);
					}
				}
				
			}
			
			{
				c64d_profiler_start_handle_cpu_instruction();

				if (maincpu_clk != c64d_maincpu_previous_instruction_clk)
				{
					c64d_maincpu_previous2_instruction_clk = c64d_maincpu_previous_instruction_clk;
					c64d_maincpu_previous_instruction_clk = maincpu_clk;
				}
				
				c64d_maincpu_current_instruction_clk = maincpu_clk;
				
				if (c64d_debug_mode != DEBUGGER_MODE_RUN_ONE_INSTRUCTION
					&& c64d_debug_mode != DEBUGGER_MODE_RUN_ONE_CYCLE)
				{
					// c64d check PC breakpoint after IRQ or trap
					c64d_c64_check_pc_breakpoint(reg_pc);
					viceCurrentC64PC = reg_pc;
					c64d_debug_pause_check(1);
					debug_iterations_after_restore = 0;
				}
			}
			
			{
				opcode_t opcode;
#ifdef VICE_DEBUG
				debug_clk = maincpu_clk;
#endif
				
#ifdef FEATURE_CPUMEMHISTORY
				memmap_state |= (MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif
				
				debug_iterations_after_restore++;
				debug_prev_iteration_pc = reg_pc;
				SET_LAST_ADDR(reg_pc);
				
				// c64 debugger fix
				if (bank_base == NULL)
				{
					if (((int)reg_pc) < bank_limit)
					{
						JAM();
						continue;
					}
				}
				///
				FETCH_OPCODE(opcode);
				
#ifdef FEATURE_CPUMEMHISTORY
				/* If reg_pc >= bank_limit  then JSR (0x20) hasn't load p2 yet.
				 The earlier LOAD(reg_pc+2) hack can break stealing badly on x64sc.
				 The fixing is now handled in JSR(). */
				monitor_cpuhistory_store(reg_pc, p0, p1, p2 >> 8, reg_a_read, reg_x, reg_y, reg_sp, LOCAL_STATUS());
				memmap_state &= ~(MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif
				
#ifdef VICE_DEBUG
				
				// TODO: shall we put here a hook to C64 Debugger?

				
				if (TRACEFLG) {
					BYTE op = (BYTE)(p0);
					BYTE lo = (BYTE)(p1);
					BYTE hi = (BYTE)(p2 >> 8);
					
					debug_maincpu((DWORD)(reg_pc), debug_clk,
								  mon_disassemble_to_string(e_comp_space,
															(WORD) reg_pc, op,
															lo, hi, 0, 1, "6502"),
								  reg_a_read, reg_x, reg_y, reg_sp);
				}
				if (debug.perform_break_into_monitor) {
					monitor_startup_trap();
					debug.perform_break_into_monitor = 0;
				}
#endif
				
			trap_skipped:
#ifndef OPCODE_UPDATE_IN_FETCH
				SET_LAST_OPCODE(p0);
#endif
				
				switch (p0) {
					case 0x00:          /* BRK */
						c64d_set_debug_mode(DEBUGGER_MODE_PAUSED);
						BRK();
						break;
						
					case 0x01:          /* ORA ($nn,X) */
						ORA(GET_IND_X, 2);
						break;
						
					case 0x02:          /* JAM - also used for traps */
						STATIC_ASSERT(TRAP_OPCODE == 0x02);
						JAM_02();
						break;
						
					case 0x22:          /* JAM */
					case 0x52:          /* JAM */
					case 0x62:          /* JAM */
					case 0x72:          /* JAM */
					case 0x92:          /* JAM */
					case 0xb2:          /* JAM */
					case 0xd2:          /* JAM */
					case 0xf2:          /* JAM */
#ifndef C64DTV
					case 0x12:          /* JAM */
					case 0x32:          /* JAM */
					case 0x42:          /* JAM */
#endif
						REWIND_FETCH_OPCODE(CLK);
						JAM();
						break;
						
#ifdef C64DTV
					case 0x12:          /* BRA $nnnn */
						BRANCH(1);
						break;
						
					case 0x32:          /* SAC #$nn */
						SAC();
						break;
						
					case 0x42:          /* SIR #$nn */
						SIR();
						break;
#endif
						
					case 0x03:          /* SLO ($nn,X) */
						SLO(2, GET_IND_X, SET_IND_RMW);
						break;
						
					case 0x04:          /* NOOP $nn */
					case 0x44:          /* NOOP $nn */
					case 0x64:          /* NOOP $nn */
						NOOP(GET_ZERO_DUMMY, 2);
						break;
						
					case 0x05:          /* ORA $nn */
						ORA(GET_ZERO, 2);
						break;
						
					case 0x06:          /* ASL $nn */
						ASL(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x07:          /* SLO $nn */
						SLO(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x08:          /* PHP */
						PHP();
						break;
						
					case 0x09:          /* ORA #$nn */
						ORA(GET_IMM, 2);
						break;
						
					case 0x0a:          /* ASL A */
						ASL_A();
						break;
						
					case 0x0b:          /* ANC #$nn */
					case 0x2b:          /* ANC #$nn */
						ANC();
						break;
						
					case 0x0c:          /* NOOP $nnnn */
						NOOP(GET_ABS_DUMMY, 3);
						break;
						
					case 0x0d:          /* ORA $nnnn */
						ORA(GET_ABS, 3);
						break;
						
					case 0x0e:          /* ASL $nnnn */
						ASL(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x0f:          /* SLO $nnnn */
						SLO(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x10:          /* BPL $nnnn */
						BRANCH(!LOCAL_SIGN());
						break;
						
					case 0x11:          /* ORA ($nn),Y */
						ORA(GET_IND_Y, 2);
						break;
						
					case 0x13:          /* SLO ($nn),Y */
						SLO(2, GET_IND_Y_RMW, SET_IND_RMW);
						break;
						
					case 0x14:          /* NOOP $nn,X */
					case 0x34:          /* NOOP $nn,X */
					case 0x54:          /* NOOP $nn,X */
					case 0x74:          /* NOOP $nn,X */
					case 0xd4:          /* NOOP $nn,X */
					case 0xf4:          /* NOOP $nn,X */
						NOOP(GET_ZERO_X_DUMMY, 2);
						break;
						
					case 0x15:          /* ORA $nn,X */
						ORA(GET_ZERO_X, 2);
						break;
						
					case 0x16:          /* ASL $nn,X */
						ASL(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x17:          /* SLO $nn,X */
						SLO(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x18:          /* CLC */
						CLC();
						break;
						
					case 0x19:          /* ORA $nnnn,Y */
						ORA(GET_ABS_Y, 3);
						break;
						
					case 0x1a:          /* NOOP */
					case 0x3a:          /* NOOP */
					case 0x5a:          /* NOOP */
					case 0x7a:          /* NOOP */
					case 0xda:          /* NOOP */
					case 0xfa:          /* NOOP */
					case 0xea:          /* NOP */
						NOOP(GET_IMM_DUMMY, 1);
						break;
						
					case 0x1b:          /* SLO $nnnn,Y */
						SLO(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
						break;
						
					case 0x1c:          /* NOOP $nnnn,X */
					case 0x3c:          /* NOOP $nnnn,X */
					case 0x5c:          /* NOOP $nnnn,X */
					case 0x7c:          /* NOOP $nnnn,X */
					case 0xdc:          /* NOOP $nnnn,X */
					case 0xfc:          /* NOOP $nnnn,X */
						NOOP(GET_ABS_X_DUMMY, 3);
						break;
						
					case 0x1d:          /* ORA $nnnn,X */
						ORA(GET_ABS_X, 3);
						break;
						
					case 0x1e:          /* ASL $nnnn,X */
						ASL(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x1f:          /* SLO $nnnn,X */
						SLO(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x20:          /* JSR $nnnn */
						JSR();
						break;
						
					case 0x21:          /* AND ($nn,X) */
						AND(GET_IND_X, 2);
						break;
						
					case 0x23:          /* RLA ($nn,X) */
						RLA(2, GET_IND_X, SET_IND_RMW);
						break;
						
					case 0x24:          /* BIT $nn */
						BIT(GET_ZERO, 2);
						break;
						
					case 0x25:          /* AND $nn */
						AND(GET_ZERO, 2);
						break;
						
					case 0x26:          /* ROL $nn */
						ROL(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x27:          /* RLA $nn */
						RLA(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x28:          /* PLP */
						PLP();
						break;
						
					case 0x29:          /* AND #$nn */
						AND(GET_IMM, 2);
						break;
						
					case 0x2a:          /* ROL A */
						ROL_A();
						break;
						
					case 0x2c:          /* BIT $nnnn */
						BIT(GET_ABS, 3);
						break;
						
					case 0x2d:          /* AND $nnnn */
						AND(GET_ABS, 3);
						break;
						
					case 0x2e:          /* ROL $nnnn */
						ROL(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x2f:          /* RLA $nnnn */
						RLA(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x30:          /* BMI $nnnn */
						BRANCH(LOCAL_SIGN());
						break;
						
					case 0x31:          /* AND ($nn),Y */
						AND(GET_IND_Y, 2);
						break;
						
					case 0x33:          /* RLA ($nn),Y */
						RLA(2, GET_IND_Y_RMW, SET_IND_RMW);
						break;
						
					case 0x35:          /* AND $nn,X */
						AND(GET_ZERO_X, 2);
						break;
						
					case 0x36:          /* ROL $nn,X */
						ROL(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x37:          /* RLA $nn,X */
						RLA(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x38:          /* SEC */
						SEC();
						break;
						
					case 0x39:          /* AND $nnnn,Y */
						AND(GET_ABS_Y, 3);
						break;
						
					case 0x3b:          /* RLA $nnnn,Y */
						RLA(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
						break;
						
					case 0x3d:          /* AND $nnnn,X */
						AND(GET_ABS_X, 3);
						break;
						
					case 0x3e:          /* ROL $nnnn,X */
						ROL(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x3f:          /* RLA $nnnn,X */
						RLA(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x40:          /* RTI */
						RTI();
						break;
						
					case 0x41:          /* EOR ($nn,X) */
						EOR(GET_IND_X, 2);
						break;
						
					case 0x43:          /* SRE ($nn,X) */
						SRE(2, GET_IND_X, SET_IND_RMW);
						break;
						
					case 0x45:          /* EOR $nn */
						EOR(GET_ZERO, 2);
						break;
						
					case 0x46:          /* LSR $nn */
						LSR(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x47:          /* SRE $nn */
						SRE(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x48:          /* PHA */
						PHA();
						break;
						
					case 0x49:          /* EOR #$nn */
						EOR(GET_IMM, 2);
						break;
						
					case 0x4a:          /* LSR A */
						LSR_A();
						break;
						
					case 0x4b:          /* ASR #$nn */
						ASR();
						break;
						
					case 0x4c:          /* JMP $nnnn */
						JMP(p2);
						break;
						
					case 0x4d:          /* EOR $nnnn */
						EOR(GET_ABS, 3);
						break;
						
					case 0x4e:          /* LSR $nnnn */
						LSR(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x4f:          /* SRE $nnnn */
						SRE(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x50:          /* BVC $nnnn */
						BRANCH(!LOCAL_OVERFLOW());
						break;
						
					case 0x51:          /* EOR ($nn),Y */
						EOR(GET_IND_Y, 2);
						break;
						
					case 0x53:          /* SRE ($nn),Y */
						SRE(2, GET_IND_Y_RMW, SET_IND_RMW);
						break;
						
					case 0x55:          /* EOR $nn,X */
						EOR(GET_ZERO_X, 2);
						break;
						
					case 0x56:          /* LSR $nn,X */
						LSR(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x57:          /* SRE $nn,X */
						SRE(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x58:          /* CLI */
						CLI();
						break;
						
					case 0x59:          /* EOR $nnnn,Y */
						EOR(GET_ABS_Y, 3);
						break;
						
					case 0x5b:          /* SRE $nnnn,Y */
						SRE(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
						break;
						
					case 0x5d:          /* EOR $nnnn,X */
						EOR(GET_ABS_X, 3);
						break;
						
					case 0x5e:          /* LSR $nnnn,X */
						LSR(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x5f:          /* SRE $nnnn,X */
						SRE(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x60:          /* RTS */
						RTS();
						break;
						
					case 0x61:          /* ADC ($nn,X) */
						ADC(GET_IND_X, 2);
						break;
						
					case 0x63:          /* RRA ($nn,X) */
						RRA(2, GET_IND_X, SET_IND_RMW);
						break;
						
					case 0x65:          /* ADC $nn */
						ADC(GET_ZERO, 2);
						break;
						
					case 0x66:          /* ROR $nn */
						ROR(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x67:          /* RRA $nn */
						RRA(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0x68:          /* PLA */
						PLA();
						break;
						
					case 0x69:          /* ADC #$nn */
						ADC(GET_IMM, 2);
						break;
						
					case 0x6a:          /* ROR A */
						ROR_A();
						break;
						
					case 0x6b:          /* ARR #$nn */
						ARR();
						break;
						
					case 0x6c:          /* JMP ($nnnn) */
						JMP_IND();
						break;
						
					case 0x6d:          /* ADC $nnnn */
						ADC(GET_ABS, 3);
						break;
						
					case 0x6e:          /* ROR $nnnn */
						ROR(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x6f:          /* RRA $nnnn */
						RRA(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0x70:          /* BVS $nnnn */
						BRANCH(LOCAL_OVERFLOW());
						break;
						
					case 0x71:          /* ADC ($nn),Y */
						ADC(GET_IND_Y, 2);
						break;
						
					case 0x73:          /* RRA ($nn),Y */
						RRA(2, GET_IND_Y_RMW, SET_IND_RMW);
						break;
						
					case 0x75:          /* ADC $nn,X */
						ADC(GET_ZERO_X, 2);
						break;
						
					case 0x76:          /* ROR $nn,X */
						ROR(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x77:          /* RRA $nn,X */
						RRA(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0x78:          /* SEI */
						SEI();
						break;
						
					case 0x79:          /* ADC $nnnn,Y */
						ADC(GET_ABS_Y, 3);
						break;
						
					case 0x7b:          /* RRA $nnnn,Y */
						RRA(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
						break;
						
					case 0x7d:          /* ADC $nnnn,X */
						ADC(GET_ABS_X, 3);
						break;
						
					case 0x7e:          /* ROR $nnnn,X */
						ROR(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x7f:          /* RRA $nnnn,X */
						RRA(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0x80:          /* NOOP #$nn */
					case 0x82:          /* NOOP #$nn */
					case 0x89:          /* NOOP #$nn */
					case 0xc2:          /* NOOP #$nn */
					case 0xe2:          /* NOOP #$nn */
						NOOP(GET_IMM_DUMMY, 2);
						break;
						
					case 0x81:          /* STA ($nn,X) */
						ST(reg_a_read, SET_IND_X, 2);
						break;
						
					case 0x83:          /* SAX ($nn,X) */
						ST(reg_a_read & reg_x, SET_IND_X, 2);
						break;
						
					case 0x84:          /* STY $nn */
						ST(reg_y, SET_ZERO, 2);
						break;
						
					case 0x85:          /* STA $nn */
						ST(reg_a_read, SET_ZERO, 2);
						break;
						
					case 0x86:          /* STX $nn */
						ST(reg_x, SET_ZERO, 2);
						break;
						
					case 0x87:          /* SAX $nn */
						ST(reg_a_read & reg_x, SET_ZERO, 2);
						break;
						
					case 0x88:          /* DEY */
						DEY();
						break;
						
					case 0x8a:          /* TXA */
						TXA();
						break;
						
					case 0x8b:          /* ANE #$nn */
						ANE();
						break;
						
					case 0x8c:          /* STY $nnnn */
						ST(reg_y, SET_ABS, 3);
						break;
						
					case 0x8d:          /* STA $nnnn */
						ST(reg_a_read, SET_ABS, 3);
						break;
						
					case 0x8e:          /* STX $nnnn */
						ST(reg_x, SET_ABS, 3);
						break;
						
					case 0x8f:          /* SAX $nnnn */
						ST(reg_a_read & reg_x, SET_ABS, 3);
						break;
						
					case 0x90:          /* BCC $nnnn */
						BRANCH(!LOCAL_CARRY());
						break;
						
					case 0x91:          /* STA ($nn),Y */
						ST(reg_a_read, SET_IND_Y, 2);
						break;
						
					case 0x93:          /* SHA ($nn),Y */
						SHA_IND_Y();
						break;
						
					case 0x94:          /* STY $nn,X */
						ST(reg_y, SET_ZERO_X, 2);
						break;
						
					case 0x95:          /* STA $nn,X */
						ST(reg_a_read, SET_ZERO_X, 2);
						break;
						
					case 0x96:          /* STX $nn,Y */
						ST(reg_x, SET_ZERO_Y, 2);
						break;
						
					case 0x97:          /* SAX $nn,Y */
						ST(reg_a_read & reg_x, SET_ZERO_Y, 2);
						break;
						
					case 0x98:          /* TYA */
						TYA();
						break;
						
					case 0x99:          /* STA $nnnn,Y */
						ST(reg_a_read, SET_ABS_Y, 3);
						break;
						
					case 0x9a:          /* TXS */
						TXS();
						break;
						
					case 0x9b:          /* NOP (SHS) $nnnn,Y */
#ifdef C64DTV
						NOOP(GET_ABS_Y_DUMMY, 3);
#else
						SHS_ABS_Y();
#endif
						break;
						
					case 0x9c:          /* SHY $nnnn,X */
						SH_ABS_I(reg_y, reg_x);
						break;
						
					case 0x9d:          /* STA $nnnn,X */
						ST(reg_a_read, SET_ABS_X, 3);
						break;
						
					case 0x9e:          /* SHX $nnnn,Y */
						SH_ABS_I(reg_x, reg_y);
						break;
						
					case 0x9f:          /* SHA $nnnn,Y */
						SH_ABS_I(reg_a_read & reg_x, reg_y);
						break;
						
					case 0xa0:          /* LDY #$nn */
						LD(reg_y, GET_IMM, 2);
						break;
						
					case 0xa1:          /* LDA ($nn,X) */
						LD(reg_a_write, GET_IND_X, 2);
						break;
						
					case 0xa2:          /* LDX #$nn */
						LD(reg_x, GET_IMM, 2);
						break;
						
					case 0xa3:          /* LAX ($nn,X) */
						LAX(GET_IND_X, 2);
						break;
						
					case 0xa4:          /* LDY $nn */
						LD(reg_y, GET_ZERO, 2);
						break;
						
					case 0xa5:          /* LDA $nn */
						LD(reg_a_write, GET_ZERO, 2);
						break;
						
					case 0xa6:          /* LDX $nn */
						LD(reg_x, GET_ZERO, 2);
						break;
						
					case 0xa7:          /* LAX $nn */
						LAX(GET_ZERO, 2);
						break;
						
					case 0xa8:          /* TAY */
						TAY();
						break;
						
					case 0xa9:          /* LDA #$nn */
						LD(reg_a_write, GET_IMM, 2);
						break;
						
					case 0xaa:          /* TAX */
						TAX();
						break;
						
					case 0xab:          /* LXA #$nn */
						LXA(p1, 2);
						break;
						
					case 0xac:          /* LDY $nnnn */
						LD(reg_y, GET_ABS, 3);
						break;
						
					case 0xad:          /* LDA $nnnn */
						LD(reg_a_write, GET_ABS, 3);
						break;
						
					case 0xae:          /* LDX $nnnn */
						LD(reg_x, GET_ABS, 3);
						break;
						
					case 0xaf:          /* LAX $nnnn */
						LAX(GET_ABS, 3);
						break;
						
					case 0xb0:          /* BCS $nnnn */
						BRANCH(LOCAL_CARRY());
						break;
						
					case 0xb1:          /* LDA ($nn),Y */
						LD(reg_a_write, GET_IND_Y, 2);
						break;
						
					case 0xb3:          /* LAX ($nn),Y */
						LAX(GET_IND_Y, 2);
						break;
						
					case 0xb4:          /* LDY $nn,X */
						LD(reg_y, GET_ZERO_X, 2);
						break;
						
					case 0xb5:          /* LDA $nn,X */
						LD(reg_a_write, GET_ZERO_X, 2);
						break;
						
					case 0xb6:          /* LDX $nn,Y */
						LD(reg_x, GET_ZERO_Y, 2);
						break;
						
					case 0xb7:          /* LAX $nn,Y */
						LAX(GET_ZERO_Y, 2);
						break;
						
					case 0xb8:          /* CLV */
						CLV();
						break;
						
					case 0xb9:          /* LDA $nnnn,Y */
						LD(reg_a_write, GET_ABS_Y, 3);
						break;
						
					case 0xba:          /* TSX */
						TSX();
						break;
						
					case 0xbb:          /* LAS $nnnn,Y */
						LAS();
						break;
						
					case 0xbc:          /* LDY $nnnn,X */
						LD(reg_y, GET_ABS_X, 3);
						break;
						
					case 0xbd:          /* LDA $nnnn,X */
						LD(reg_a_write, GET_ABS_X, 3);
						break;
						
					case 0xbe:          /* LDX $nnnn,Y */
						LD(reg_x, GET_ABS_Y, 3);
						break;
						
					case 0xbf:          /* LAX $nnnn,Y */
						LAX(GET_ABS_Y, 3);
						break;
						
					case 0xc0:          /* CPY #$nn */
						CP(reg_y, GET_IMM, 2);
						break;
						
					case 0xc1:          /* CMP ($nn,X) */
						CP(reg_a_read, GET_IND_X, 2);
						break;
						
					case 0xc3:          /* DCP ($nn,X) */
						DCP(2, GET_IND_X, SET_IND_RMW);
						break;
						
					case 0xc4:          /* CPY $nn */
						CP(reg_y, GET_ZERO, 2);
						break;
						
					case 0xc5:          /* CMP $nn */
						CP(reg_a_read, GET_ZERO, 2);
						break;
						
					case 0xc6:          /* DEC $nn */
						DEC(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0xc7:          /* DCP $nn */
						DCP(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0xc8:          /* INY */
						INY();
						break;
						
					case 0xc9:          /* CMP #$nn */
						CP(reg_a_read, GET_IMM, 2);
						break;
						
					case 0xca:          /* DEX */
						DEX();
						break;
						
					case 0xcb:          /* SBX #$nn */
						SBX();
						break;
						
					case 0xcc:          /* CPY $nnnn */
						CP(reg_y, GET_ABS, 3);
						break;
						
					case 0xcd:          /* CMP $nnnn */
						CP(reg_a_read, GET_ABS, 3);
						break;
						
					case 0xce:          /* DEC $nnnn */
						DEC(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0xcf:          /* DCP $nnnn */
						DCP(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0xd0:          /* BNE $nnnn */
						BRANCH(!LOCAL_ZERO());
						break;
						
					case 0xd1:          /* CMP ($nn),Y */
						CP(reg_a_read, GET_IND_Y, 2);
						break;
						
					case 0xd3:          /* DCP ($nn),Y */
						DCP(2, GET_IND_Y_RMW, SET_IND_RMW);
						break;
						
					case 0xd5:          /* CMP $nn,X */
						CP(reg_a_read, GET_ZERO_X, 2);
						break;
						
					case 0xd6:          /* DEC $nn,X */
						DEC(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0xd7:          /* DCP $nn,X */
						DCP(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0xd8:          /* CLD */
						CLD();
						break;
						
					case 0xd9:          /* CMP $nnnn,Y */
						CP(reg_a_read, GET_ABS_Y, 3);
						break;
						
					case 0xdb:          /* DCP $nnnn,Y */
						DCP(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
						break;
						
					case 0xdd:          /* CMP $nnnn,X */
						CP(reg_a_read, GET_ABS_X, 3);
						break;
						
					case 0xde:          /* DEC $nnnn,X */
						DEC(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0xdf:          /* DCP $nnnn,X */
						DCP(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0xe0:          /* CPX #$nn */
						CP(reg_x, GET_IMM, 2);
						break;
						
					case 0xe1:          /* SBC ($nn,X) */
						SBC(GET_IND_X, 2);
						break;
						
					case 0xe3:          /* ISB ($nn,X) */
						ISB(2, GET_IND_X, SET_IND_RMW);
						break;
						
					case 0xe4:          /* CPX $nn */
						CP(reg_x, GET_ZERO, 2);
						break;
						
					case 0xe5:          /* SBC $nn */
						SBC(GET_ZERO, 2);
						break;
						
					case 0xe6:          /* INC $nn */
						INC(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0xe7:          /* ISB $nn */
						ISB(2, GET_ZERO, SET_ZERO_RMW);
						break;
						
					case 0xe8:          /* INX */
						INX();
						break;
						
					case 0xe9:          /* SBC #$nn */
					case 0xeb:          /* USBC #$nn (same as SBC) */
						SBC(GET_IMM, 2);
						break;
						
					case 0xec:          /* CPX $nnnn */
						CP(reg_x, GET_ABS, 3);
						break;
						
					case 0xed:          /* SBC $nnnn */
						SBC(GET_ABS, 3);
						break;
						
					case 0xee:          /* INC $nnnn */
						INC(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0xef:          /* ISB $nnnn */
						ISB(3, GET_ABS, SET_ABS_RMW);
						break;
						
					case 0xf0:          /* BEQ $nnnn */
						BRANCH(LOCAL_ZERO());
						break;
						
					case 0xf1:          /* SBC ($nn),Y */
						SBC(GET_IND_Y, 2);
						break;
						
					case 0xf3:          /* ISB ($nn),Y */
						ISB(2, GET_IND_Y_RMW, SET_IND_RMW);
						break;
						
					case 0xf5:          /* SBC $nn,X */
						SBC(GET_ZERO_X, 2);
						break;
						
					case 0xf6:          /* INC $nn,X */
						INC(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0xf7:          /* ISB $nn,X */
						ISB(2, GET_ZERO_X, SET_ZERO_X_RMW);
						break;
						
					case 0xf8:          /* SED */
						SED();
						break;
						
					case 0xf9:          /* SBC $nnnn,Y */
						SBC(GET_ABS_Y, 3);
						break;
						
					case 0xfb:          /* ISB $nnnn,Y */
						ISB(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
						break;
						
					case 0xfd:          /* SBC $nnnn,X */
						SBC(GET_ABS_X, 3);
						break;
						
					case 0xfe:          /* INC $nnnn,X */
						INC(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
						
					case 0xff:          /* ISB $nnnn,X */
						ISB(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
						break;
				}
			}
		}
		
		///
		/// end of 6510dtvcore.c
		
		// c64d: mark cell execute
		c64d_mark_c64_cell_execute(LAST_OPCODE_ADDR & 0xFFFF, LAST_OPCODE_INFO & 0xFF);
		
		c64d_profiler_end_cpu_instruction();
		
		c64d_c64_instruction_cycle = 0;
		
		if (c64d_is_debug_on_c64())
		{
			// one instruction has been just run, pause
			if (c64d_debug_mode == DEBUGGER_MODE_RUN_ONE_INSTRUCTION)
			{
				c64d_debug_mode = DEBUGGER_MODE_PAUSED;
				LOGD("maincpuclk=%d previous=%d previous2=%d",
					 maincpu_clk, c64d_maincpu_previous_instruction_clk, c64d_maincpu_previous2_instruction_clk);
			}
			
			//c64d_c64_check_pc_breakpoint(reg_pc);
			
			viceCurrentC64PC = reg_pc;
			
			c64d_debug_pause_check(0);
		}
		
		//		if (c64d_debug_mode == C64_DEBUG_SHUTDOWN)
		//		{
		//			//LOGD("c64d_debug_mode=C64_DEBUG_SHUTDOWN!");
		//			return;
		//		}
		
		///

		
		maincpu_int_status->num_dma_per_opcode = 0;
		
		if (maincpu_clk_limit && (maincpu_clk > maincpu_clk_limit)) {
			log_error(LOG_DEFAULT, "cycle limit reached.");
			exit(EXIT_FAILURE);
		}
#if 0
		if (CLK > 246171754) {
			debug.maincpu_traceflg = 1;
		}
#endif
	}
}

void c64d_set_debug_mode(int newMode)
{
	LOGD("c64d_set_debug_mode: %d", newMode);
	c64d_debug_mode = newMode;
}

int c64d_c64_do_cycle()
{
	// do one machine cycle
	//	LOGD("c64d_do_cycle: %d", maincpu_clk);
	
	c64d_c64_instruction_cycle++;
	
	if (c64d_is_debug_on_c64())
	{
		if (c64d_debug_mode == DEBUGGER_MODE_RUN_ONE_CYCLE)
		{
			c64d_debug_mode = DEBUGGER_MODE_PAUSED;
		}
		
		if (c64d_debug_mode == DEBUGGER_MODE_RUNNING)
		{
			return vicii_cycle();
		}
		
		c64d_debug_pause_check(0);
	}
	
	return vicii_cycle();
}

void c64d_stop_vice_emulation()
{
	
}

/* ------------------------------------------------------------------------- */

void maincpu_set_pc(int pc) {
	MOS6510_REGS_SET_PC(&maincpu_regs, pc);
}

void maincpu_set_a(int a) {
	MOS6510_REGS_SET_A(&maincpu_regs, a);
}

void maincpu_set_x(int x) {
	MOS6510_REGS_SET_X(&maincpu_regs, x);
}

void maincpu_set_y(int y) {
	MOS6510_REGS_SET_Y(&maincpu_regs, y);
}

void maincpu_set_sign(int n) {
	MOS6510_REGS_SET_SIGN(&maincpu_regs, n);
}

void maincpu_set_zero(int z) {
	MOS6510_REGS_SET_ZERO(&maincpu_regs, z);
}

void maincpu_set_carry(int c) {
	MOS6510_REGS_SET_CARRY(&maincpu_regs, c);
}

void maincpu_set_interrupt(int i) {
	MOS6510_REGS_SET_INTERRUPT(&maincpu_regs, i);
}

unsigned int maincpu_get_pc(void) {
	return MOS6510_REGS_GET_PC(&maincpu_regs);
}

unsigned int maincpu_get_a(void) {
	return MOS6510_REGS_GET_A(&maincpu_regs);
}

unsigned int maincpu_get_x(void) {
	return MOS6510_REGS_GET_X(&maincpu_regs);
}

unsigned int maincpu_get_y(void) {
	return MOS6510_REGS_GET_Y(&maincpu_regs);
}

unsigned int maincpu_get_sp(void) {
	return MOS6510_REGS_GET_SP(&maincpu_regs);
}

/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "MAINCPU";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int maincpu_snapshot_write_module(snapshot_t *s)
{
	snapshot_module_t *m;
	
	m = snapshot_module_create(s, snap_module_name, ((BYTE)SNAP_MAJOR),
							   ((BYTE)SNAP_MINOR));
	if (m == NULL) {
		return -1;
	}
	
	if (0
		|| SMW_DW(m, maincpu_clk) < 0
		|| SMW_B(m, MOS6510_REGS_GET_A(&maincpu_regs)) < 0
		|| SMW_B(m, MOS6510_REGS_GET_X(&maincpu_regs)) < 0
		|| SMW_B(m, MOS6510_REGS_GET_Y(&maincpu_regs)) < 0
		|| SMW_B(m, MOS6510_REGS_GET_SP(&maincpu_regs)) < 0
		|| SMW_W(m, (WORD)MOS6510_REGS_GET_PC(&maincpu_regs)) < 0
		|| SMW_B(m, (BYTE)MOS6510_REGS_GET_STATUS(&maincpu_regs)) < 0
		|| SMW_DW(m, (DWORD)last_opcode_info) < 0
		|| SMW_DW(m, (DWORD)maincpu_ba_low_flags) < 0) {
		goto fail;
	}
	
	if (interrupt_write_snapshot(maincpu_int_status, m) < 0) {
		goto fail;
	}
	
	if (interrupt_write_new_snapshot(maincpu_int_status, m) < 0) {
		goto fail;
	}
	
	if (interrupt_write_sc_snapshot(maincpu_int_status, m) < 0) {
		goto fail;
	}
	
	return snapshot_module_close(m);
	
fail:
	if (m != NULL) {
		snapshot_module_close(m);
	}
	return -1;
}

int maincpu_snapshot_read_module(snapshot_t *s)
{
	BYTE a, x, y, sp, status;
	WORD pc;
	BYTE major, minor;
	snapshot_module_t *m;
	
	m = snapshot_module_open(s, snap_module_name, &major, &minor);
	if (m == NULL) {
		return -1;
	}
	
	/* XXX: Assumes `CLOCK' is the same size as a `DWORD'.  */
	if (0
		|| SMR_DW(m, &maincpu_clk) < 0
		|| SMR_B(m, &a) < 0
		|| SMR_B(m, &x) < 0
		|| SMR_B(m, &y) < 0
		|| SMR_B(m, &sp) < 0
		|| SMR_W(m, &pc) < 0
		|| SMR_B(m, &status) < 0
		|| SMR_DW_UINT(m, &last_opcode_info) < 0
		|| SMR_DW_INT(m, &maincpu_ba_low_flags) < 0) {
		goto fail;
	}
	
	MOS6510_REGS_SET_A(&maincpu_regs, a);
	MOS6510_REGS_SET_X(&maincpu_regs, x);
	MOS6510_REGS_SET_Y(&maincpu_regs, y);
	MOS6510_REGS_SET_SP(&maincpu_regs, sp);
	MOS6510_REGS_SET_PC(&maincpu_regs, pc);
	MOS6510_REGS_SET_STATUS(&maincpu_regs, status);
	
	if (interrupt_read_snapshot(maincpu_int_status, m) < 0) {
		goto fail;
	}
	
	if (interrupt_read_new_snapshot(maincpu_int_status, m) < 0) {
		goto fail;
	}
	
	if (interrupt_read_sc_snapshot(maincpu_int_status, m) < 0) {
		goto fail;
	}
	
	return snapshot_module_close(m);
	
fail:
	if (m != NULL) {
		snapshot_module_close(m);
	}
	return -1;
}

int c64d_check_cpu_snapshot_manager_restore()
{
	if (c64d_check_snapshot_restore())
	{
		enum cpu_int pending_interrupt;

//		EXPORT_REGISTERS();                                            \
//		interrupt_do_trap(CPU_INT_STATUS, (WORD)reg_pc);               \
//		IMPORT_REGISTERS();                                            \
//		if (CPU_INT_STATUS->global_pending_int & IK_RESET) {           \
//			ik |= IK_RESET;                                            \
//		}
		
		LOGD("GLOBAL_REGS.pc=%04x", GLOBAL_REGS.pc);
		
		IMPORT_REGISTERS();
		viceCurrentC64PC = maincpu_get_pc();
		
		LOGD("viceCurrentC64PC=%04x reg_pc=%04x", viceCurrentC64PC, reg_pc);
		
		// TODO: this is copy-pasted from CPU emulation, make generic function
		pending_interrupt = CPU_INT_STATUS->global_pending_int;
		if (pending_interrupt != IK_NONE)
		{
			DO_INTERRUPT(pending_interrupt);
			
			if ((pending_interrupt & IK_NMI)
				&& (CLK >= (CPU_INT_STATUS->nmi_clk + INTERRUPT_DELAY)))
			{
				c64d_c64_check_irqnmi_breakpoint();
				viceCurrentC64PC = reg_pc;
			}
			
			// c64 debugger - check interrupt breakpoints when irq is ack'ed
			if ((pending_interrupt & IK_IRQ) && ((reg_p & P_INTERRUPT) == P_INTERRUPT))
			{
				if (c64d_c64_is_checking_irq_breakpoints_enabled() == 1)
				{
					if (vicii.c64d_irq_flag == 1)
					{
						c64d_c64_check_irqvic_breakpoint();
						vicii.c64d_irq_flag = 0;
						viceCurrentC64PC = reg_pc;
					}
					
					if (machine_context.cia1->irq_enabled)
					{
						if (machine_context.cia1->c64d_irq_flag == 1)
						{
							c64d_c64_check_irqcia_breakpoint(1);
							machine_context.cia1->c64d_irq_flag = 0;
							viceCurrentC64PC = reg_pc;
						}
					}
					
					if (machine_context.cia2->irq_enabled)
					{
						if (machine_context.cia2->c64d_irq_flag == 1)
						{
							c64d_c64_check_irqcia_breakpoint(2);
							machine_context.cia2->c64d_irq_flag = 0;
							viceCurrentC64PC = reg_pc;
						}
					}
				}
			}
			/////
			
			if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
				&& CPU_INT_STATUS->global_pending_int & IK_IRQPEND) {
				CPU_INT_STATUS->global_pending_int &= ~IK_IRQPEND;
			}
			while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
				alarm_context_dispatch(ALARM_CONTEXT, CLK);
			}
		}

		
		
		
			/* OLD
		if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
			&& CPU_INT_STATUS->global_pending_int & IK_IRQPEND) {
			CPU_INT_STATUS->global_pending_int &= ~IK_IRQPEND;
		}
		while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
			alarm_context_dispatch(ALARM_CONTEXT, CLK);
		}
			 */
		
		LOGD("after alarm_context_dispatch: viceCurrentC64PC=%04x reg_pc=%04x cycle=%d", viceCurrentC64PC, reg_pc, maincpu_clk);
		
		return 1;
	}
	return 0;
}

void c64d_check_cpu_snapshot_manager_store()
{
	// check snapshot interval by snapshot manager
	EXPORT_REGISTERS();
	c64d_check_snapshot_interval();
	IMPORT_REGISTERS();
}


/// "../mainc64cpu.c" ends here
///
