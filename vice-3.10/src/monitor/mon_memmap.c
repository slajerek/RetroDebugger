/*
 * mon_memmap.c - The VICE built-in monitor, memmap/cpuhistory functions.
 *
 * Written by
 *  Hannu Nuotio <nojoopa@users.sourceforge.net>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mon_disassemble.h"
#include "mon_memmap.h"
#include "monitor.h"
#include "montypes.h"
#include "screenshot.h"
#include "types.h"


/* Globals */

uint8_t memmap_state = 0;

#ifdef FEATURE_CPUMEMHISTORY

/* Defines */

#define MEMMAP_SIZE 0x10000
#define MEMMAP_PICX 0x100
#define MEMMAP_PICY 0x100

#define OP_JSR 0x20
#define OP_RTI 0x40
#define OP_RTS 0x60

#define BAD_ADDR (new_addr(e_invalid_space, 0))

/* Types */

#define MEMMAP_ELEM uint16_t

/* CPU history variables */
static cpuhistory_t *cpuhistory = NULL;
static int cpuhistory_buffer_lines = 0;     /* actual size of the cyclic buffer */
static int cpuhistory_show_lines = 0;       /* number of lines to show in the monitor */
static int cpuhistory_i = 0;


/** \brief  (re)allocate the buffer used for the cpu history info
 *
 * \param[in]   lines   new number of lines of the cpu history info
 */
int monitor_cpuhistory_allocate(int lines)
{
    uint32_t i;

    if (lines <= 0) {
        fprintf(stderr, "%s(): illegal cpuhistory line count: %d\n",
                __func__, lines);
        return -1;
    }

    cpuhistory_show_lines = lines;

    /* HACK HACK HACK
       since all CPUs are sharing one cyclic buffer for the history, we must make
       sure that one CPU can not "flush out" the history of another CPU. For this
       to work we need to buffer at least one frame, since the emulation is synced
       at least once per frame.

       312 lines * 65 cycles makes 20280 cycles per frame, that 10140 NOPs

       For now, we use this number as minimum, and multiply by the number of
       CPUs when the buffer is actually allocated.

       also see https://sourceforge.net/p/vice-emu/bugs/1994/
    */
    if (lines < 10140) {
        lines = 10140;
    }
    lines = lines * 5 + 1;

    cpuhistory = lib_realloc(cpuhistory, (size_t)lines * sizeof(cpuhistory_t));

    /* do we resize the array? */
    if (cpuhistory_buffer_lines != lines) {
        /* Initialize array to avoid mon_memmap_store() using unitialized
         * data when reading the RESET vector on boot.
         * WHY reading the RESET vector causes a STORE is another issue.
         * -- Compyx
         * */
        memset((void *)cpuhistory, 0, sizeof(cpuhistory_t) * (size_t)lines);
        /* flag lines so they won't output anything after startup */
        for (i = 0; i < lines ; i++) {
            cpuhistory[i].origin = e_invalid_space;
        }
    }

    cpuhistory_buffer_lines = lines;
    cpuhistory_i = 0;
    return 0;
}


void monitor_cpuhistory_store(CLOCK cycle, unsigned int addr, unsigned int op,
                              unsigned int p1, unsigned int p2,
                              uint8_t reg_a,
                              uint8_t reg_x,
                              uint8_t reg_y,
                              uint8_t reg_sp,
                              unsigned int reg_st,
                              MEMSPACE origin)
{
    if (machine_is_jammed()) {
        return;
    }

    ++cpuhistory_i;
    if (cpuhistory_i == cpuhistory_buffer_lines) {
        cpuhistory_i = 0;
    }
    cpuhistory[cpuhistory_i].cycle = cycle;
    cpuhistory[cpuhistory_i].addr = addr;
    cpuhistory[cpuhistory_i].op = op;
    cpuhistory[cpuhistory_i].p1 = p1;
    cpuhistory[cpuhistory_i].p2 = p2;
    cpuhistory[cpuhistory_i].reg_a = reg_a;
    cpuhistory[cpuhistory_i].reg_x = reg_x;
    cpuhistory[cpuhistory_i].reg_y = reg_y;
    cpuhistory[cpuhistory_i].reg_sp = reg_sp;
    cpuhistory[cpuhistory_i].reg_st = reg_st;
    cpuhistory[cpuhistory_i].origin = origin;
}

void monitor_cpuhistory_fix_p2(unsigned int p2)
{
    cpuhistory[cpuhistory_i].p2 = p2;
}

cpuhistory_t *mon_cpuhistory_seek(int count, MEMSPACE filter1, MEMSPACE filter2,
                                  MEMSPACE filter3, MEMSPACE filter4, MEMSPACE filter5) {
    int i, pos;

    /* 'i' is the actual counter */
    i = 0;
    /* start looking at last entry */
    pos = cpuhistory_i;

    if (count >= cpuhistory_buffer_lines) {
        count = cpuhistory_buffer_lines - 1;
    }

    /* find out where we need to start */
    while (i < count) {
        /* make sure the record matches */
        if ((cpuhistory[pos].origin != e_invalid_space)
            && ((filter1 == cpuhistory[pos].origin)
                || (filter2 == cpuhistory[pos].origin)
                || (filter3 == cpuhistory[pos].origin)
                || (filter4 == cpuhistory[pos].origin)
                || (filter5 == cpuhistory[pos].origin))) {
            i++;
        }
        pos--;
        if (pos < 0) {
            pos += cpuhistory_buffer_lines;
        }
        /* stop if we hit the starting point */
        /* this is totally possible since the emulation runs each CPU in
            chunks and eventually syncs up. Syncing is more aggressive
            when talking between devices. */
        if (pos == (cpuhistory_i + 1) % cpuhistory_buffer_lines) {
            break;
        }
    }

    return &cpuhistory[pos];
}

cpuhistory_t *mon_cpuhistory_next(cpuhistory_t *current, MEMSPACE filter1, MEMSPACE filter2,
                         MEMSPACE filter3, MEMSPACE filter4, MEMSPACE filter5) {
    cpuhistory_t *wrap = &cpuhistory[cpuhistory_buffer_lines];
    cpuhistory_t *head = &cpuhistory[(cpuhistory_i + 1) % cpuhistory_buffer_lines];
    do {
        current += 1;
        if (current >= wrap) {
            current = &cpuhistory[0];
        } else if (current == head) {
            return NULL;
        }
    /* make sure the record matches */
    } while (!((current->origin != e_invalid_space)
        && ((filter1 == current->origin)
            || (filter2 == current->origin)
            || (filter3 == current->origin)
            || (filter4 == current->origin)
            || (filter5 == current->origin))));

    return current;
}

void mon_cpuhistory(int count, MEMSPACE filter1, MEMSPACE filter2, MEMSPACE filter3,
                    MEMSPACE filter4, MEMSPACE filter5)
{
    uint8_t op, p1, p2, p3 = 0;
    MEMSPACE mem;
    uint16_t loc, addr;
    int hex_mode = 1;
    const char *dis_inst;
    cpuhistory_t *current;
    unsigned opc_size;
    CLOCK cycle;
    char otext[10];

    /* if nothing passed, set the first filter to the default device */
    if ((filter1 == e_invalid_space) &&
        (filter2 == e_invalid_space) &&
        (filter3 == e_invalid_space) &&
        (filter4 == e_invalid_space) &&
        (filter5 == e_invalid_space)) {
        filter1 = default_memspace;
    }

    /* determine the actual maximum records to go through */
    if (count < 1) {
        count = cpuhistory_show_lines;
    }

    current = mon_cpuhistory_seek(count, filter1, filter2, filter3, filter4, filter5);

    /* loop through all entries until we find the number records requested */
    while ((current = mon_cpuhistory_next(current, filter1, filter2, filter3, filter4, filter5))) {
        cycle = current->cycle;
        addr = current->addr;
        op = current->op;
        p1 = current->p1;
        p2 = current->p2;

        mem = current->origin;
        loc = addr_location(addr);

        dis_inst = mon_disassemble_to_string_ex(mem, loc, op, p1, p2, p3, hex_mode, &opc_size);

        strncpy(otext, mon_memspace_string[mem], 4);

        /* Print the disassembled instruction */
        mon_out(".%s:%04x  %-26s A:%02x X:%02x Y:%02x SP:%02x %c%c-%c%c%c%c%c %12"PRIu64"\n",
            otext, loc, dis_inst,
            current->reg_a, current->reg_x,
            current->reg_y, current->reg_sp,
            ((current->reg_st & (1 << 7)) != 0) ? 'N' : '.',
            ((current->reg_st & (1 << 6)) != 0) ? 'V' : '.',
            ((current->reg_st & (1 << 4)) != 0) ? 'B' : '.',
            ((current->reg_st & (1 << 3)) != 0) ? 'D' : '.',
            ((current->reg_st & (1 << 2)) != 0) ? 'I' : '.',
            ((current->reg_st & (1 << 1)) != 0) ? 'Z' : '.',
            ((current->reg_st & (1 << 0)) != 0) ? 'C' : '.',
            cycle
            );
    }
}


/* memmap variables */
static MEMMAP_ELEM *mon_memmap = NULL;
static int mon_memmap_size;
static int mon_memmap_picx;
static int mon_memmap_picy;
static unsigned int mon_memmap_mask;

/* mmzap */
void mon_memmap_zap(void)
{
    memset(mon_memmap, 0, mon_memmap_size * sizeof(MEMMAP_ELEM));
}

/* mmsh */
void mon_memmap_show(int mask, MON_ADDR start_addr, MON_ADDR end_addr)
{
    unsigned int addr;
    MEMMAP_ELEM b;
    const char *line_fmt = NULL;

    if (start_addr == BAD_ADDR) {
        start_addr = 0;
    }

    if (end_addr == BAD_ADDR) {
        end_addr = mon_memmap_mask;
    }

    if (start_addr > end_addr) {
        start_addr = end_addr;
    }

    if (machine_class == VICE_MACHINE_C64DTV) {
        mon_out("  addr: IO  ROM RAM\n");
        line_fmt = "%06x: %c%c%c %c%c%c %c%c%c%s%s%s\n";
    } else {
        mon_out("addr: IO  ROM RAM\n");
        line_fmt = "%04x: %c%c%c %c%c%c %c%c%c%s%s%s\n";
    }

    for (addr = start_addr; addr <= end_addr; ++addr) {
        b = mon_memmap[addr];

        if ((b & mask) == 0) {
            continue;
        }

        mon_out(line_fmt, addr,
                (b & MEMMAP_I_O_R) ? 'r' : '-',
                (b & MEMMAP_I_O_W) ? 'w' : '-',
                (b & MEMMAP_I_O_X) ? 'x' : '-',
                (b & MEMMAP_ROM_R) ? 'r' : '-',
                (b & MEMMAP_ROM_W) ? 'w' : '-',
                (b & MEMMAP_ROM_X) ? 'x' : '-',
                (b & MEMMAP_RAM_R) ? 'r' : '-',
                (b & MEMMAP_RAM_W) ? 'w' : '-',
                (b & MEMMAP_RAM_X) ? 'x' : '-',
                (b & MEMMAP_RAM_R) && (!(b & MEMMAP_REGULAR_READ)) ? " (dummy)" : "",
                (b & MEMMAP_UNINITIALIZED_READ) ? " (uninitialized read)" : "",
                (b & MEMMAP_UNINITIALIZED_EXEC) ? " (uninitialized exec)" : ""
                );
    }
}

/* this is called per memory access, so it should only do whats really needed */
void monitor_memmap_store(unsigned int addr, unsigned int type)
{
    /* uint8_t op = cpuhistory[cpuhistory_i].op; */
    unsigned int last;
#if 0
    static int repeat = 0;

    if (repeat < 4) {
        printf("%s(): addr = $%04x, type = %u\n", __func__, addr, type);
        repeat++;
    }
#endif

    if (memmap_state & MEMMAP_STATE_IN_MONITOR) {
        return;
    }
#if 0 /* FIXME: why would we do this? */
    /* Ignore reg_pc+2 reads on branches & JSR
       and return address read on RTS */
    if (type & (MEMMAP_ROM_R | MEMMAP_RAM_R)
      && (((op & 0x1f) == 0x10) || (op == OP_JSR)
      || ((op == OP_RTS) && ((addr > 0x1ff) || (addr < 0x100))))) {
        return;
    }
#endif
    last = mon_memmap[addr & mon_memmap_mask];
    if (!(last & MEMMAP_RAM_W)) { /* if this address was not written to yet */
        if (type & MEMMAP_REGULAR_READ) { /* and this is a regular, non dummy, read */
            if (type & MEMMAP_RAM_R) {
                type |= MEMMAP_UNINITIALIZED_READ; /* mark as non-initialized read in the history */
            } else if (type & MEMMAP_RAM_X) {
                type |= MEMMAP_UNINITIALIZED_EXEC; /* mark as non-initialized execute in the history */
            }
        }
    }
    mon_memmap[addr & mon_memmap_mask] |= type;
}

void mon_memmap_save(const char *filename, int format)
{
    const char *drvname;
    int i;
    uint8_t mon_memmap_palette[256 * 3];
    uint8_t *memmap_bitmap = NULL;

    switch (format) {
        case 1:
            drvname = "PCX";
            break;
        case 2:
            drvname = "PNG";
            break;
        case 3:
            drvname = "GIF";
            break;
        case 4:
            drvname = "IFF";
            break;
        default:
            drvname = "BMP";
            break;
    }

    /* build palette */
    for (i = 0; i < 256; ++i) {
        mon_memmap_palette[i * 3 + 0] = (i & (MEMMAP_RAM_W)) ? 0x80 : 0 + (i & (MEMMAP_ROM_W)) ? 0x60 : 0 + (i & (MEMMAP_I_O_W)) ? 0x1f : 0;
        mon_memmap_palette[i * 3 + 1] = (i & (MEMMAP_RAM_X)) ? 0x80 : 0 + (i & (MEMMAP_ROM_X)) ? 0x60 : 0 + (i & (MEMMAP_I_O_W | MEMMAP_I_O_X)) ? 0x1f : 0;
        mon_memmap_palette[i * 3 + 2] = (i & (MEMMAP_RAM_R)) ? 0x80 : 0 + (i & (MEMMAP_ROM_R)) ? 0x60 : 0 + (i & (MEMMAP_I_O_X)) ? 0x1f : 0;
    }

    /* make a 8bpp bitmap of the memmap */
    memmap_bitmap = lib_malloc(mon_memmap_size);

    for (i = 0; i < mon_memmap_size; ++i) {
        /* use same bit for I/O R and X */
        memmap_bitmap[i] = (mon_memmap[i] & 0xffu) | ((mon_memmap[i] & MEMMAP_I_O_R) >> 2);
    }

    if (memmap_screenshot_save(drvname, filename, mon_memmap_picx, mon_memmap_picy, memmap_bitmap, mon_memmap_palette)) {
        mon_out("Failed.\n");
    }

    lib_free(memmap_bitmap);
}

void mon_memmap_init(void)
{
    mon_memmap_picx = MEMMAP_PICX;

    if (machine_class == VICE_MACHINE_C64DTV) {
        mon_memmap_picy = 0x2000;
    } else {
        mon_memmap_picy = 0x100;
    }

    mon_memmap_size = mon_memmap_picx * mon_memmap_picy;
    mon_memmap = lib_malloc(mon_memmap_size * sizeof(MEMMAP_ELEM));
    mon_memmap_mask = mon_memmap_size - 1;

    mon_memmap_zap();
}

void mon_memmap_shutdown(void)
{
    lib_free(mon_memmap);
    mon_memmap = NULL;
    if (cpuhistory != NULL) {
        lib_free(cpuhistory);
    }
}


#else /* !FEATURE_CPUMEMHISTORY */

/* stubs */
static void mon_memmap_stub(void)
{
    mon_out("Disabled. configure with --enable-cpuhistory and recompile.\n");
}

void mon_cpuhistory(int count, MEMSPACE filter1, MEMSPACE filter2, MEMSPACE filter3,
                    MEMSPACE filter4, MEMSPACE filter5)
{
    mon_memmap_stub();
}

void mon_memmap_zap(void)
{
    mon_memmap_stub();
}

void mon_memmap_show(int mask, MON_ADDR start_addr, MON_ADDR end_addr)
{
    mon_memmap_stub();
}

void mon_memmap_save(const char* filename, int format)
{
    mon_memmap_stub();
}

void mon_memmap_init(void)
{
}

void mon_memmap_shutdown(void)
{
}

#endif
