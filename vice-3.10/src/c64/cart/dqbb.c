/*
 * dqbb.c - Double Quick Brown Box emulation.
 *
 * Written by
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOT1_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT1_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "mem.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "dqbb.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    (Double) Quick Brown Box

    There have been 3 incarnations of this cartridge, and each was available in
    various versions with different amount of memory:

    1st: "QBB" (Quick Brown Box) - C64 only, 8kb in the smallest variant
    2nd: "QBB-B" adds battery backup
    3rd: "DQBB" (Double Quick Brown Box) - adds C128 support, 16kb

    The last incarnation has a switch to enable either C64 or C128 mode (see below)

    Write-Only register at $de00:

    bit 2:   controls the /GAME line:
             1 = $A000-$BFFF mapped in (/GAME low)
             0 = $A000-$BFFF not mapped in (/GAME high)
    bit 4:   1 = read/write, 0 = read-only.
    bit 6:   controls the /EXROM line: (*)
             1 = /EXROM low
             0 = /EXROM low
    bit 7:   1 = cart off, 0 = cart on. (register remains active)

    (*) The switch holds /EXROM low when in C64 position, so this bit can be
        used to force the C128 into C64, when the switch is in C128 position.

    The remaining 4 bits are used for banking, consequently the largest available
    variant had 256k RAM. Which variants actually existed for real remains unknown,
    ads frequently mention all possible (16/32/64/128/256k) variants however.

    C128 mode (DQBB only):

    - ROM mapped to $8000-BFFF

    A hardware RESET or power up clears all bits, so in C64 mode it will always
    start with 8k mapped.

    The current emulation has the register mirrorred through the
    range of $de00-$deff
*/

/* #define DBGDQBB */

#ifdef DBGDQBB
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

static log_t dqbb_log = LOG_DEFAULT; /*!< the log output for the dqbb_log */

/* DQBB register bits */
static int dqbb_game;
static int dqbb_readwrite;
static int dqbb_off;
static int dqbb_exrom;
static int dqbb_bank;

/* DQBB image.  */
static uint8_t *dqbb_ram = NULL;

static int dqbb_activate(void);
static int dqbb_deactivate(void);
static void dqbb_change_config(void);

/* Flag: Do we enable the DQBB?  */
static int dqbb_enabled = 0;

/* Filename of the DQBB image.  */
static char *dqbb_filename = NULL;

#define DQBB_RAM_SIZE   (0x400 * 256)   /* max. size */

static int dqbb_size; /* actual size */
static int dqbb_bank_mask;

static int dqbb_mode_switch;    /* 0: C128, 1: C64 */

static int reg_value = 0;

static int dqbb_write_image = 0;

/* ------------------------------------------------------------------------- */

static uint8_t dqbb_io1_peek(uint16_t addr);
static void dqbb_io1_store(uint16_t addr, uint8_t byte);
static int dqbb_dump(void);

static io_source_t dqbb_io1_device = {
    CARTRIDGE_NAME_DQBB,  /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "DQBB",               /* resource to set to '0' */
    0xde00, 0xdeff, 0xff, /* range for the device, address is ignored, reg:$de00, mirrors: $de01-$deff */
    0,                    /* read is never valid, device is write only */
    dqbb_io1_store,       /* store function */
    NULL,                 /* NO poke function */
    NULL,                 /* NO read function */
    dqbb_io1_peek,        /* peek function */
    dqbb_dump,            /* device state information dump function */
    CARTRIDGE_DQBB,       /* cartridge ID */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0,                    /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE        /* NO mirroring */
};

static io_source_list_t *dqbb_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_DQBB, 1, 1, &dqbb_io1_device, NULL, CARTRIDGE_DQBB
};

/* ------------------------------------------------------------------------- */

int dqbb_cart_enabled(void)
{
    return dqbb_enabled;
}

static void dqbb_change_config(void)
{
    int mode = CMODE_RAM;

    if (dqbb_enabled) {
        if (!dqbb_off) {
            /* The mode switch pulls exrom when in C64 position */
            if (dqbb_mode_switch || dqbb_exrom) {
                if (dqbb_game) {
                    mode = CMODE_16KGAME;
                } else {
                    mode = CMODE_8KGAME;
                }
            } else {
                if (dqbb_game) {
                    /* switch is in C128 position, and /GAME bit is set */
                    mode = CMODE_ULTIMAX;
                }
            }
        }
    }

    cart_config_changed_slot1(mode, mode, CMODE_READ);
    DBG(("dqbb_change_config: 0x%02x (%s) mode:%d enable:%d off:%d game:%d exrom:%d",
         (unsigned int)mode, cart_config_string(mode), dqbb_mode_switch, dqbb_enabled,
         dqbb_off, dqbb_game, dqbb_exrom));
}

static void dqbb_io1_store(uint16_t addr, uint8_t byte)
{
    dqbb_game = (byte >> 2) & 1;
    dqbb_readwrite = (byte >> 4) & 1;
    dqbb_exrom = (byte >> 6) & 1;
    dqbb_off = (byte >> 7) & 1;
    dqbb_bank = (byte & 3) | ((byte >> 1) & 4) | ((byte >> 2) & 8);
    dqbb_bank &= dqbb_bank_mask;
    DBG(("dqbb_io1_store reg: 0x%02x enabled:%d r/w:%d bank:%d game:%d exrom:%d",
         byte, dqbb_off, dqbb_readwrite, dqbb_bank, dqbb_game, dqbb_exrom));
    dqbb_change_config();
    reg_value = byte;
}

static uint8_t dqbb_io1_peek(uint16_t addr)
{
    return reg_value;
}

static int dqbb_dump(void)
{
    mon_out("$A000-$BFFF RAM: %s, cart status: %s\n",
            (reg_value & 4) ? "mapped in" : "not mapped in",
            (reg_value & 0x80) ? ((reg_value & 0x10) ? "read/write" : "read-only") : "disabled");
    mon_out("current bank: %d of %d\n", dqbb_bank, dqbb_size / 16);
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

void dqbb_powerup(void)
{
    DBG(("dqbb_powerup"));
    if ((dqbb_filename != NULL) && (*dqbb_filename != 0)) {
        /* do not init ram if a file is used for ram content (like battery backup) */
        return;
    }
    if (dqbb_ram) {
        DBG(("dqbb_powerup ram clear"));
        ram_init_with_pattern(dqbb_ram, DQBB_RAM_SIZE, &ramparam);
    }
}

void dqbb_shutdown(void)
{
    if (dqbb_ram) {
        lib_free(dqbb_ram);
    }
}

static int dqbb_activate(void)
{
    DBG(("dqbb_activate"));
    lib_free(dqbb_ram);
    dqbb_ram = lib_malloc(DQBB_RAM_SIZE);
    ram_init_with_pattern(dqbb_ram, DQBB_RAM_SIZE, &ramparam);

    if (dqbb_log == LOG_DEFAULT) {
        dqbb_log = log_open("DQBB");
    }

    if (!util_check_null_string(dqbb_filename)) {
        if (util_file_load(dqbb_filename, dqbb_ram, dqbb_size * 0x400, UTIL_FILE_LOAD_RAW) < 0) {
            /* only create a new file if no file exists, so we dont accidently overwrite any files */
            if (!util_file_exists(dqbb_filename)) {
                if (util_file_save(dqbb_filename, dqbb_ram, dqbb_size * 0x400) < 0) {
                    return -1;
                }
                log_message(dqbb_log, "created '%s'", dqbb_filename);
            }
        } else {
            log_message(dqbb_log, "loaded '%s'", dqbb_filename);
        }
    }
    return 0;
}

static int dqbb_deactivate(void)
{
    DBG(("dqbb_deactivate"));
    if (dqbb_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(dqbb_filename)) {
        if (dqbb_write_image) {
            if (util_file_save(dqbb_filename, dqbb_ram, dqbb_size * 0x400) < 0) {
                return -1;
            }
        }
    }

    lib_free(dqbb_ram);
    dqbb_ram = NULL;

    export_remove(&export_res);

    return 0;
}

static int set_dqbb_enabled(int value, void *param)
{
    int val = value ? 1 : 0;
    DBG(("set_dqbb_enabled: val:%d", val));

    if ((!val) && (dqbb_enabled)) {
        cart_power_off();
        if (dqbb_deactivate() < 0) {
            return -1;
        }
        io_source_unregister(dqbb_io1_list_item);
        dqbb_io1_list_item = NULL;
        dqbb_enabled = 0;
        dqbb_reset();
        dqbb_change_config();
    } else if ((val) && (!dqbb_enabled)) {
        cart_power_off();
        if (export_add(&export_res) < 0) {
            return -1;
        }
        if (dqbb_activate() < 0) {
            return -1;
        }
        dqbb_io1_list_item = io_source_register(&dqbb_io1_device);
        dqbb_enabled = 1;
        dqbb_reset();
        dqbb_change_config();
    }
    DBG(("set_dqbb_enabled: dqbb_enabled:%d", dqbb_enabled));
    return 0;
}

static int set_dqbb_filename(const char *name, void *param)
{
    if (dqbb_filename != NULL && name != NULL && strcmp(name, dqbb_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (dqbb_enabled) {
        dqbb_deactivate();
        util_string_set(&dqbb_filename, name);
        dqbb_activate();
    } else {
        util_string_set(&dqbb_filename, name);
    }

    return 0;
}

static int set_dqbb_image_write(int val, void *param)
{
    dqbb_write_image = val ? 1 : 0;

    return 0;
}

static int set_dqbb_size(int val, void *param)
{
    DBG(("set_dqbb_size: val:%d", val));
    if (val != dqbb_size) {
        if ((val == 16) ||
            (val == 32) ||
            (val == 64) ||
            (val == 128) ||
            (val == 256)) {
            int was_enabled = dqbb_enabled;
            dqbb_deactivate();
            dqbb_size = val;
            dqbb_bank_mask = (val == 0) ? 0 : (val / 16) - 1;
            if (was_enabled) {
                dqbb_activate();
            }
            DBG(("set_dqbb_size size: %d mask: 0x%02x", dqbb_size, (unsigned int)dqbb_bank_mask));
        } else {
            DBG(("set_dqbb_size: (error) dqbb_size:%d", dqbb_size));
            return -1;
        }
    }
    DBG(("set_dqbb_size: (ok) dqbb_size:%d", dqbb_size));
    return 0;
}

static int set_dqbb_mode(int val, void *param)
{
    dqbb_mode_switch = (val == DQBB_MODE_C128) ? DQBB_MODE_C128 : DQBB_MODE_C64;
    DBG(("set_dqbb_mode: val:%d dqbb_mode_switch: %s", val, dqbb_mode_switch == DQBB_MODE_C64 ? "C64" : "C128"));
    if (dqbb_enabled) {
        dqbb_change_config();
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

static const resource_string_t resources_string[] = {
    { "DQBBfilename", "", RES_EVENT_NO, NULL,
      &dqbb_filename, set_dqbb_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "DQBB", 0, RES_EVENT_SAME, NULL,
      &dqbb_enabled, set_dqbb_enabled, NULL },
    { "DQBBSize", 16, RES_EVENT_SAME, NULL,
      &dqbb_size, set_dqbb_size, NULL },
    { "DQBBMode", DQBB_MODE_C64, RES_EVENT_SAME, NULL,
      &dqbb_mode_switch, set_dqbb_mode, NULL },
    { "DQBBImageWrite", 0, RES_EVENT_NO, NULL,
      &dqbb_write_image, set_dqbb_image_write, NULL },
    RESOURCE_INT_LIST_END
};

int dqbb_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void dqbb_resources_shutdown(void)
{
    lib_free(dqbb_filename);
    dqbb_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-dqbb", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DQBB", (resource_value_t)1,
      NULL, "Enable Double Quick Brown Box" },
    { "+dqbb", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DQBB", (resource_value_t)0,
      NULL, "Disable Double Quick Brown Box" },
    { "-dqbbsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DQBBSize", NULL,
      "<Size>", "Set Double Quick Brown Box RAM size (16/32/64/128/256kiB)" },
    { "-dqbbmode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DQBBMode", NULL,
      "<Mode>", "Set Double Quick Brown Box mode switch (0: C128, 1:C64)" },
    { "-dqbbimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DQBBfilename", NULL,
      "<Name>", "Specify Double Quick Brown Box filename" },
    { "-dqbbimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DQBBImageWrite", (resource_value_t)1,
      NULL, "Allow writing to DQBB image" },
    { "+dqbbimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DQBBImageWrite", (resource_value_t)0,
      NULL, "Do not write to DQBB image" },
    CMDLINE_LIST_END
};

int dqbb_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

const char *dqbb_get_file_name(void)
{
    return dqbb_filename;
}

void dqbb_reset(void)
{
    dqbb_game = 0;
    dqbb_readwrite = 0;
    dqbb_off = 0;
    dqbb_bank = 0;
    dqbb_exrom = 0;

    if (dqbb_enabled) {
        dqbb_change_config();
    }
}

void dqbb_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    /* FIXME: this doesn't incorporate the banking, nor C128 */
#if 0
    switch (addr & 0xf000) {
        case 0xb000:
        case 0xa000:
        case 0x9000:
        case 0x8000:
            *base = dqbb_ram - 0x8000;
            *start = 0x8000;
            *limit = 0xbffd;
            return;
        default:
            break;
    }
#endif
    *base = NULL;
    *start = 0;
    *limit = 0;
}

void dqbb_init_config(void)
{
    dqbb_reset();
}

void dqbb_config_setup(uint8_t *rawcart)
{
    memcpy(dqbb_ram, rawcart, DQBB_RAM_SIZE);
}

/* ------------------------------------------------------------------------- */

void dqbb_detach(void)
{
    resources_set_int("DQBB", 0);
}

int dqbb_enable(void)
{
    if (resources_set_int("DQBB", 1) < 0) {
        return -1;
    }
    return 0;
}


/** \brief  Disable the cart
 *
 * Does the same as dqbb_detach(), but required for symmetry I suppose.
 *
 * \return  0 on success, -1 on failure
 */
int dqbb_disable(void)
{
    if (resources_set_int("DQBB", 0) < 0) {
        return -1;
    }
    return 0;
}


int dqbb_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, dqbb_size * 0x400, UTIL_FILE_LOAD_RAW) < 0) {
        return -1;
    }
    util_string_set(&dqbb_filename, filename);
    return dqbb_enable();
}

int dqbb_bin_save(const char *filename)
{
    if (dqbb_ram == NULL) {
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    if (util_file_save(filename, dqbb_ram, dqbb_size * 0x400) < 0) {
        return -1;
    }
    return 0;
}

int dqbb_flush_image(void)
{
    return dqbb_bin_save(dqbb_filename);
}

/* ------------------------------------------------------------------------- */

uint8_t dqbb_roml_read(uint16_t addr)
{
    return dqbb_ram[(addr & 0x1fff) + (dqbb_bank * 0x4000)];
}

void dqbb_roml_store(uint16_t addr, uint8_t byte)
{
    if (dqbb_readwrite) {
        dqbb_ram[(addr & 0x1fff) + (dqbb_bank * 0x4000)] = byte;
    }
    mem_store_without_romlh(addr, byte);
}

uint8_t dqbb_romh_read(uint16_t addr)
{
    return dqbb_ram[(addr & 0x1fff) + 0x2000 + (dqbb_bank * 0x4000)];
}

void dqbb_romh_store(uint16_t addr, uint8_t byte)
{
    if (dqbb_readwrite) {
        dqbb_ram[(addr & 0x1fff) + 0x2000 + (dqbb_bank * 0x4000)] = byte;
    }
    mem_store_without_romlh(addr, byte);
}

int dqbb_peek_mem(uint16_t addr, uint8_t *value)
{
    if ((addr >= 0x8000) && (addr <= 0x9fff)) {
        *value = dqbb_ram[(addr & 0x1fff) + (dqbb_bank * 0x4000)];
        return CART_READ_VALID;
    } else if ((addr >= 0xa000) && (addr <= 0xbfff)) {
        *value = dqbb_ram[(addr & 0x1fff) + 0x2000 + (dqbb_bank * 0x4000)];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}


/* ------------------------------------------------------------------------- */

/* In C128 mode the RAM is mapped to $8000-$BFFF */
int dqbb_c128_read(uint16_t addr, uint8_t *value)
{
    if ((addr >= 0x8000) && (addr <= 0xbfff)) {
        *value = dqbb_ram[(addr & 0x3fff) + (dqbb_bank * 0x4000)];
        return CART_READ_VALID; /* read was valid */
    }
    return CART_READ_THROUGH;
}

int dqbb_c128_store(uint16_t addr, uint8_t value)
{
    if ((addr >= 0x8000) && (addr <= 0xbfff)) {
        if (dqbb_readwrite) {
            dqbb_ram[(addr & 0x3fff) + (dqbb_bank * 0x4000)] = value;
        }
        return 1; /* write was valid */
    }
    return 0; /* write was invalid */
}

/* ---------------------------------------------------------------------*/

/* CARTDQBB snapshot module format:

   type  | name       | description
   --------------------------------
   BYTE  | enabled    | cartridge enabled flag
   BYTE  | read write | read/write flag
   BYTE  | a000 map   | $A000 mapped flag (GAME line)
   BYTE  | off        | dqbb off flag
   BYTE  | register   | register
   BYTE  | exrom      | state of EXROM line
   BYTE  | size       | RAM size in kb
   BYTE  | bank       | selected ram bank
   ARRAY | RAM        | <size> BYTES of RAM data
 */

static const char snap_module_name[] = "CARTDQBB";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int dqbb_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)dqbb_enabled) < 0)
        || (SMW_B(m, (uint8_t)dqbb_readwrite) < 0)
        || (SMW_B(m, (uint8_t)dqbb_game) < 0)
        || (SMW_B(m, (uint8_t)dqbb_off) < 0)
        || (SMW_B(m, (uint8_t)reg_value) < 0)
        || (SMW_B(m, (uint8_t)dqbb_exrom) < 0)
        || (SMW_B(m, (uint8_t)dqbb_size) < 0)
        || (SMW_B(m, (uint8_t)dqbb_bank) < 0)
        || (SMW_B(m, (uint8_t)dqbb_mode_switch) < 0)
        || (SMW_BA(m, dqbb_ram, dqbb_size * 0x400) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int dqbb_snapshot_read_module(snapshot_t *s)
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
        snapshot_module_close(m);
        return -1;
    }

    dqbb_ram = lib_malloc(DQBB_RAM_SIZE);

    if (0
        || (SMR_B_INT(m, &dqbb_enabled) < 0)
        || (SMR_B_INT(m, &dqbb_readwrite) < 0)
        || (SMR_B_INT(m, &dqbb_game) < 0)
        || (SMR_B_INT(m, &dqbb_off) < 0)
        || (SMR_B_INT(m, &reg_value) < 0)
        || (SMR_B_INT(m, &dqbb_exrom) < 0)
        || (SMR_B_INT(m, &dqbb_size) < 0)
        || (SMR_B_INT(m, &dqbb_bank) < 0)
        || (SMR_B_INT(m, &dqbb_mode_switch) < 0)
        || (SMR_BA(m, dqbb_ram, dqbb_size * 0x400) < 0)) {
        snapshot_module_close(m);
        lib_free(dqbb_ram);
        dqbb_ram = NULL;
        return -1;
    }

    snapshot_module_close(m);

    /* dqbb_filetype = 0; */
    dqbb_write_image = 0;
    dqbb_enabled = 1;

    /* FIXME: ugly code duplication to avoid cart_config_changed calls */
    dqbb_io1_list_item = io_source_register(&dqbb_io1_device);

    if (export_add(&export_res) < 0) {
        lib_free(dqbb_ram);
        dqbb_ram = NULL;
        io_source_unregister(dqbb_io1_list_item);
        dqbb_io1_list_item = NULL;
        dqbb_enabled = 0;
        return -1;
    }

    return 0;
}
