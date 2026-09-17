/*
 * minimon.c -- VIC20 "Minimon" Cartridge emulation.
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
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "alarm.h"
#include "minimon.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "zfile.h"

/* #define DEBUGMINIMON */

#ifdef DEBUGMINIMON
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

/*

    "Minimon"

    - 2KiB ROM mapped into IO2/IO3 ($9800-$9BFF, $9C00-9FFF)

    The Cartridge has two switches and one button:

    - the MON/CART switch essentially enables/disables the cartridge, it either
      routes IO2/3 accesses to the cartridge ROM or to the pass-through port.

    - the PGM switch enables writing to the cartridge (which obviously only
      works if there is static RAM or EEPROM in the socket, which again is
      completely optional)

    - the reset button performs two functions:
      - Reset (short) is just that, a regular reset
      - Reset (long) resets and briefly "disables" a BLK5 cartridge on the pass-
        through port to prevent it from starting. The pass through port will
        also not "see" reset.
      The reset logic is available all the time.

    test:

    xvic -debug -cartA Poker-a000.prg -cartmini monitor.bin

*/

/* ------------------------------------------------------------------------- */
static const char STRING_MINIMON[] = CARTRIDGE_VIC20_NAME_MINIMON;

#define CART_ROM_SIZE (0x400 * 2)
static uint8_t *minimon_rom = NULL;
static int minimon_bios_type = 0;   /* flag: type of loaded file */

static int minimon_enabled = 0;       /* resource, is minimon cartridge attached? */
static int minimon_io_enabled = 0;    /* resource, is minimon ROM in IO enabled? (MON/CART) */
static int minimon_pgm_enabled = 0;   /* resource, is minimon ROM writeable? (PGM) */

static int minimon_bios_write = 0;  /* resource: write back the ROM when it was changed? */

static char *minimon_image_filename = NULL; /* resource: ROM filename */
static int minimon_bios_changed = 0; /* flag: was the ROM written to? */

static int minimon_io23_temp = 0;

static int set_minimon_enabled(int value, void *param);
static int minimon_rom_reload(char *filename);

/* ------------------------------------------------------------------------- */

static int freeze_triggered = 0; /* if not 0, a "freeze reset" was just triggered */

struct alarm_s *minimon_alarm;
static CLOCK freeze_timeout = 0;
static int freeze_bkl5_temp = 0;

/* this should be ~300ms */
#define FREEZE_DELAY    (1000000UL / 3UL)

#define ALARM_FREQ      (1000000UL / 100UL)

static void minimon_trigger_alarm(void)
{
    freeze_timeout = CLOCK_MAX;
    alarm_unset(minimon_alarm);

    /* HACK: mem_cart_blocks is reserved for main slot */
    /* remember blk5 config */
    freeze_bkl5_temp |= (mem_cart_blocks & VIC_CART_BLK5);
    /* "unmap" blk5, so our blk5 hook gets called instead */
    mem_cart_blocks &= ~(VIC_CART_BLK5);
    mem_initialize_memory();

    freeze_timeout = maincpu_clk + FREEZE_DELAY; /* ~ 300ms */
    alarm_set(minimon_alarm, maincpu_clk + ALARM_FREQ);
    freeze_triggered = 1;

    DBG(("minimon_trigger_alarm %"PRIu64" (start freeze, timeout: %"PRIu64")", maincpu_clk, freeze_timeout));
}

static void minimon_alarm_handler(CLOCK offset, void *data)
{
    if (maincpu_clk < freeze_timeout) {
        alarm_set(minimon_alarm, maincpu_clk + ALARM_FREQ);
        /* DBG(("minimon_alarm_handler %"PRIu64"", maincpu_clk)); */
        return;
    }
    freeze_timeout = CLOCK_MAX;
    alarm_unset(minimon_alarm);

    DBG(("minimon_alarm_handler %"PRIu64" (end freeze)", maincpu_clk));
    freeze_triggered = 0;

    /* HACK: mem_cart_blocks is reserved for main slot */
    /* restore blk5 config */
    mem_cart_blocks |= freeze_bkl5_temp;
    mem_initialize_memory();
    freeze_bkl5_temp = 0;
}

static void minimon_alarm_install(void)
{
    DBG(("minimon_alarm_install"));
    if (minimon_alarm == NULL) {
        minimon_alarm = alarm_new(maincpu_alarm_context, "MinimonAlarm", minimon_alarm_handler, NULL);
    }
    freeze_timeout = CLOCK_MAX;
}

static void minimon_alarm_deinstall(void)
{
    DBG(("minimon_alarm_deinstall"));
    alarm_destroy(minimon_alarm);
    minimon_alarm = NULL;
    freeze_triggered = 0;
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

static void clear_ram(void)
{
    DBG(("clear_rom: clear buffer with RAM pattern"));
    ram_init_with_pattern(minimon_rom, CART_ROM_SIZE, &ramparam);
}

static void clear_eprom(void)
{
    DBG(("clear_rom: clear buffer with EPROM pattern"));
    memset(minimon_rom, CART_ROM_SIZE, 0xff);
}

static void allocate_rom(void)
{
    if (!minimon_rom) {
        DBG(("allocate_rom: allocate buffer"));
        minimon_rom = lib_malloc(CART_ROM_SIZE);
        clear_ram();
        if (minimon_image_filename) {
            minimon_rom_reload(minimon_image_filename);
        }
    }
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t minimon_io2_read(uint16_t addr);
static uint8_t minimon_io3_read(uint16_t addr);
static void minimon_io2_write(uint16_t addr, uint8_t value);
static void minimon_io3_write(uint16_t addr, uint8_t value);
static int minimon_mon_dump(void);

static io_source_t minimon_io2_device = {
    CARTRIDGE_VIC20_NAME_MINIMON,  /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0x9800, 0x9bff, 0x3ff,         /* range for the device */
    1,                             /* read is always valid */
    minimon_io2_write,             /* store function */
    NULL,                          /* poke function */
    minimon_io2_read,              /* read function */
    NULL,                          /* NO peek function */
    minimon_mon_dump,              /* device state information dump function */
    CARTRIDGE_VIC20_MINIMON,       /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0,                             /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                 /* NO mirroring */
};

static io_source_list_t *minimon_io2_list_item = NULL;

static io_source_t minimon_io3_device = {
    CARTRIDGE_VIC20_NAME_MINIMON,  /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0x9c00, 0x9fff, 0x3ff,         /* range for the device, address is ignored, reg:$9c00, mirrors:$9c01-$9fff */
    1,                             /* read is always valid */
    minimon_io3_write,             /* store function */
    NULL,                          /* poke function */
    minimon_io3_read,              /* read function */
    NULL,                          /* NO peek function */
    minimon_mon_dump,              /* device state information dump function */
    CARTRIDGE_VIC20_MINIMON,       /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0,                             /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                 /* NO mirroring */
};

static io_source_list_t *minimon_io3_list_item = NULL;

static const export_resource_t export_res23 = {
    CARTRIDGE_VIC20_NAME_MINIMON, 0, 0, &minimon_io2_device, &minimon_io3_device, CARTRIDGE_VIC20_MINIMON
};

/* (only) register the io devices */
static int io_register(void)
{
    DBG(("io_register"));
    allocate_rom();

    /* HACK: mem_cart_blocks is reserved for main slot */
    minimon_io23_temp = mem_cart_blocks & (VIC_CART_IO2 | VIC_CART_IO3);
    mem_cart_blocks |= (VIC_CART_IO2 | VIC_CART_IO3);
    mem_initialize_memory();

    if (minimon_io2_list_item != NULL) {
        io_source_unregister(minimon_io2_list_item);
    }
    if (minimon_io3_list_item != NULL) {
        io_source_unregister(minimon_io3_list_item);
    }
    minimon_io2_list_item = io_source_register(&minimon_io2_device);
    minimon_io3_list_item = io_source_register(&minimon_io3_device);

    return 0;
}

/* (only) un-register the io devices */
static int io_unregister(void)
{
    DBG(("io_unregister"));
    if (minimon_io2_list_item != NULL) {
        io_source_unregister(minimon_io2_list_item);
        minimon_io2_list_item = NULL;
    }
    if (minimon_io3_list_item != NULL) {
        io_source_unregister(minimon_io3_list_item);
        minimon_io3_list_item = NULL;
    }
    /* HACK: mem_cart_blocks is reserved for main slot */
    mem_cart_blocks &= ~(VIC_CART_IO2 | VIC_CART_IO3);
    mem_cart_blocks |= minimon_io23_temp;
    mem_initialize_memory();

    return 0;
}

/* ------------------------------------------------------------------------- */

static uint8_t minimon_io2_read(uint16_t addr)
{
    if (minimon_bios_type == CARTRIDGE_FILETYPE_NONE) {
        return (0x9800 + (addr & 0x3ff)) >> 8; /* open bus */
    }
    return minimon_rom[0x000 + (addr & 0x3ff)];
}

static uint8_t minimon_io3_read(uint16_t addr)
{
    if (minimon_bios_type == CARTRIDGE_FILETYPE_NONE) {
        return (0x9a00 + (addr & 0x3ff)) >> 8; /* open bus */
    }
    return minimon_rom[0x400 + (addr & 0x3ff)];
}

static void minimon_io2_write(uint16_t addr, uint8_t value)
{
    if (minimon_bios_type != CARTRIDGE_FILETYPE_NONE) {
        if (minimon_pgm_enabled) {
            minimon_rom[0x000 + (addr & 0x3ff)] = value;
            minimon_bios_changed = 1;
        }
    }
}

static void minimon_io3_write(uint16_t addr, uint8_t value)
{
    if (minimon_bios_type != CARTRIDGE_FILETYPE_NONE) {
        if (minimon_pgm_enabled) {
            minimon_rom[0x400 + (addr & 0x3ff)] = value;
            minimon_bios_changed = 1;
        }
    }
}

static int minimon_mon_dump(void)
{
    mon_out("PGM is %s.\n", minimon_pgm_enabled ? "enabled" : "disabled");
    mon_out("ROM in IO2/3 is %s.\n", minimon_io_enabled ? "enabled" : "disabled");
    return 0;
}

/* ------------------------------------------------------------------------- */

/* read 0xa000-0xbfff */
int minimon_blk5_read(uint16_t addr, uint8_t *value)
{
    if (freeze_triggered) {
        /* inhibit reads for ~300ms */
        DBG(("minimon_blk5_read 0x%04x (ignored)", addr));
        *value = 0xff;
        return CART_READ_VALID;
    }
    /* read from secondary cartridge */
    /*DBG(("minimon_blk5_read 0x%04x", addr));*/
    return CART_READ_THROUGH;
}

/* ------------------------------------------------------------------------- */


void minimon_freeze(void)
{
    DBG(("minimon_freeze"));
    /* trigger reset first (resets cycle count) */
    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
    minimon_trigger_alarm();

}

void minimon_reset(void)
{
    if (freeze_triggered) {
        DBG(("minimon_reset (freeze)"));
        /* retrigger the alarm, else the cycle count is off */
        minimon_trigger_alarm();
    } else {
        DBG(("minimon_reset (regular)"));
    }
}

void minimon_powerup(void)
{
    DBG(("minimon_powerup"));
    freeze_triggered = 0;
    allocate_rom();
    /* FIXME: no ROM loaded yet */
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-minimon", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonEnabled", (resource_value_t)1,
      NULL, "Enable the Minimon expansion" },
    { "+minimon", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonEnabled", (resource_value_t)0,
      NULL, "Disable the Minimon expansion" },
    { "-minimonrom", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MinimonFilename", NULL,
      "<Name>", "Specify name of Minimon ROM image" },
    { "-minimonrw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonImageWrite", (resource_value_t)1,
      NULL, "Save the Minimon ROM when changed" },
    { "-minimonro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonImageWrite", (resource_value_t)0,
      NULL, "Do not save the Minimon ROM when changed" },
    { "-minimonpgm", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonPgmSwitch", (resource_value_t)1,
      NULL, "Set the Minimon PGM switch" },
    { "+minimonpgm", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonPgmSwitch", (resource_value_t)0,
      NULL, "Remove the Minimon PGM switch" },
    { "-minimonio", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonIoSwitch", (resource_value_t)1,
      NULL, "Set the Minimon IO switch" },
    { "+minimonio", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MinimonIoSwitch", (resource_value_t)0,
      NULL, "Remove the Minimon IO switch" },
    CMDLINE_LIST_END
};

int minimon_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* get the MinimonFilename resource */
const char *minimon_get_file_name(void)
{
    return minimon_image_filename;
}

/* set the MinimonFilename resource */
static int set_minimon_image_filename(const char *name, void *param)
{
    int enabled = minimon_enabled;

    DBG(("set_minimon_image_filename '%s' (enabled: %d)", name, enabled));

    /* if filename was already set, but not changed, return */
    if ((minimon_image_filename != NULL) &&
        (name != NULL) &&
        (strcmp(name, minimon_image_filename) == 0)) {
        return 0;
    }

    /* if filename is valid, check if a file with that name exists */
    if ((name != NULL) && (*name != '\0')) {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    DBG(("set_minimon_image_filename"));

    /* if already enabled, flush image and disable */
    set_minimon_enabled(0, (void*)1);

    util_string_set(&minimon_image_filename, name);

    /* enable, load new image */
    return set_minimon_enabled(enabled, (void*)1);
}

/* get the MinimonEnabled resource */
int minimon_cart_enabled(void)
{
    return minimon_enabled;
}

static int minimon_activate(void)
{
    minimon_bios_changed = 0;
    /* minimon_reset(); */
    return 0;
}

static int minimon_deactivate(void)
{
    int ret;

    DBG(("minimon_deactivate: minimon_bios_changed: %d minimon_bios_write: %d",
         minimon_bios_changed, minimon_bios_write));

    if (minimon_bios_changed && minimon_bios_write) {
        DBG(("minimon_deactivate: flushing image"));
        if (minimon_bios_type == CARTRIDGE_FILETYPE_CRT) {
            ret = minimon_crt_save(minimon_image_filename);
        } else {
            ret = minimon_bin_save(minimon_image_filename);
        }
        if (ret <= 0) {
            DBG(("minimon_deactivate: flush failed"));
            return 0; /* FIXME */
        }
    }
    return 0;
}

/* setup the MinimonEnabled resource */
static int set_minimon_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("Minimon: set_enabled: '%s' %d to %d", minimon_image_filename, minimon_enabled, val));
    if (!minimon_enabled && val) {
        /* activate minimon */
        if (param) {
            /* if the param is != NULL, then we should load the default image file */
            DBG(("Minimon: set_enabled(1) '%s'", minimon_image_filename));
            if (minimon_image_filename) {
                if (*minimon_image_filename) {
                    /* try .crt first */
                    if ((cartridge_attach_image(CARTRIDGE_CRT, minimon_image_filename) < 0) &&
                        (cartridge_attach_image(CARTRIDGE_VIC20_MINIMON, minimon_image_filename) < 0)) {
                        DBG(("Minimon: set_enabled(1) did not register"));
                        return -1;
                    }
                    /* minimon_enabled = 1; */ /* cartridge_attach_image will end up calling set_minimon_enabled again */
                    return 0;
                }
            }
        } else {
            DBG(("Minimon: set_enabled(0) '%s'", minimon_image_filename));
            /* cart_power_off(); */
            /* if the param is == NULL, then we should actually set the resource */
            if (export_add(&export_res23) < 0) {
                DBG(("Minimon: set_enabled(0) did not register"));
                return -1;
            } else {
                DBG(("Minimon: set_enabled registered"));

                if (minimon_activate() < 0) {
                    return -1;
                }
                minimon_enabled = 1;
                minimon_io2_list_item = io_source_register(&minimon_io2_device);
                minimon_io3_list_item = io_source_register(&minimon_io3_device);
                minimon_reset();
                minimon_alarm_install();
            }
        }
    } else if (minimon_enabled && !val) {
        /* remove minimon */
        if (minimon_deactivate() < 0) {
            return -1;
        }
        minimon_alarm_deinstall();
        /* cart_power_off(); */
        export_remove(&export_res23);
        minimon_enabled = 0;
        io_source_unregister(minimon_io2_list_item);
        io_source_unregister(minimon_io3_list_item);
        minimon_io2_list_item = NULL;
        minimon_io3_list_item = NULL;
    }
    DBG(("Minimon: set_enabled done: '%s' %d : %d", minimon_image_filename, val, minimon_enabled));
    return 0;
}


/* set the resource for the PGM switch */
static int set_minimon_pgm_enabled(int value, void *param)
{
    minimon_pgm_enabled = (value == 0) ? 0 : 1;
    DBG(("set_minimon_pgm_enabled: %d", minimon_pgm_enabled));
    return 0;
}

/* set the resource for the IO aka CART/MON switch */
static int set_minimon_io_enabled(int value, void *param)
{
    int new = (value == 0) ? 0 : 1;
    DBG(("set_minimon_io_enabled: %d->%d", minimon_io_enabled, new));
    if (minimon_io_enabled != new) {
        minimon_io_enabled = new;
        minimon_alarm_deinstall();
        if (minimon_enabled) {
            if (minimon_io_enabled) {
                io_register();
            } else {
                io_unregister();
            }
            minimon_alarm_install();
        }
    }
    return 0;
}

/* set "MinimonImageWrite" resource */
static int set_minimon_bios_write(int value, void *param)
{
    minimon_bios_write = (value == 0) ? 0 : 1;
    DBG(("set_minimon_bios_write: %d", minimon_bios_write));
    return 0;
}

static const resource_string_t resources_string[] = {
    { "MinimonFilename", "", RES_EVENT_NO, NULL,
      &minimon_image_filename, set_minimon_image_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MinimonEnabled", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &minimon_enabled, set_minimon_enabled, (void *)1 },
    { "MinimonPgmSwitch", 0, RES_EVENT_NO, NULL,
      &minimon_pgm_enabled, set_minimon_pgm_enabled, NULL },
    { "MinimonIoSwitch", 1, RES_EVENT_NO, NULL,
      &minimon_io_enabled, set_minimon_io_enabled, NULL },
    { "MinimonImageWrite", 0, RES_EVENT_NO, NULL,
      &minimon_bios_write, set_minimon_bios_write, NULL },
    RESOURCE_INT_LIST_END
};

int minimon_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void minimon_resources_shutdown(void)
{
    if (minimon_image_filename) {
        lib_free(minimon_image_filename);
    }
    minimon_image_filename = NULL;
}

/* ------------------------------------------------------------------------- */


static int zfile_load(const char *filename, uint8_t *dest, size_t size)
{
    FILE *fd;
    off_t len;

    fd = zfile_fopen(filename, MODE_READ);
    if (!fd) {
        return -1;
    }
    len = archdep_file_size(fd);
    if (len < 0 || (size_t)len != size) {
        zfile_fclose(fd);
        return -1;
    }
    if (fread(dest, size, 1, fd) < 1) {
        zfile_fclose(fd);
        return -1;
    }
    zfile_fclose(fd);
    return 0;
}

static int minimon_rom_reload(char *filename)
{
    crt_header_t header;
    crt_chip_header_t chip;
    FILE *fd = NULL;

    DBG(("minimon_rom_attach"));

    clear_eprom();
    if (minimon_bios_type != CARTRIDGE_FILETYPE_BIN) {
        fd = crt_open(filename, &header);

        if (fd == NULL) {
            return -1;
        }
        if (crt_read_chip_header(&chip, fd)) {
            fclose(fd);
            goto trybinary;
        }

        if (chip.size != CART_ROM_SIZE) {
            fclose(fd);
            goto trybinary;
        }

        if (crt_read_chip(&minimon_rom[0], 0, &chip, fd)) {
            fclose(fd);
            goto trybinary;
        }

        minimon_bios_type = CARTRIDGE_FILETYPE_CRT;
        DBG(("minimon_rom_attach (loaded: crt)"));
        fclose(fd);
        return 0;
    }
trybinary:

    if (minimon_bios_type != CARTRIDGE_FILETYPE_CRT) {
        if (zfile_load(filename, minimon_rom, (size_t)CART_ROM_SIZE) < 0) {
            return -1;
        }
        minimon_bios_type  = CARTRIDGE_FILETYPE_BIN;
        DBG(("minimon_rom_attach (loaded: bin)"));
        return 0;
    }
    return -1;
}

int minimon_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    DBG(("minimon_crt_attach"));

    allocate_rom();
    clear_eprom();

    if (crt_read_chip_header(&chip, fd)) {
        goto exiterror;
    }

    DBG(("chip at %02x len %02x\n", chip.start, chip.size));
    if (chip.size != CART_ROM_SIZE) {
        goto exiterror;
    }

    if (crt_read_chip(&minimon_rom[0], 0, &chip, fd)) {
        goto exiterror;
    }

    if (export_add(&export_res23) < 0) {
        goto exiterror;
    }

    io_register();

    minimon_enabled = 1; /* resource */
    minimon_io_enabled = 1; /* resource */

    minimon_bios_type = CARTRIDGE_FILETYPE_CRT;
    /* FIXME: set_minimon_image_filename(filename, NULL); */ /* set the resource */

    minimon_alarm_install();

    return CARTRIDGE_VIC20_MINIMON;

exiterror:
    minimon_detach();
    return -1;
}

int minimon_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("minimon_bin_attach"));

    allocate_rom();
    clear_eprom();

    if (zfile_load(filename, minimon_rom, (size_t)CART_ROM_SIZE) < 0) {
        minimon_detach();
        return -1;
    }

    if (export_add(&export_res23) < 0) {
        return -1;
    }

    io_register();

    minimon_enabled = 1; /* resource */
    minimon_io_enabled = 1; /* resource */

    minimon_bios_type = CARTRIDGE_FILETYPE_BIN;
    set_minimon_image_filename(filename, NULL); /* set the resource */

    minimon_alarm_install();

    return 0;
}

int minimon_bin_save(const char *filename)
{
    FILE *fd;
    size_t ret;

    DBG(("minimon_bin_save '%s'", filename));

    if (filename == NULL) {
        return -1;
    }

    if (minimon_rom == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);
    if (fd == NULL) {
        return -1;
    }

    ret = fwrite(minimon_rom, 1, CART_ROM_SIZE, fd);
    fclose(fd);
    if (ret != (CART_ROM_SIZE)) {
        return -1;
    }
    minimon_bios_changed = 0;
    return 0;
}

int minimon_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;

    DBG(("minimon_crt_save '%s'", filename));

    if (minimon_rom == NULL) {
        return -1;
    }

    fd = crt_create_vic20(filename, CARTRIDGE_VIC20_MINIMON, 0, STRING_MINIMON);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = CART_ROM_SIZE;
    chip.start = 0x9800;
    chip.bank = 0;

    if (crt_write_chip(minimon_rom, &chip, fd)) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

int minimon_flush_image(void)
{
    int ret = 0;
    DBG(("minimon_flush_image minimon_bios_changed:%d minimon_bios_write:%d",
         minimon_bios_changed, minimon_bios_write));
    if (minimon_bios_type == CARTRIDGE_FILETYPE_NONE) {
        log_warning(LOG_DEFAULT, "Flush: no minimon image attached");
    } else if (minimon_bios_changed && minimon_bios_write) {
        if (minimon_bios_type == CARTRIDGE_FILETYPE_CRT) {
            ret = minimon_crt_save(minimon_image_filename);
        } else {
            ret = minimon_bin_save(minimon_image_filename);
        }
    }
    minimon_bios_changed = 0;
    DBG(("minimon_flush_image ret: %d", ret));
    return ret;
}

void minimon_detach(void)
{
    minimon_alarm_deinstall();

    io_unregister();

    export_remove(&export_res23);

    lib_free(minimon_rom);
    minimon_rom = NULL;

    minimon_enabled = 0; /* resource */
}


int minimon_enable(void)
{
    return set_minimon_enabled(1, (void*)1); /* setup the resource */
}

int minimon_disable(void)
{
    return set_minimon_enabled(0, (void*)1); /* setup the resource */
}


/* ------------------------------------------------------------------------- */

/* MINIMON snapshot module format:

   type  | name              | description
   ---------------------------------------
   BYTE  | active            | cartridge active flag
   BYTE  | io rom            | cartridge rom in io
   BYTE  | bios write        | bios writable flag
   BYTE  | image readonly    | image read-only flag
   BYTE  | bios changed      | bios changed flag
   ARRAY | BIOS              | 2048 bytes of BIOS data ($9800-$9FFF)

 */

#define VIC20CART_DUMP_VER_MAJOR   0
#define VIC20CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "MINIMON"

int minimon_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)minimon_enabled) < 0)
        || (SMW_B(m, (uint8_t)minimon_io_enabled) < 0)
        || (SMW_B(m, (uint8_t)minimon_pgm_enabled) < 0)
        || (SMW_B(m, (uint8_t)minimon_bios_write) < 0)
        || (SMW_B(m, (uint8_t)minimon_bios_changed) < 0)
        || (SMW_BA(m, minimon_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int minimon_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != VIC20CART_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    allocate_rom();

    if (0
        || (SMR_B_INT(m, &minimon_enabled) < 0)
        || (SMR_B_INT(m, &minimon_io_enabled) < 0)
        || (SMR_B_INT(m, &minimon_pgm_enabled) < 0)
        || (SMR_B_INT(m, &minimon_bios_write) < 0)
        || (SMR_B_INT(m, &minimon_bios_changed) < 0)
        || (SMR_BA(m, minimon_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(minimon_rom);
        minimon_rom = NULL;
        minimon_bios_type = CARTRIDGE_FILETYPE_NONE;
        return -1;
    }

    snapshot_module_close(m);

    minimon_bios_type = CARTRIDGE_FILETYPE_SNAPSHOT;

    mem_cart_blocks = VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    return 0;
}
