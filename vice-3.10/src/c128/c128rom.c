/*
 * c128rom.c
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

#include "c128.h"
#include "c128mem.h"
#include "c128memrom.h"
#include "c128rom.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "machine.h"
#include "mem.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"
#include "util.h"
#include "z80mem.h"

/* #define DEBUG_C128ROM */

#ifdef DEBUG_C128ROM
#define DBG(x)  log_printf  x
#else
#define DBG(x)
#endif

static log_t c128rom_log = LOG_DEFAULT;

/* Flag: nonzero if the Kernal and BASIC ROMs are available.  */
static int rom_loaded = 0;

/******************************************************************************/

typedef struct
{
    char *name;
    uint16_t checksum;
    int id;
} ROMINFO;

static ROMINFO kernalinfo[] = {
    { "international r1", C128_KERNAL_R01_CHECKSUM, 1 },
    { "swedish r1", C128_KERNAL_SE_R01_CHECKSUM, 1 },
    { "german r1", C128_KERNAL_DE_R01_CHECKSUM, 1 },
    { "swiss r1", C128_KERNAL_CH_R01_CHECKSUM, 1 },
    { NULL, 0, 0 }
};

static char *checkrominfo(ROMINFO *info, uint16_t checksum, int id)
{
    int n = 0;
    while (info[n].name) {
        if (checksum == info[n].checksum) {
            if ((id == -1) || (id == info[n].id)) {
                return info[n].name;
            }
        }
        ++n;
    }
    return NULL;
}

int c128rom_kernal_checksum(void)
{
    int i, id;
    uint16_t sum;
    char *name;

    /* Check Kernal ROM.  */
    for (i = 0, sum = 0; i < C128_KERNAL_ROM_SIZE; i++) {
        sum = (uint16_t)(sum + c128memrom_kernal_rom[i]);
    }
    id = c128memrom_rom_read(0xff80);
    name = checkrominfo(kernalinfo, sum, id);

    if (name == NULL) {
        log_warning(c128rom_log, "Unknown kernal image. ID: %d Sum: %d.", id, sum);
    } else {
        log_message(c128rom_log, "Kernal is '%s' rev #%d.", name, id);
    }
    return 0;
}

/******************************************************************************/

/* Check if international Kernal ROM is available.  */
int c128rom_load_kernal_int(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if German Kernal ROM is available.  */
int c128rom_load_kernal_de(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if Finnish Kernal ROM is available.  */
int c128rom_load_kernal_fi(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if French Kernal ROM is available.  */
int c128rom_load_kernal_fr(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if Italian Kernal ROM is available.  */
int c128rom_load_kernal_it(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if Norwegian Kernal ROM is available.  */
int c128rom_load_kernal_no(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if Swedish Kernal ROM is available.  */
int c128rom_load_kernal_se(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if Swiss Kernal ROM is available.  */
int c128rom_load_kernal_ch(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

#define NUM_TRAP_DEVICES 9  /* FIXME: is there a better constant ? */
static int trapfl[NUM_TRAP_DEVICES];
static int trapdevices[NUM_TRAP_DEVICES + 1] = { 1, 4, 5, 6, 7, 8, 9, 10, 11, -1 };

static void get_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_get_int_sprintf("TrapDevice%d", &trapfl[i], trapdevices[i]);
    }
}

static void clear_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_set_int_sprintf("TrapDevice%d", 0, trapdevices[i]);
    }
}

static void restore_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_set_int_sprintf("TrapDevice%d", trapfl[i], trapdevices[i]);
    }
}

static ROMINFO machineinfo[] = {
    { "international", C128_MACHINE_INT, 0 },
    { "finnish", C128_MACHINE_FINNISH, 0 },
    { "french", C128_MACHINE_FRENCH, 0 },
    { "german", C128_MACHINE_GERMAN, 0 },
    { "italian", C128_MACHINE_ITALIAN, 0 },
    { "norwegian", C128_MACHINE_NORWEGIAN, 0 },
    { "swedish", C128_MACHINE_SWEDISH, 0 },
    { "swiss", C128_MACHINE_SWISS, 0 },
    { NULL, 0, 0 }
};

int c128rom_kernal_setup(void)
{
    int machine_type;
    const char *resname = NULL;
    const char *rom_name = NULL;
    uint8_t *kernal = NULL;
    static const char *last_kernal64 = NULL;
    char *name;
    const char *kernal64 = C128_KERNAL64_NAME;

    if (!rom_loaded) {
        return 0;
    }

    resources_get_int("MachineType", &machine_type);

    switch (machine_type) {
        case C128_MACHINE_INT:
            resname = "KernalIntName";
            break;
        case C128_MACHINE_FINNISH:
            resname = "KernalFIName";
            break;
        case C128_MACHINE_FRENCH:
            resname = "KernalFRName";
            break;
        case C128_MACHINE_GERMAN:
            resname = "KernalDEName";
            break;
        case C128_MACHINE_ITALIAN:
            resname = "KernalITName";
            break;
        case C128_MACHINE_NORWEGIAN:
            kernal64 = C128_KERNAL64_NO_NAME;
            resname = "KernalNOName";
            break;
        case C128_MACHINE_SWEDISH:
            kernal64 = C128_KERNAL64_SE_NAME;
            resname = "KernalSEName";
            break;
        case C128_MACHINE_SWISS:
            resname = "KernalCHName";
            break;
        default:
            log_error(c128rom_log, "Unknown machine type %i.", machine_type);
            return -1;
    }

    log_verbose(c128rom_log, "kernal64:%s", kernal64);

    if (kernal64 != last_kernal64) {
        resources_set_string("Kernal64Name", kernal64);
    }
    last_kernal64 = kernal64;

    /* disable traps before loading the ROM */
    get_trapflags();
    clear_trapflags();

    kernal = lib_malloc(C128_KERNAL_ROM_IMAGE_SIZE);
    if (kernal == NULL) {
        goto exiterror;
    }

    if (resources_get_string(resname, &rom_name) < 0) {
        goto exiterror;
    }

    if (sysfile_load(rom_name, machine_name, kernal, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
        log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
        goto exiterror;
    }

    memcpy(&c128memrom_basic_rom[C128_BASIC_ROM_SIZE], kernal, C128_EDITOR_ROM_SIZE);
    memcpy(z80bios_rom, &kernal[C128_EDITOR_ROM_SIZE], C128_Z80BIOS_ROM_SIZE);
    memcpy(c128memrom_kernal_rom, &kernal[C128_EDITOR_ROM_SIZE + C128_Z80BIOS_ROM_SIZE], C128_KERNAL_ROM_SIZE);
    memcpy(c128memrom_kernal_trap_rom, c128memrom_kernal_rom, C128_KERNAL_ROM_SIZE);

    if ((name = checkrominfo(machineinfo, machine_type, -1)) != NULL) {
        log_message(c128rom_log, "Switching ROMs to '%s':", name);
    }
    c128rom_basic_checksum();
    c128rom_kernal_checksum();

    restore_trapflags();

    lib_free(kernal);
    return 0;

exiterror:
    if (kernal) {
        lib_free(kernal);
    }
    return -1;
}

/******************************************************************************/

static ROMINFO basicinfo[] = {
    { "85", C128_BASIC_85_CHECKSUM, 0 },
    { "86", C128_BASIC_86_CHECKSUM, 0 },
    { NULL, 0, 0 }
};

static ROMINFO editorinfo[] = {
    { "international r1", C128_EDITOR_R01_CHECKSUM, 1 },
    { "swedish r1", C128_EDITOR_SE_R01_CHECKSUM, 1 },
    { "german r1", C128_EDITOR_DE_R01_CHECKSUM, 1 },
    { NULL, 0, 0 }
};

int c128rom_basic_checksum(void)
{
    int i, id;
    uint16_t sum;
    char *name;

    /* Check Basic ROM.  */
    for (i = 0, sum = 0; i < C128_BASIC_ROM_SIZE; i++) {
        sum += c128memrom_basic_rom[i];
    }
    name = checkrominfo(basicinfo, sum, -1);

    if (name == NULL) {
        log_warning(c128rom_log, "Unknown BASIC image. Sum: %d ($%04X).", sum, sum);
    } else {
        log_message(c128rom_log, "BASIC is '%s'.", name);
    }

    /* Check Editor ROM.  */
    for (i = C128_BASIC_ROM_SIZE, sum = 0; i < C128_BASIC_ROM_SIZE + C128_EDITOR_ROM_SIZE; i++) {
        sum += c128memrom_basic_rom[i];
    }
    id = c128memrom_rom_read(0xff80);
    name = checkrominfo(editorinfo, sum, id);

    if (name == NULL) {
        log_warning(c128rom_log, "Unknown editor image. ID: %d Sum: %d.", id, sum);
    } else {
        log_message(c128rom_log, "Editor is '%s' rev #%d.", name, id);
    }
    return 0;
}

/******************************************************************************/

int c128rom_basic_setup(void)
{
    const char *rom_name = NULL;

    if (!rom_loaded) {
        return 0;
    }

    if (resources_get_string("BasicLoName", &rom_name) < 0) {
        return -1;
    }

    /* Load Basic ROM.  */
    if (sysfile_load(rom_name, machine_name, c128memrom_basic_rom, C128_BASIC_ROM_IMAGELO_SIZE, C128_BASIC_ROM_IMAGELO_SIZE) < 0) {
        log_error(c128rom_log, "Couldn't load basic ROM `%s'.", rom_name);
        return -1;
    }

    if (resources_get_string("BasicHiName", &rom_name) < 0) {
        return -1;
    }

    if (sysfile_load(rom_name, machine_name, &c128memrom_basic_rom[C128_BASIC_ROM_IMAGELO_SIZE], C128_BASIC_ROM_IMAGEHI_SIZE, C128_BASIC_ROM_IMAGEHI_SIZE) < 0) {
        log_error(c128rom_log, "Couldn't load basic ROM `%s'.", rom_name);
        return -1;
    }
    return 0;
}

/* Check if Basic ROM (lo) exists  */
int c128rom_load_basiclo(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if Basic ROM (hi) exists  */
int c128rom_load_basichi(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/******************************************************************************/

int c128rom_chargen_setup(void)
{
    int machine_type;
    const char *resname = NULL;
    const char *rom_name = NULL;

    if (!rom_loaded) {
        return 0;
    }

    resources_get_int("MachineType", &machine_type);

    switch (machine_type) {
        case C128_MACHINE_INT:
            resname = "ChargenIntName";
            break;
        case C128_MACHINE_ITALIAN:
            resname = "ChargenITName";
            break;
        case C128_MACHINE_FRENCH:
            resname = "ChargenFRName";
            break;
        case C128_MACHINE_GERMAN:
            resname = "ChargenDEName";
            break;
        case C128_MACHINE_FINNISH:
            resname = "ChargenFIName";
            break;
        case C128_MACHINE_SWEDISH:
            resname = "ChargenSEName";
            break;
        case C128_MACHINE_SWISS:
            resname = "ChargenCHName";
            break;
        case C128_MACHINE_NORWEGIAN:
            resname = "ChargenNOName";
            break;
        default:
            log_error(c128rom_log, "Unknown machine type %i.", machine_type);
            return -1;
    }

    if (resources_get_string(resname, &rom_name) < 0) {
        return -1;
    }

    /* Load chargen ROM.  */
    if (sysfile_load(rom_name, machine_name, mem_chargen_rom, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE) < 0) {
        log_error(c128rom_log, "Couldn't load character ROM `%s'.", rom_name);
        return -1;
    }

    return 0;
}

/* Check if international Chargen ROM is available.  */
int c128rom_load_chargen_int(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if german Chargen ROM is available.  */
int c128rom_load_chargen_de(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if italian french ROM is available.  */
int c128rom_load_chargen_fr(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if swedish Chargen ROM is available.  */
int c128rom_load_chargen_se(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if swiss Chargen ROM is available.  */
int c128rom_load_chargen_ch(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if norwegian Chargen ROM is available.  */
int c128rom_load_chargen_no(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if finnish Chargen ROM is available.  */
int c128rom_load_chargen_fi(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Check if italian Chargen ROM is available.  */
int c128rom_load_chargen_it(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/******************************************************************************/

int c128rom_kernal64_setup(void)
{
    const char *rom_name = NULL;

    if (!rom_loaded) {
        return 0;
    }

    if (resources_get_string("Kernal64Name", &rom_name) < 0) {
        return -1;
    }

    /* Load C64 kernal ROM.  */
    if (sysfile_load(rom_name, machine_name, c64memrom_kernal64_rom, C128_KERNAL64_ROM_SIZE, C128_KERNAL64_ROM_SIZE) < 0) {
        log_error(c128rom_log, "Couldn't load C64 kernal ROM `%s'.", rom_name);
        return -1;
    }

    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C128_KERNAL64_ROM_SIZE);

    return 0;
}

/* check if C64 kernal ROM exists */
int c128rom_load_kernal64(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

int c128rom_basic64_setup(void)
{
    const char *rom_name = NULL;

    if (!rom_loaded) {
        return 0;
    }

    if (resources_get_string("Basic64Name", &rom_name) < 0) {
        return -1;
    }

    /* Load Basic ROM.  */
    if (sysfile_load(rom_name, machine_name, c64memrom_basic64_rom, C128_BASIC64_ROM_SIZE, C128_BASIC64_ROM_SIZE) < 0) {
        log_error(c128rom_log, "Couldn't load C64 basic ROM `%s'.", rom_name);
        return -1;
    }

    return 0;
}

/* Check if C64 basic ROM exists.  */
int c128rom_load_basic64(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}

/******************************************************************************/

static int c128rom_check_res_file(const char* resname)
{
    const char *rom_name = NULL;
    if (resources_get_string(resname, &rom_name) < 0) {
        return -1;
    }
    if (!rom_loaded) {
        return -1;
    }
    if (!util_check_null_string(rom_name)) {
        if (sysfile_locate(rom_name, machine_name, NULL) < 0) {
            return -1;
        }
    }
    return 0;
}


int mem_load(void)
{
    if (c128rom_log == LOG_DEFAULT) {
        c128rom_log = log_open("C128MEM");
    }

    mem_initialize_memory();

    rom_loaded = 1;

    if ((c128rom_check_res_file("BasicLoName") < 0) ||
        (c128rom_check_res_file("BasicHiName") < 0)) {
        return -1;
    }

    c128rom_basic_setup();

    if ((c128rom_check_res_file("KernalIntName") < 0) ||
        (c128rom_check_res_file("KernalDEName") < 0) ||
        (c128rom_check_res_file("KernalFIName") < 0) ||
        (c128rom_check_res_file("KernalFRName") < 0) ||
        (c128rom_check_res_file("KernalITName") < 0) ||
        (c128rom_check_res_file("KernalNOName") < 0) ||
        (c128rom_check_res_file("KernalSEName") < 0) ||
        (c128rom_check_res_file("KernalCHName") < 0)) {
        return -1;
    }

    c128rom_kernal_setup();

    if ((c128rom_check_res_file("ChargenIntName") < 0) ||
        (c128rom_check_res_file("ChargenDEName") < 0) ||
        (c128rom_check_res_file("ChargenFRName") < 0) ||
        (c128rom_check_res_file("ChargenSEName") < 0) ||
        (c128rom_check_res_file("ChargenCHName") < 0) ||
        (c128rom_check_res_file("ChargenNOName") < 0) ||
        (c128rom_check_res_file("ChargenFIName") < 0) ||
        (c128rom_check_res_file("ChargenITName") < 0)) {
        return -1;
    }

    c128rom_chargen_setup();

    if (c128rom_check_res_file("Kernal64Name") < 0) {
        return -1;
    }
    c128rom_kernal64_setup();

    if (c128rom_check_res_file("Basic64Name") < 0) {
        return -1;
    }
    c128rom_basic64_setup();

    return 0;
}

/* FIXME: the extra parameter cartkernal was used to replace the kernal
   with a cartridge kernal rom image.

   CAUTION: The current code does NOT use this anymore, cartkernal is always NULL
*/
int c64rom_load_kernal(const char *rom_name, uint8_t *cartkernal)
{
    /* CAUTION: this is the only place where c128rom_load_kernal64 gets called
     *  with cartkernal potentially being not NULL */
    return c128rom_load_kernal64(rom_name);
}
