/*
 * memiec.c - IEC drive memory.
 *
 * Written by
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

#include "ciad.h"
#include "drivemem.h"
#include "drivetypes.h"
#include "lib.h"
#include "memiec.h"
#include "types.h"
#include "via1d1541.h"
#include "viad.h"
#include "wd1770.h"
#include "via4000.h"
#include "pc8477.h"

#include "SYS_Types.h"
#include "ViceWrapper.h"

BYTE drive_read_rom(drive_context_t *drv, WORD address)
{
    return drv->drive->rom[address & 0x7fff];
}

BYTE drive_read_rom_ds1216(drive_context_t *drv, WORD address)
{
    return ds1216e_read(drv->drive->ds1216, address, drv->drive->rom[address & 0x7fff]);
}

static BYTE drive_read_ram(drive_context_t *drv, WORD address)
{
	c64d_mark_disk_cell_read(address);

    return drv->drive->drive_ram[address];
}

static void drive_store_ram(drive_context_t *drv, WORD address, BYTE value)
{
	c64d_mark_disk_cell_write(address, value);

	drv->drive->drive_ram[address] = value;
}

static BYTE drive_read_1541ram(drive_context_t *drv, WORD address)
{
	c64d_mark_disk_cell_read(address);
	
    return drv->drive->drive_ram[address & 0x7ff];
}

static void drive_store_1541ram(drive_context_t *drv, WORD address, BYTE value)
{
	c64d_mark_disk_cell_write(address, value);
	
	drv->drive->drive_ram[address & 0x7ff] = value;
}

static BYTE drive_read_zero(drive_context_t *drv, WORD address)
{
	c64d_mark_disk_cell_read(address);
	
    return drv->drive->drive_ram[address & 0xff];
}

static void drive_store_zero(drive_context_t *drv, WORD address, BYTE value)
{
	c64d_mark_disk_cell_write(address, value);

	drv->drive->drive_ram[address & 0xff] = value;
}

/* ------------------------------------------------------------------------- */

void memiec_init(struct drive_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    switch (type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        drv->cpu->pageone = drv->drive->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive->drive_ram, 0x000007fd);
        drivemem_set_func(cpud, 0x01, 0x08, drive_read_1541ram, drive_store_1541ram, NULL, &drv->drive->drive_ram[0x0100], 0x000007fd);
        drivemem_set_func(cpud, 0x18, 0x1c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store, via2d_peek, NULL, 0);
        if (drv->drive->drive_ram2_enabled) {
            drivemem_set_func(cpud, 0x20, 0x40, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x2000], 0x20003ffd);
        } else {
            drivemem_set_func(cpud, 0x20, 0x28, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive->drive_ram, 0x200027fd);
            drivemem_set_func(cpud, 0x38, 0x3c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
            drivemem_set_func(cpud, 0x3c, 0x40, via2d_read, via2d_store, via2d_peek, NULL, 0);
        }
        if (drv->drive->drive_ram4_enabled) {
            drivemem_set_func(cpud, 0x40, 0x60, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x4000], 0x40005ffd);
        } else {
            drivemem_set_func(cpud, 0x40, 0x48, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive->drive_ram, 0x400047fd);
            drivemem_set_func(cpud, 0x58, 0x5c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
            drivemem_set_func(cpud, 0x5c, 0x60, via2d_read, via2d_store, via2d_peek, NULL, 0);
        }
        if (drv->drive->drive_ram6_enabled) {
            drivemem_set_func(cpud, 0x60, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x6000], 0x60007ffd);
        } else {
            drivemem_set_func(cpud, 0x60, 0x68, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive->drive_ram, 0x600067fd);
            drivemem_set_func(cpud, 0x78, 0x7c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
            drivemem_set_func(cpud, 0x7c, 0x80, via2d_read, via2d_store, via2d_peek, NULL, 0);
        }
        if (drv->drive->drive_ram8_enabled) {
            drivemem_set_func(cpud, 0x80, 0xa0, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x8000], 0x80009ffd);
        } else {
            drivemem_set_func(cpud, 0x80, 0xa0, drive_read_rom, NULL, NULL, drv->drive->trap_rom, 0x80009ffd);
        }
        if (drv->drive->drive_rama_enabled) {
            drivemem_set_func(cpud, 0xa0, 0xc0, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0xa000], 0xa000bffd);
        } else {
            drivemem_set_func(cpud, 0xa0, 0xc0, drive_read_rom, NULL, NULL, &drv->drive->trap_rom[0x2000], 0xa000bffd);
        }
        drivemem_set_func(cpud, 0xc0, 0x100, drive_read_rom, NULL, NULL, &drv->drive->trap_rom[0x4000], 0xc000fffd);
        break;
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        drv->cpu->pageone = drv->drive->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive->drive_ram, 0x000007fd);
        drivemem_set_func(cpud, 0x01, 0x08, drive_read_1541ram, drive_store_1541ram, NULL, &drv->drive->drive_ram[0x0100], 0x000007fd);
        drivemem_set_func(cpud, 0x08, 0x10, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive->drive_ram, 0x08000ffd);
        drivemem_set_func(cpud, 0x18, 0x1c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store, via2d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x20, 0x30, wd1770d_read, wd1770d_store, wd1770d_peek, NULL, 0);
        if (drv->drive->drive_ram4_enabled) {
            drivemem_set_func(cpud, 0x40, 0x48, cia1571_read, cia1571_store, cia1571_peek, NULL, 0);
            drivemem_set_func(cpud, 0x48, 0x60, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x4000], 0x48005ffd);
        } else {
            drivemem_set_func(cpud, 0x40, 0x60, cia1571_read, cia1571_store, cia1571_peek, NULL, 0);
        }
        if (drv->drive->drive_ram6_enabled) {
            drivemem_set_func(cpud, 0x60, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x6000], 0x60007ffd);
        } else {
            drivemem_set_func(cpud, 0x60, 0x80, cia1571_read, cia1571_store, cia1571_peek, NULL, 0);
        }
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->drive->trap_rom, 0x8000fffd);
        break;
    case DRIVE_TYPE_1581:
        drv->cpu->pageone = drv->drive->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive->drive_ram, 0x00001ffd);
        drivemem_set_func(cpud, 0x01, 0x20, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x0100], 0x00001ffd);
        drivemem_set_func(cpud, 0x40, 0x60, cia1581_read, cia1581_store, cia1581_peek, NULL, 0);
        drivemem_set_func(cpud, 0x60, 0x80, wd1770d_read, wd1770d_store, wd1770d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->drive->trap_rom, 0x8000fffd);
        break;
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
        drv->cpu->pageone = drv->drive->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive->drive_ram, 0x00003ffd);
        drivemem_set_func(cpud, 0x01, 0x40, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x0100], 0x00003ffd);
        drivemem_set_func(cpud, 0x40, 0x4c, via4000_read, via4000_store, via4000_peek, NULL, 0);
        drivemem_set_func(cpud, 0x4e, 0x50, pc8477d_read, pc8477d_store, pc8477d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x50, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive->drive_ram[0x5000], 0x50007ffd);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->drive->trap_rom, 0x8000fffd);
        /* for performance reasons it's only this page */
        drivemem_set_func(cpud, 0xf0, 0xf1, drive_read_rom_ds1216, NULL, NULL, &drv->drive->trap_rom[0x7000], 0x8000fffd);
        break;
    default:
        return;
    }
}

/// c64d
/// 1541 peek
uint8 c64d_peek_mem_drive_internal(drive_context_t *drv, uint16 addr)
{
	//#define LOAD(a)           (drv->cpud->read_func[(a) >> 8](drv, (WORD)(a)))
	//#define LOAD_ZERO(a)      (drv->cpud->read_func[0](drv, (WORD)(a)))
	//#define LOAD_ADDR(a)      (LOAD(a) | (LOAD((a) + 1) << 8))
	//#define LOAD_ZERO_ADDR(a) (LOAD_ZERO(a) | (LOAD_ZERO((a) + 1) << 8))
	//#define STORE(a, b)       (drv->cpud->store_func[(a) >> 8](drv, (WORD)(a), \
	//(BYTE)(b)))
	//#define STORE_ZERO(a, b)  (drv->cpud->store_func[0](drv, (WORD)(a), \
	//(BYTE)(b)))
	
	if (addr < 0x0800)
	{
		return drv->drive->drive_ram[addr & 0x7ff];
	}
	
	if (addr >= 0x0800 && addr < 0x1800)
	{
		return drive_peek_free(drv, addr);
	}
	
	if (addr >= 0x1800 && addr < 0x1c00)
	{
		return c64d_via1d1541_peek(drv, addr);
	}
	
	if (addr >= 0x1c00 && addr < 0x2000)
	{
		return c64d_via2d_peek(drv, addr);
	}
	
	if (addr >= 0x8000)
	{
		return drv->drive->rom[addr & 0x7fff]; //this marks cell read: drive_read_rom(drv, addr);
	}
	
	return drive_peek_free(drv, addr); //drv->cpud->read_func[(addr) >> 8](drv, (WORD)addr);
}

void c64d_peek_memory_drive_internal(drive_context_t *drv, BYTE *memoryBuffer, uint16 addrStart, uint16 addrEnd)
{
	uint16 addr;
	uint8 *bufPtr = memoryBuffer + addrStart;
	for (addr = addrStart; addr < addrEnd; addr++)
	{
		*bufPtr++ = c64d_peek_mem_drive_internal(drv, addr);
	}
}

void c64d_copy_ram_memory_drive_internal(drive_context_t *drv, BYTE *memoryBuffer, uint16 addrStart, uint16 addrEnd)
{
	uint16 addr;
	uint8 *bufPtr = memoryBuffer + addrStart;
	for (addr = addrStart; addr < addrEnd; addr++)
	{
		*bufPtr++ = drv->drive->drive_ram[addr];
	}
}

void c64d_peek_whole_map_drive_internal(drive_context_t *drv, uint8 *memoryBuffer)
{
	uint16 addr;
	uint8 *bufPtr = memoryBuffer;
	for (addr = 0; addr < 0x0800; addr++)
	{
		*bufPtr++ = drv->drive->drive_ram[addr];
	}
	
	//	if (addr >= 0x0800 && addr < 0x1800)
	//		return drive_peek_free(drv, addr);
	
	bufPtr = memoryBuffer + 0x1800;
	for (addr = 0x1800; addr < 0x1c00; addr++)
	{
		*bufPtr++ = c64d_via1d1541_peek(drv, addr);
	}
	
	bufPtr = memoryBuffer + 0x1c00;
	for (addr = 0x1c00; addr < 0x2000; addr++)
	{
		*bufPtr++ = c64d_via2d_peek(drv, addr);
	}
}

uint8 c64d_mem_ram_read_drive_internal(drive_context_t *drv, uint16 addr)
{
	if (addr < 0x0800)
	{
		return drv->drive->drive_ram[addr & 0x7ff];
	}
	
	//	if (addr >= 0x0800) // && addr < 0x1800)
	{
		return drive_peek_free(drv, addr);
	}
}

void c64d_copy_mem_ram_drive_internal(drive_context_t *drv, uint8 *memoryBuffer)
{
	uint16 addr;
	uint8 *bufPtr = memoryBuffer;
	for (addr = 0; addr < 0x0800; addr++)
	{
		*bufPtr++ = drv->drive->drive_ram[addr];
	}
}

void c64d_mem_ram_write_drive_internal(drive_context_t *drv, uint16 addr, uint8 value)
{
	drv->drive->drive_ram[addr & 0x7ff] = value;
}


///

