
/*
 * megabyter.c - Cartridge handling of the megabyter cart.
 *
 * Written by
 *  Chester Kollschen <mail@chesterkollschen.com>
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

#include "archdep.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "megabyter.h"
#include "export.h"
#include "flash800.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "util.h"
#include "sysfile.h"

/*
    Protovision "Megabyter"

    - 1MiB Flash ROM (29F800CB)

    ROM is always banked to ROML, ie $8000

    - two write-only Registers in IO1

    $de00 - selects ROM bank
        bit 7       unused
        bit 0-6     bank 0-127

    $de02
        bit 7       LED status (1: enabled)
        bit 2-6     unused
        bit 1       EXROM (0: low, 1: high)
        bit 0       GAME  (0: high, 1: low)
*/

/* #define DEBUG_MEGABYTER */

#ifdef DEBUG_MEGABYTER
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

#define MEGABYTER_BANK_BITS     7
#define MEGABYTER_NUM_BANKS     (1 << (MEGABYTER_BANK_BITS))
#define MEGABYTER_BANK_MASK     ((MEGABYTER_NUM_BANKS) -1)
#define MEGABYTER_BANK_SIZE     0x2000
#define MEGABYTER_ROM_SIZE      (MEGABYTER_NUM_BANKS * MEGABYTER_BANK_SIZE)

/* the 29F800CB statemachine */
static flash800_context_t *megabyter_state = NULL;

/* writing back to crt enabled */
static int megabyter_crt_write;

/* optimizing crt enabled */
static int megabyter_crt_optimize;

/* backup of the registers */
static uint8_t megabyter_register_00, megabyter_register_02;

/* decoding table of the modes */
static const uint8_t megabyter_memconfig[4] = {
    CMODE_8KGAME,   /* exrom=0, game=1 */
    CMODE_16KGAME,  /* exrom=0, game=0 */
    CMODE_RAM,      /* exrom=1, game=1 */
    CMODE_ULTIMAX   /* exrom=1, game=0 */
};

/* filename when attached */
static char *megabyter_filename = NULL;
static int megabyter_filetype = 0;

/* ---------------------------------------------------------------------*/
static io_source_list_t *megabyter_io1_list_item = NULL;

static void megabyter_io1_store(uint16_t addr, uint8_t value);
static uint8_t megabyter_io1_peek(uint16_t addr);
static int megabyter_io1_dump(void);

static io_source_t megabyter_io1_device = {
    CARTRIDGE_NAME_MEGABYTER,          /* name of the device */
    IO_DETACH_CART,                    /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,             /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,              /* range for the device */
    0,                                 /* read is never valid, device is write only */
    megabyter_io1_store,               /* store function */
    NULL,                              /* NO poke function */
    NULL,                              /* NO read function */
    megabyter_io1_peek,                /* peek function */
    megabyter_io1_dump,                /* device state information dump function */
    CARTRIDGE_MEGABYTER,               /* cartridge ID */
    0,                                 /* normal priority, device read needs to be checked for collisions */
    0,                                 /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                     /* NO mirroring */
};

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MEGABYTER, 1, 1, &megabyter_io1_device, NULL, CARTRIDGE_MEGABYTER
};

static void megabyter_io1_store(uint16_t addr, uint8_t value)
{
    uint8_t mem_mode;

    /* only A1 is decoded */
    if (addr & 2) {
        /* mode register */
        megabyter_register_02 = value & 0x83; /* we only remember led, mode, exrom, game */
        mem_mode = megabyter_memconfig[megabyter_register_02 & 0x03];
        cart_config_changed_slotmain(mem_mode, mem_mode, CMODE_READ);
        /* TODO: change led */
        /* (value & 0x80) -> led on if true, led off if false */
    } else {
        /* bank register */
        megabyter_register_00 = (uint8_t)(value & MEGABYTER_BANK_MASK);
    }

    DBG(("Megabyter: addr:0x%04x, value:%02x (bank:%02x mode:%i LED:%i)",
         addr | 0xde00u, value, megabyter_register_00,
         megabyter_memconfig[megabyter_register_02 & 0x03],
         megabyter_register_02 >> 7));

    cart_romlbank_set_slotmain(megabyter_register_00);
    cart_port_config_changed_slotmain();
}

/* ---------------------------------------------------------------------*/

static uint8_t megabyter_io1_peek(uint16_t addr)
{
    return (addr & 2) ? megabyter_register_02 : megabyter_register_00;
}

static int megabyter_io1_dump(void)
{
    int config = megabyter_memconfig[megabyter_register_02 & 0x03];
    mon_out("Mode: %i (%s), LED is %s\n",
            config, cart_config_string(config),
            (megabyter_register_02 & 0x80) ? "on" : "off");
    return 0;
}


/* ---------------------------------------------------------------------*/

static int set_megabyter_crt_write(int val, void *param)
{
    megabyter_crt_write = val ? 1 : 0;
    return 0;
}

static int set_megabyter_crt_optimize(int val, void *param)
{
    megabyter_crt_optimize = val ? 1 : 0;
    return 0;
}

static int megabyter_write_chip_if_not_empty(FILE* fd, crt_chip_header_t *chip, uint8_t *data)
{
    int i;

    for (i = 0; i < chip->size; i++) {
        if ((data[i] != 0xff) || (megabyter_crt_optimize == 0)) {
            if (crt_write_chip(data, chip, fd)) {
                return -1;
            }
            return 0;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "MegabyterWriteCRT", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &megabyter_crt_write, set_megabyter_crt_write, NULL },
    { "MegabyterOptimizeCRT", 1, RES_EVENT_STRICT, (resource_value_t)1,
      &megabyter_crt_optimize, set_megabyter_crt_optimize, NULL },
    RESOURCE_INT_LIST_END
};

int megabyter_resources_init(void)
{
    return resources_register_int(resources_int);
}

void megabyter_resources_shutdown(void)
{
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-megabytercrtwrite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MegabyterWriteCRT", (resource_value_t)1,
      NULL, "Enable writing to " CARTRIDGE_NAME_MEGABYTER " .crt image" },
    { "+megabytercrtwrite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MegabyterWriteCRT", (resource_value_t)0,
      NULL, "Disable writing to " CARTRIDGE_NAME_MEGABYTER " .crt image" },
    { "-megabytercrtoptimize", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MegabyterOptimizeCRT", (resource_value_t)1,
      NULL, "Enable " CARTRIDGE_NAME_MEGABYTER " .crt image optimize on write" },
    { "+megabytercrtoptimize", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MegabyterOptimizeCRT", (resource_value_t)0,
      NULL, "Disable optimizing " CARTRIDGE_NAME_MEGABYTER " .crt image on write" },
    CMDLINE_LIST_END
};

int megabyter_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

uint8_t megabyter_roml_read(uint16_t addr)
{
    return flash800core_read(megabyter_state, (megabyter_register_00 * MEGABYTER_BANK_SIZE) + (addr & 0x1fff));
}

void megabyter_roml_store(uint16_t addr, uint8_t value)
{
   flash800core_store(megabyter_state, (megabyter_register_00 * MEGABYTER_BANK_SIZE) + (addr & 0x1fff), value);
}

void megabyter_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    if (megabyter_state && megabyter_state->flash_data) {
        switch (addr & 0xe000) {
            case 0x8000:
                if (megabyter_state->flash_state == FLASH800_STATE_READ) {
                    *base = megabyter_state->flash_data + (megabyter_register_00 * MEGABYTER_BANK_SIZE) - 0x8000;
                    *start = 0x8000;
                    *limit = 0x9ffd;
                    return;
                }
                break;
            default:
                break;
        }
    }
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

void megabyter_config_init(void)
{
    megabyter_io1_store((uint16_t)0xde00, 0);   /* bank 0 */
    megabyter_io1_store((uint16_t)0xde02, 0);   /* 8k game, LED off */
}

void megabyter_config_setup(uint8_t *rawcart)
{
    int i;

    megabyter_state = lib_malloc(sizeof(flash800_context_t));

    flash800core_init(megabyter_state, maincpu_alarm_context, FLASH800_TYPE_CB, roml_banks);

    for (i = 0; i < MEGABYTER_NUM_BANKS; i++) {
        memcpy(megabyter_state->flash_data + (i * MEGABYTER_BANK_SIZE),
               rawcart + (i * MEGABYTER_BANK_SIZE), MEGABYTER_BANK_SIZE);
    }
}

/* ---------------------------------------------------------------------*/

static int megabyter_common_attach(const char *filename)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    megabyter_io1_list_item = io_source_register(&megabyter_io1_device);
    megabyter_filename = lib_strdup(filename);

    return 0;
}

int megabyter_bin_attach(const char *filename, uint8_t *rawcart)
{
    megabyter_filetype = CARTRIDGE_FILETYPE_NONE;

    if (util_file_load(filename, rawcart, MEGABYTER_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    megabyter_filetype = CARTRIDGE_FILETYPE_BIN;
    return megabyter_common_attach(filename);
}

int megabyter_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename)
{
    crt_chip_header_t chip;

    megabyter_filetype = CARTRIDGE_FILETYPE_NONE;
    memset(rawcart, 0xff, MEGABYTER_ROM_SIZE); /* empty flash */

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.size == MEGABYTER_BANK_SIZE) {
            if (chip.bank >= MEGABYTER_NUM_BANKS || !(chip.start == 0x8000)) {
                return -1;
            }
            if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    megabyter_filetype = CARTRIDGE_FILETYPE_CRT;
    return megabyter_common_attach(filename);
}

void megabyter_detach(void)
{
    if (megabyter_crt_write) {
        megabyter_flush_image();
    }
    flash800core_shutdown(megabyter_state);
    lib_free(megabyter_state);
    lib_free(megabyter_filename);
    megabyter_filename = NULL;
    io_source_unregister(megabyter_io1_list_item);
    megabyter_io1_list_item = NULL;
    export_remove(&export_res);
}

int megabyter_flush_image(void)
{
    if (megabyter_filename != NULL) {
        if (megabyter_filetype == CARTRIDGE_FILETYPE_BIN) {
            return megabyter_bin_save(megabyter_filename);
        } else if (megabyter_filetype == CARTRIDGE_FILETYPE_CRT) {
            return megabyter_crt_save(megabyter_filename);
        }
        return -1;
    }
    return -2;
}

int megabyter_bin_save(const char *filename)
{
    FILE *fd;
    int i;
    uint8_t *data;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    data = megabyter_state->flash_data;

    for (i = 0; i < MEGABYTER_NUM_BANKS; i++, data += MEGABYTER_BANK_SIZE) {
        if (fwrite(data, 1, MEGABYTER_BANK_SIZE, fd) != MEGABYTER_BANK_SIZE) {
            fclose(fd);
            return -1;
        }
    }

    fclose(fd);
    return 0;
}

int megabyter_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;
    uint8_t *data;
    int bank;

    fd = crt_create(filename, CARTRIDGE_MEGABYTER, 1, 0, CARTRIDGE_NAME_MEGABYTER);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = MEGABYTER_BANK_SIZE;

    for (bank = 0; bank < MEGABYTER_NUM_BANKS; bank++) {
        chip.bank = bank;

        data = megabyter_state->flash_data + (bank * MEGABYTER_BANK_SIZE);
        chip.start = 0x8000;
        if (megabyter_write_chip_if_not_empty(fd, &chip, data) != 0) {
            fclose(fd);
            return -1;
        }

    }
    fclose(fd);
    return 0;
}

/* ---------------------------------------------------------------------*/

/* CARTMEGABYTER snapshot module format:

   type  | name       | description
   --------------------------------
   BYTE  | register 0 | register 0
   BYTE  | register 2 | register 2
   ARRAY | ROML       | 1048576 BYTES of ROML data
 */

static char snap_module_name[] = "CARTMEGABYTER";
static char flash_snap_module_name[] = "FLASH800CB";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int megabyter_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, megabyter_register_00) < 0)
        || (SMW_B(m, megabyter_register_02) < 0)
        || (SMW_BA(m, roml_banks, MEGABYTER_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || (flash800core_snapshot_write_module(s, megabyter_state, flash_snap_module_name) < 0)) {
        return -1;
    }

    return 0;
}

int megabyter_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_B(m, &megabyter_register_00) < 0)
        || (SMR_B(m, &megabyter_register_02) < 0)
        || (SMR_BA(m, roml_banks, MEGABYTER_ROM_SIZE) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    megabyter_state = lib_malloc(sizeof(flash800_context_t));

    flash800core_init(megabyter_state, maincpu_alarm_context, FLASH800_TYPE_CB, roml_banks);

    if (0
        || (flash800core_snapshot_read_module(s, megabyter_state, flash_snap_module_name) < 0)) {
        flash800core_shutdown(megabyter_state);
        lib_free(megabyter_state);
        return -1;
    }

    megabyter_common_attach("dummy");

    /* remove dummy filename, set filetype to none */
    lib_free(megabyter_filename);
    megabyter_filename = NULL;
    megabyter_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
