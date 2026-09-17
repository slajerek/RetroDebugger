/*
 * profdos.c - Professional DOS emulation.
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
#include <string.h>

#include "cmdline.h"
#include "drive.h"
#include "drivemem.h"
#include "drivetypes.h"
#include "lib.h"
#include "log.h"
#include "profdos.h"
#include "resources.h"
#include "util.h"

/* #define DEBUG_PROFDOS */

#ifdef DEBUG_PROFDOS
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

/*
 * - extra 8k RAM at $4000
 * - extra 8k ROM at $6000
 * - 32k DOS ROM at $8000
 *
 * x64sc -drive8type 1571 -drive8ram4000 -drive8profdos -profdos1571 ROM -kernal Kernal -dos1571 1571 -parallel8 "1" -userportdevice "21" foo.d64
 */

/* FIXME: should be dynamically allocated */
#define PROFDOS_ROM_SIZE 0x2000
static uint8_t profdos_1571_rom[PROFDOS_ROM_SIZE];

static unsigned int profdos_al[NUM_DISK_UNITS];


int profdos_load_1571(const char *name)
{
    DBG(("profdos_load_1571 '%s'", name));
    if (util_check_null_string(name)) {
        return 0;
    }

    if (util_file_load(name, profdos_1571_rom,
                       PROFDOS_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    DBG(("profdos_load_1571 ok"));
    return 0;
}

/* $6000-$6fff extra 8k "decoder" rom (first 4k) */
static uint8_t profdos_read(diskunit_context_t *drv, uint16_t addr)
{
/*    DBG(("profdos_read %04x", addr)); */
    return (drv->cpu->cpu_last_data = profdos_1571_rom[addr & 0x1fff]);
}

/* $7000-$7fff extra 8k "decoder" rom (second 4k) */
static uint8_t profdos_read2(diskunit_context_t *drv, uint16_t addr)
{
    /*DBG(("profdos2_read %04x", addr));*/
    if (addr >= 0x7000) {
        if (!(addr & 0x0800)) {
            addr = (uint16_t)((addr & 0xff0f) | (profdos_al[drv->mynumber] << 4));
        } else {
            addr = (uint16_t)((addr & 0xff00)
                          | (profdos_al[drv->mynumber] << 4) | ((addr >> 4) & 15));
        }

        profdos_al[drv->mynumber] = addr & 15;
    }

    return (drv->cpu->cpu_last_data = profdos_1571_rom[addr & 0x1fff]);
}

/* CAUTION: gets called no matter if profdos is enabled or not */
void profdos_mem_init(struct diskunit_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    DBG(("profdos_mem_init (profdos:%d)", drv->profdos));
    if (!drv->profdos) {
        DBG(("profdos_mem_init skipped!"));
        return;
    }

    /* Setup additional profdos rom */
    switch (type) {
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        drivemem_set_func(cpud, 0x60, 0x70, profdos_read, NULL, NULL, profdos_1571_rom, 0x60006ffd);
        drivemem_set_func(cpud, 0x70, 0x80, profdos_read2, NULL, NULL, NULL, 0);
        break;
    default:
        break;
    }
}

/* CAUTION: gets called no matter if profdos is enabled or not */
void profdos_init(diskunit_context_t *drv)
{
    DBG(("profdos_init (profdos:%d)", drv->profdos));
    if (!drv->profdos) {
        DBG(("profdos_init skipped!"));
        return;
    }
}

/* CAUTION: gets called no matter if profdos is enabled or not */
void profdos_reset(diskunit_context_t *drv)
{
    DBG(("profdos_reset (profdos:%d)", drv->profdos));
    if (!drv->profdos) {
        DBG(("profdos_reset skipped!"));
        return;
    }
    profdos_al[drv->mynumber] = 0;
}
