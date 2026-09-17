/** \file   cartconv.c
 * \brief   Cartridge Conversion utility
 *
 * \author  Marco van den heuvel <blackystardust68@yahoo.com>
 * \author  groepaz <groepaz@gmx.net>
 */

/*
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

/* #define DEBUG_CARTCONV */

#include "vice.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#include "version.h"
#ifdef USE_SVN_REVISION
# include "svnversion.h"
#endif

#include "cartridge.h"
#include "machine.h"

#include "cartconv.h"
#include "c64-cartridges.h"
#include "c64-saver.h"
#include "c128-cartridges.h"
#include "c128-saver.h"
#include "cbm2-cartridges.h"
#include "cbm2-saver.h"
#include "crt.h"
#include "plus4-cartridges.h"
#include "vic20-cartridges.h"

#ifdef main
#  if main == SDL_main
#    undef main
#  endif
#endif

#ifdef DEBUG_CARTCONV
#define LOG(x)  printf x
#else
#define LOG(x)
#endif

unsigned int loadfile_size = 0;
unsigned char filebuffer[CARTRIDGE_SIZE_MAX + 2];
FILE *outfile;
int loadfile_offset = 0;
char *output_filename[MAX_OUTPUT_FILES];
int output_filenames = 0;
char *input_filename[MAX_INPUT_FILES];
int input_filenames = 0;
char loadfile_is_crt = 0;
int load_address = 0;
int loadfile_cart_type = 0;
unsigned char extra_buffer_32kb[0x8000];
unsigned char cart_subtype = 0;
signed char cart_type = -1;
char *cart_name = NULL;

static unsigned char chipbuffer[16];
static FILE *infile;

/* c64 flags */
char loadfile_is_ultimax = 0;
char convert_to_ultimax = 0;

/* plus4 flags */
int convert_to_c2 = 0;

/* options given on commandline */
int omit_empty_banks = 1;
int quiet_mode = 0;
static char convert_to_bin = 0;
static char convert_to_prg = 0;
static int repair_mode = 0;
static int input_padding = 0;
static char *options_filename = NULL;

int machine_class = VICE_MACHINE_C64;

int verbose = 0;

static int detect_input_file(char *filename);

/*****************************************************************************/

/* this is POSIX only */
#ifndef HAVE_STRDUP
char *strdup(const char *string)
{
    char *new;

    new = malloc(strlen(string) + 1);
    if (new != NULL) {
        strcpy(new, string);
    }
    return new;
}
#endif

#if 0
/* this is POSIX only */
#if !defined(HAVE_STRNCASECMP)
static const unsigned char charmap[] = {
    '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
    '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
    '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
    '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
    '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
    '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
    '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
    '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
    '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
    '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
    '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
    '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
    '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
    '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
    '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
    '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
    '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
    '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
    '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
    '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
    '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
    '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
    '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
    '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
    '\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
    '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
    '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
    '\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
    '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
    '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
    '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
    '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    unsigned char u1, u2;

    for (; n != 0; --n) {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (charmap[u1] != charmap[u2]) {
            return charmap[u1] - charmap[u2];
        }

        if (u1 == '\0') {
            return 0;
        }
    }
    return 0;
}
#endif
#endif

/*****************************************************************************/

void cleanup(void)
{
    int i;

    if (options_filename != NULL) {
        free(options_filename);
    }
    if (cart_name != NULL) {
        free(cart_name);
    }
    for (i = 0; i < MAX_INPUT_FILES; i++) {
        if (input_filename[i] != NULL) {
            free(input_filename[i]);
        }
    }
    for (i = 0; i < MAX_OUTPUT_FILES; i++) {
        if (output_filename[i] != NULL) {
            free(output_filename[i]);
        }
    }
}

static void too_many_inputs(void)
{
    fprintf(stderr, "Error: too many input files\n");
    cleanup();
    exit(1);
}

static void too_many_outputs(void)
{
    fprintf(stderr, "Error: too many output files\n");
    cleanup();
    exit(1);
}

/* this loads the easyflash cart and puts it as the interleaved way into
   the buffer for easy binary saving */
static int load_easyflash_crt(void)
{
    unsigned int load_position;

    memset(filebuffer, 0xff, 0x100000);
    while (1) {
        if (fread(chipbuffer, 1, 16, infile) != 16) {
            if (loadfile_size == 0) {
                return -1;
            } else {
                return 0;
            }
        }
        loadfile_size = 0x100000;
        if (chipbuffer[0] != 'C' ||
            chipbuffer[1] != 'H' ||
            chipbuffer[2] != 'I' ||
            chipbuffer[3] != 'P') {
            return -1;
        }
        if (load_address == 0) {
            load_address = (chipbuffer[CRT_CHIP_OFFS_LOAD_HI] << 8) + chipbuffer[CRT_CHIP_OFFS_LOAD_LO];
        }
        load_position = (unsigned int)((chipbuffer[CRT_CHIP_OFFS_BANK_LO] * 0x4000) + ((chipbuffer[CRT_CHIP_OFFS_LOAD_HI] == 0x80) ? 0 : 0x2000));
        if (fread(filebuffer + load_position, 1, 0x2000, infile) != 0x2000) {
            return -1;
        }
    }
}

/* load a megabyter crt, handle non existing banks */
static int load_megabyter_crt(void)
{
    unsigned int load_position;

    memset(filebuffer, 0xff, 0x100000);
    while (1) {
        if (fread(chipbuffer, 1, 16, infile) != 16) {
            if (loadfile_size == 0) {
                return -1;
            } else {
                return 0;
            }
        }
        loadfile_size = 0x100000;
        if (chipbuffer[0] != 'C' ||
            chipbuffer[1] != 'H' ||
            chipbuffer[2] != 'I' ||
            chipbuffer[3] != 'P') {
            return -1;
        }
        if (load_address == 0) {
            load_address = (chipbuffer[CRT_CHIP_OFFS_LOAD_HI] << 8) + chipbuffer[CRT_CHIP_OFFS_LOAD_LO];
        }
        load_position = (unsigned int)(chipbuffer[CRT_CHIP_OFFS_BANK_LO] * 0x2000);
        if (fread(filebuffer + load_position, 1, 0x2000, infile) != 0x2000) {
            return -1;
        }
    }
}

static int load_multicart_crt(void)
{
    unsigned int load_position;
    unsigned int size = 0;
    memset(filebuffer, 0xff, 0x400000);
    while (1) {
        loadfile_size = size;
        if (fread(chipbuffer, 1, 16, infile) != 16) {
            if (loadfile_size == 0) {
                return -1;
            } else {
                if (size == 0x200000) {
                    memcpy(&filebuffer[0x100000], &filebuffer[0x200000], 0x100000);
                    memset(&filebuffer[0x200000], 0xff, 0x100000);
                }
                return 0;
            }
        }
        if (chipbuffer[0] != 'C' ||
            chipbuffer[1] != 'H' ||
            chipbuffer[2] != 'I' ||
            chipbuffer[3] != 'P') {
            return -1;
        }
        if (load_address == 0) {
            load_address = (chipbuffer[CRT_CHIP_OFFS_LOAD_HI] << 8) +
                            chipbuffer[CRT_CHIP_OFFS_LOAD_LO];
        }
        load_position = (unsigned int)((chipbuffer[CRT_CHIP_OFFS_BANK_LO] * 0x4000) +
                                      ((chipbuffer[CRT_CHIP_OFFS_LOAD_HI] == 0x80) ? 0 : 0x200000));
        if (fread(filebuffer + load_position, 1, 0x4000, infile) != 0x4000) {
            return -1;
        }
        size += 0x4000;
        LOG(("size: %06x load_position: %06x load_address: %04x\n", size, load_position, load_address));
    }
}

static int load_all_banks_from_crt(void)
{
    unsigned int length, datasize, loadsize, pad;

    if (machine_class == VICE_MACHINE_C64) {
        if (loadfile_cart_type == CARTRIDGE_EASYFLASH) {
            return load_easyflash_crt();
        }
        if (loadfile_cart_type == CARTRIDGE_MEGABYTER) {
            return load_megabyter_crt();
        }
    } else if (machine_class == VICE_MACHINE_PLUS4) {
        if (loadfile_cart_type == CARTRIDGE_PLUS4_MULTI) {
            return load_multicart_crt();
        }
    }

    while (1) {
        /* get CHIP header */
        if (fread(chipbuffer, 1, CRT_CHIP_HEADER_LEN, infile) != CRT_CHIP_HEADER_LEN) {
            if (loadfile_size == 0) {
                fprintf(stderr, "Error: could not read data from file.\n");
                return -1;
            } else {
                return 0;
            }
        }
        if (chipbuffer[CRT_CHIP_OFFS_C] != 'C' ||
            chipbuffer[CRT_CHIP_OFFS_H] != 'H' ||
            chipbuffer[CRT_CHIP_OFFS_I] != 'I' ||
            chipbuffer[CRT_CHIP_OFFS_P] != 'P') {
            fprintf(stderr, "Error: CHIP tag not found.\n");
            return -1;
        }
        /* set load address to the load address of first CHIP in the file. this is not quite
           correct, but works ok for the few cases when it matters */
        if (load_address == 0) {
            load_address = (chipbuffer[CRT_CHIP_OFFS_LOAD_HI] << 8) + chipbuffer[CRT_CHIP_OFFS_LOAD_LO];
        }
        length = (unsigned int)((chipbuffer[4] << 24) + (chipbuffer[5] << 16) + (chipbuffer[6] << 8) + chipbuffer[7]);
        datasize = (unsigned int)((chipbuffer[CRT_CHIP_OFFS_SIZE_HI] * 0x100) + chipbuffer[CRT_CHIP_OFFS_SIZE_LO]);
        loadsize = datasize;
        if ((datasize + 0x10) > length) {
            if (repair_mode) {
                fprintf(stderr, "Warning: data size exceeds chunk length. (data:%04x chunk:%04x)\n", datasize, length);
                loadsize = length - 0x10;
            } else {
                fprintf(stderr, "Error: data size exceeds chunk length. (data:%04x chunk:%04x) (use -r to force)\n", datasize, length);
                return -1;
            }
        }
        /* load data */
        if (fread(filebuffer + loadfile_size, 1, loadsize, infile) != loadsize) {
            if (repair_mode) {
                fprintf(stderr, "Warning: unexpected end of file.\n");
                loadfile_size += datasize;
                break;
            }
            fprintf(stderr, "Error: could not read data from file. (use -r to force)\n");
            return -1;
        }
        /* if the chunk is larger than the contained data+chip header, skip the rest */
        pad = length - (datasize + 0x10);
        if (pad > 0) {
            fprintf(stderr, "Warning: chunk length exceeds data size (data:%04x chunk:%04x), skipping %04x bytes.\n", datasize, length, pad);
            fseek(infile, pad, SEEK_CUR);
        }
        loadfile_size += datasize;
    }
    return 0;
}

void bin2crt_ok(void)
{
    int i;
    if (!quiet_mode) {
        for (i = 0; i < input_filenames; i++) {
            printf("Input file: %s\n", input_filename[i]);
        }
        for (i = 0; i < output_filenames; i++) {
            printf("Output file: %s\n", output_filename[i]);
        }
        if (machine_class == VICE_MACHINE_C64) {
            printf("Conversion from binary format to C64 %s .crt successful.\n",
                cart_info[(unsigned char)cart_type].name);
        } else if (machine_class == VICE_MACHINE_C128) {
            printf("Conversion from binary format to C128 %s .crt successful.\n",
                cart_info_c128[(unsigned char)cart_type].name);
        } else if (machine_class == VICE_MACHINE_CBM6x0 ||
                   machine_class == VICE_MACHINE_CBM6x0) {
            printf("Conversion from binary format to CBM2 %s .crt successful.\n",
                cart_info_cbm2[(unsigned char)cart_type].name);
        } else if (machine_class == VICE_MACHINE_VIC20) {
            printf("Conversion from binary format to VIC20 %s .crt successful.\n",
                cart_info_vic20[(unsigned char)cart_type].name);
        } else if (machine_class == VICE_MACHINE_PLUS4) {
            printf("Conversion from binary format to Plus4 %s .crt successful.\n",
                cart_info_plus4[(unsigned char)cart_type].name);
        }
    }
}

void crt2bin_ok(void)
{
    int i;
    if (!quiet_mode) {
        for (i = 0; i < input_filenames; i++) {
            printf("Input file: %s\n", input_filename[i]);
        }
        for (i = 0; i < output_filenames; i++) {
            printf("Output file: %s\n", output_filename[i]);
        }
        if (machine_class == VICE_MACHINE_C64) {
            printf("Conversion from C64 %s .crt to binary format successful.\n",
                cart_info[loadfile_cart_type].name);
        } else if (machine_class == VICE_MACHINE_C128) {
            printf("Conversion from C128 %s .crt to binary format successful.\n",
                cart_info_c128[loadfile_cart_type].name);
        } else if (machine_class == VICE_MACHINE_CBM6x0 ||
                   machine_class == VICE_MACHINE_CBM6x0) {
            printf("Conversion from CBM2 %s .crt to binary format successful.\n",
                cart_info_cbm2[loadfile_cart_type].name);
        } else if (machine_class == VICE_MACHINE_VIC20) {
            printf("Conversion from VIC20 %s .crt to binary format successful.\n",
                cart_info_vic20[loadfile_cart_type].name);
        } else if (machine_class == VICE_MACHINE_PLUS4) {
            printf("Conversion from Plus4 %s .crt to binary format successful.\n",
                cart_info_plus4[loadfile_cart_type].name);
        }
    }
}

static int save_binary(unsigned char *buffer, char *filename, unsigned int address, unsigned int size)
{
    unsigned char address_buffer[2];
    outfile = fopen(filename, "wb");
    if (outfile == NULL) {
        fprintf(stderr, "Error: Can't open output file %s\n", filename);
        return -1;
    }
    if (convert_to_prg == 1) {
        address_buffer[0] = (unsigned char)(address & 0xff);
        address_buffer[1] = (unsigned char)(address >> 8);
        if (fwrite(address_buffer, 1, 2, outfile) != 2) {
            fprintf(stderr, "Error: Can't write to file %s\n", filename);
            fclose(outfile);
            return -1;
        }
    }
    if (fwrite(buffer, 1, size, outfile) != size) {
        fprintf(stderr, "Error: Can't write to file %s\n", filename);
        fclose(outfile);
        return -1;
    }
    fclose(outfile);
    return 0;
}

static int save_binary_output_file(void)
{

    LOG(("save_binary_output_file mode:%s addr:%04x size:%04x\n",
           convert_to_prg ? "prg" : "bin", load_address, loadfile_size));

    if ((machine_class == VICE_MACHINE_VIC20) &&
        (output_filenames == 2) &&
        (loadfile_size == 0x4000)) {
        /* handle vic20 bins that contain a gap between two blocks */
        if (save_binary(filebuffer, output_filename[0], load_address, 0x2000) < 0) {
            return -1;
        }
        /* FIXME: get address for second chunk */
        if (save_binary(filebuffer + 0x2000, output_filename[1], load_address, 0x2000) < 0) {
            return -1;
        }
    } else {
        if (save_binary(filebuffer, output_filename[0], load_address, loadfile_size) < 0) {
            return -1;
        }
    }
    crt2bin_ok();
    return 0;
}

void save_regular_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    unsigned int i;
    unsigned int real_banks = banks;

    LOG(("save_regular_crt  loadfile_size: %x cart length:%x banks:%u load@:%02x chiptype:%u\n",
            loadfile_size, length, banks, address, type));

    if (write_crt_header(game, exrom) < 0) {
        cleanup();
        exit(1);
    }

    if (real_banks == 0) {
        /* handle the case when a chip of half/4th the regular size
           is used on an otherwise identical hardware (eg 2k/4k
           chip on a 8k cart)
        */
        if (loadfile_size == (length / 2)) {
            length /= 2;
        } else if (loadfile_size == (length / 4)) {
            length /= 4;
        }
        real_banks = loadfile_size / length;
    }

    for (i = 0; i < real_banks; i++) {
        if (write_chip_package(length, i, address, (unsigned char)type) < 0) {
            cleanup();
            exit(1);
        }
    }
    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

static int detect_input_file(char *filename)
{
    loadfile_offset = 0;
    loadfile_is_crt = 0;

    if (read_crt_header(filename) < 0) {
        /* not a crt file */
        return 0;
    } else {

        if (!strncmp("C64 CARTRIDGE   ", (char *)headerbuffer, 16)) {
            loadfile_is_crt = 1;
            machine_class = VICE_MACHINE_C64;
        } else if (!strncmp("C128 CARTRIDGE  ", (char *)headerbuffer, 16)) {
            loadfile_is_crt = 1;
            machine_class = VICE_MACHINE_C128;
        } else if (!strncmp("CBM2 CARTRIDGE  ", (char *)headerbuffer, 16)) {
            loadfile_is_crt = 1;
            machine_class = VICE_MACHINE_CBM6x0;
        } else if (!strncmp("VIC20 CARTRIDGE ", (char *)headerbuffer, 16)) {
            loadfile_is_crt = 1;
            machine_class = VICE_MACHINE_VIC20;
        } else if (!strncmp("PLUS4 CARTRIDGE ", (char *)headerbuffer, 16)) {
            loadfile_is_crt = 1;
            machine_class = VICE_MACHINE_PLUS4;
        }

    }

    LOG(("detect_input_file loadfile_is_crt:%d machine_class:%d\n",
           loadfile_is_crt, machine_class));

    return 0;
}

static int detect_load_address(char *filename, unsigned int *loadaddress, unsigned int *firstbank)
{
    FILE *f;
    unsigned char buffer[0x10];

    if (loadfile_is_crt) {
        f = fopen(filename, "rb");
        if (f == NULL) {
            fprintf(stderr, "Error: Can't open %s\n", filename);
            return -1;
        }
        fseek(f, 0x40, SEEK_SET); /* skip header */
        /* get CHIP header */
        if (fread(buffer, 1, 16, f) != 16) {
            fprintf(stderr, "Error: could not read data from file.\n");
            fclose(f);
            return -1;
        }
        if (buffer[0] != 'C' ||
            buffer[1] != 'H' ||
            buffer[2] != 'I' ||
            buffer[3] != 'P') {
            fprintf(stderr, "Error: CHIP tag not found.\n");
            fclose(f);
            return -1;
        }
        /* set load address to the load address of first CHIP in the file. this is not quite
           correct, but works ok for the few cases when it matters */
        *loadaddress = (buffer[CRT_CHIP_OFFS_LOAD_HI] << 8) + buffer[CRT_CHIP_OFFS_LOAD_LO];
        *firstbank = (buffer[CRT_CHIP_OFFS_BANK_HI] << 8) + buffer[CRT_CHIP_OFFS_BANK_LO];
        fclose(f);
        return 0;
    }
    return -1;
}

int load_input_file(char *filename)
{
    if (detect_input_file(filename) < 0) {
        return -1;
    }

    /* FIXME: from the following code remove everything already done by detect_input_file */

    loadfile_offset = 0;
    infile = fopen(filename, "rb");
    if (infile == NULL) {
        fprintf(stderr, "Error: Can't open %s\n", filename);
        return -1;
    }
    /* fill buffer with 0xff, like empty eproms */
    memset(filebuffer, 0xff, CARTRIDGE_SIZE_MAX);
    /* read first 16 bytes */
    if (fread(filebuffer, 1, 16, infile) != 16) {
        fprintf(stderr, "Error: Can't read %s\n", filename);
        fclose(infile);
        return -1;
    }

    if (loadfile_is_crt) {

        if (fread(headerbuffer + 0x10, 1, 0x30, infile) != 0x30) {
            fprintf(stderr, "Error: Can't read the full header of %s\n", filename);
            fclose(infile);
            return -1;
        }

        if (headerbuffer[0x10] != 0 ||
            headerbuffer[0x11] != 0 ||
            headerbuffer[0x12] != 0 ||
            headerbuffer[0x13] != CRT_HEADER_LEN) {
            fprintf(stderr, "Error: Illegal header size in %s\n", filename);
            if (!repair_mode) {
                fclose(infile);
                return -1;
            }
        }

        if (machine_class == VICE_MACHINE_C64) {
            if (headerbuffer[0x18] == 1 && headerbuffer[0x19] == 0) {
                loadfile_is_ultimax = 1;
            } else {
                loadfile_is_ultimax = 0;
            }
        }

        loadfile_cart_type = headerbuffer[0x17] + (headerbuffer[0x16] << 8);
        if (headerbuffer[0x17] & 0x80) {
            /* handle our negative test IDs */
            loadfile_cart_type -= 0x10000;
        }

        if (machine_class == VICE_MACHINE_C64) {
            if (!((loadfile_cart_type >= 0) && (loadfile_cart_type <= CARTRIDGE_LAST))) {
                fprintf(stderr, "Error: Unknown CRT ID: %d\n", loadfile_cart_type);
                fclose(infile);
                return -1;
            }
        } else if (machine_class == VICE_MACHINE_C128) {
            if (!((loadfile_cart_type >= 0) && (loadfile_cart_type <= CARTRIDGE_C128_LAST))) {
                fprintf(stderr, "Error: Unknown CRT ID: %d\n", loadfile_cart_type);
                fclose(infile);
                return -1;
            }
        } else if ((machine_class == VICE_MACHINE_CBM5x0) ||
                   (machine_class == VICE_MACHINE_CBM6x0)) {
            if (!((loadfile_cart_type >= 0) && (loadfile_cart_type <= CARTRIDGE_CBM2_LAST))) {
                fprintf(stderr, "Error: Unknown CRT ID: %d\n", loadfile_cart_type);
                fclose(infile);
                return -1;
            }
        } else if (machine_class == VICE_MACHINE_VIC20) {
            if (!((loadfile_cart_type >= 0) && (loadfile_cart_type <= CARTRIDGE_VIC20_LAST))) {
                fprintf(stderr, "Error: Unknown CRT ID: %d\n", loadfile_cart_type);
                fclose(infile);
                return -1;
            }
        } else if (machine_class == VICE_MACHINE_PLUS4) {
            if (!((loadfile_cart_type >= 0) && (loadfile_cart_type <= CARTRIDGE_PLUS4_LAST))) {
                fprintf(stderr, "Error: Unknown CRT ID: %d\n", loadfile_cart_type);
                fclose(infile);
                return -1;
            }
        }

        loadfile_size = 0;
        if (load_all_banks_from_crt() < 0) {
            if (repair_mode) {
                fprintf(stderr, "Warning: Can't load all banks of %s\n", filename);
                fclose(infile);
                return 0;
            } else {
                fprintf(stderr, "Error: Can't load all banks of %s (use -r to force)\n", filename);
                fclose(infile);
                return -1;
            }
        } else {
            fclose(infile);
            return 0;
        }
    } else {
        loadfile_is_crt = 0;
        /* read the rest of the file */
        loadfile_size = (unsigned int)fread(filebuffer + 0x10, 1, CARTRIDGE_SIZE_MAX - 14, infile) + 0x10;
        LOG(("loadfile_size: %06x\n", loadfile_size));
        switch (loadfile_size) {
            case CARTRIDGE_SIZE_2KB:
            case CARTRIDGE_SIZE_4KB:
            case CARTRIDGE_SIZE_8KB:
            case CARTRIDGE_SIZE_12KB:
            case CARTRIDGE_SIZE_16KB:
            case CARTRIDGE_SIZE_20KB:
            case CARTRIDGE_SIZE_24KB:
            case CARTRIDGE_SIZE_32KB:
            case CARTRIDGE_SIZE_64KB:
            case CARTRIDGE_SIZE_96KB:
            case CARTRIDGE_SIZE_128KB:
            case CARTRIDGE_SIZE_256KB:
            case CARTRIDGE_SIZE_512KB:
            case CARTRIDGE_SIZE_1024KB:
            case CARTRIDGE_SIZE_2048KB:
            case CARTRIDGE_SIZE_4096KB:
            case CARTRIDGE_SIZE_8192KB:
            case CARTRIDGE_SIZE_16384KB:
                loadfile_offset = 0;
                fclose(infile);
                return 0;
                break;
            case CARTRIDGE_SIZE_2KB + 2:
            case CARTRIDGE_SIZE_4KB + 2:
            case CARTRIDGE_SIZE_8KB + 2:
            case CARTRIDGE_SIZE_12KB + 2:
            case CARTRIDGE_SIZE_16KB + 2:
            case CARTRIDGE_SIZE_20KB + 2:
            case CARTRIDGE_SIZE_24KB + 2:
            case CARTRIDGE_SIZE_32KB + 2:
            case CARTRIDGE_SIZE_64KB + 2:
            case CARTRIDGE_SIZE_96KB + 2:
            case CARTRIDGE_SIZE_128KB + 2:
            case CARTRIDGE_SIZE_256KB + 2:
            case CARTRIDGE_SIZE_512KB + 2:
            case CARTRIDGE_SIZE_1024KB + 2:
            case CARTRIDGE_SIZE_2048KB + 2:
            case CARTRIDGE_SIZE_4096KB + 2:
            case CARTRIDGE_SIZE_8192KB + 2:
            case CARTRIDGE_SIZE_16384KB + 2:
                loadfile_size -= 2;
                loadfile_offset = 2;
                load_address = filebuffer[0] + (filebuffer[1] << 8);
                fclose(infile);
                return 0;
                break;
            /* FIXME: wth is this supposed to be? skip 4 bytes why? */
            case CARTRIDGE_SIZE_32KB + 4:
                loadfile_size -= 4;
                loadfile_offset = 4;
                fclose(infile);
                return 0;
                break;
            default:
                fclose(infile);
                if (input_padding) {
                    return 0;
                }
                fprintf(stderr, "Error: Illegal file size of %s\n", filename);
                return -1;
                break;
        }
    }
}

/*****************************************************************************/
static int find_crtid_from_type(const cart_t *info, char *type)
{
    int i;
    for (i = 0; info[i].name != NULL; i++) {
        if (info[i].opt != NULL) {
            /* FIXME:   Non-standard function
             *          We can't use util_strcasecmp() here either since that'll
             *          make cartconv depend on a lot of other code.
             */
            if (!strcasecmp(info[i].opt, type)) {
                return i; /* found */
            }
        }
    }
    return -1;
}

const cart_t *find_cartinfo_from_crtid(int crtid, int machine)
{
    const cart_t *info = cart_info;
    int i;

    switch (machine) {
        case VICE_MACHINE_C128:
            info = cart_info_c128;
            break;
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
            info = cart_info_cbm2;
            break;
        case VICE_MACHINE_PLUS4:
            info = cart_info_plus4;
            break;
        case VICE_MACHINE_VIC20:
            info = cart_info_vic20;
            break;
    }

    for (i = 0; info[i].name != NULL; i++) {
        if (i == crtid) {
            return &info[i];
        }
    }
    return NULL;
}

static unsigned long printbanks(char *name, int silent)
{
    FILE *f;
    unsigned char b[0x10];
    long len, filelen;
    long pos;
    unsigned int type, bank, start, size;
    char *typestr[CRT_CHIP_TYPES_NUM + 1] = { "ROM", "RAM", "FLASH", "EEPROM", "UNK" };
    unsigned int numbanks;
    unsigned long tsize;

    f = fopen(name, "rb");
    fseek(f, 0, SEEK_END);
    filelen = ftell(f);

    tsize = 0; numbanks = 0;
    if (f) {
        fseek(f, CRT_HEADER_LEN, SEEK_SET); /* skip crt header */
        pos = CRT_HEADER_LEN;
        if (!silent) {
            printf("\noffset  sig  type  bank start size  chunklen\n");
        }
        while (!feof(f)) {
            fseek(f, pos, SEEK_SET);
            /* get chip header */
            if (fread(b, 1, 0x10, f) < 0x10) {
                break;
            }
            len = (b[7] + (b[6] * 0x100) + (b[5] * 0x10000) + (b[4] * 0x1000000));
            type = (unsigned int)((b[8] * 0x100) + b[9]);
            bank = (unsigned int)((b[10] * 0x100) + b[11]);
            start = (unsigned int)((b[12] * 0x100) + b[13]);
            size = (unsigned int)((b[14] * 0x100) + b[15]);
            if (type > CRT_CHIP_TYPES_MAX) {
                type = CRT_CHIP_TYPES_MAX + 1; /* invalid */
            }
            if (!silent) {
                printf("$%06lx %-1c%-1c%-1c%-1c %-5s #%03u $%04x $%04x $%04lx\n",
                        (unsigned long)pos, b[0], b[1], b[2], b[3],
                        typestr[type], bank, start, size, (unsigned long)len);
            }
            if ((size + 0x10) > len) {
                if (!silent) {
                    printf("  Error: data size exceeds chunk length\n");
                }
            }
            if (len > (filelen - pos)) {
                if (!silent) {
                    printf("  Error: data size exceeds end of file\n");
                }
                break;
            }
            pos += len;
            numbanks++;
            tsize += size;
        }
        fclose(f);
        if (!silent) {
            printf("\ntotal banks: %u size: $%06lx\n", numbanks, tsize);
        }
    }
    return tsize;
}

/* check if game/exrom fields in the header are correctly set, handling some
   of the odd exceptions */
static void checkgameexrom(char *name, int crtid, char **game, char **exrom)
{
    if (crtid < 0) {
        return;
    }

    switch (machine_class) {
        case VICE_MACHINE_C64:
            if (crtid == 0) {
                /* for generic cartridge having both zero is broken */
                if ((headerbuffer[0x18] == 0x01) && (headerbuffer[0x19] == 0x01)) {
                    *exrom = "Warning: exrom in crt image set incorrectly";
                    *game = "Warning: game in crt image set incorrectly";
                }
            } else if ((crtid == CARTRIDGE_OCEAN) &&
                        (printbanks(name, 1) == CARTRIDGE_SIZE_512KB)) {
                /* 512k Ocean uses a different startup config than whats in the table */
                if (headerbuffer[0x18] != 0x00) {
                    *exrom = "Warning: exrom in crt image set incorrectly";
                }
                if (headerbuffer[0x19] != 0x01) {
                    *game = "Warning: game in crt image set incorrectly";
                }
            } else {
                if (headerbuffer[0x18] != cart_info[crtid].exrom) {
                    *exrom = "Warning: exrom in crt image set incorrectly";
                }
                if (headerbuffer[0x19] != cart_info[crtid].game) {
                    *game = "Warning: game in crt image set incorrectly";
                }
            }
        break;
        case VICE_MACHINE_C128:
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
            /* game and exrom bytes are only used for C64 cartridges */
            if (headerbuffer[0x18] != 0) {
                *exrom = "Warning: exrom in crt image set incorrectly";
            }
            if (headerbuffer[0x19] != 0) {
                *game = "Warning: game in crt image set incorrectly";
            }
        break;
    }
}

static void printinfo(char *name)
{
    int crtid;
    char *idname = "unknown";
    char *modename = NULL;  /* initialize to avoid GCC warnings */
    char systemname[0x20 + 1];
    char cartname[0x20 + 1];
    char *exrom_warning = NULL;
    char *game_warning = NULL;

    if (detect_input_file(name) < 0) {
        fprintf(stderr, "Error: can not detect file type.\n");
    }

    if (loadfile_is_crt == 1) {
        crtid = headerbuffer[0x17] + (headerbuffer[0x16] << 8);
        if (headerbuffer[0x17] & 0x80) {
            /* handle our negative test IDs */
            crtid -= 0x10000;
        }

        switch (machine_class) {
            case VICE_MACHINE_C64:
                if ((crtid >= 0) && (crtid <= CARTRIDGE_LAST)) {
                    idname = cart_info[crtid].name;
                }
                if ((headerbuffer[0x18] == 1) && (headerbuffer[0x19] == 0)) {
                    modename = "ultimax";
                } else if ((headerbuffer[0x18] == 0) && (headerbuffer[0x19] == 0)) {
                    modename = "16k Game";
                } else if ((headerbuffer[0x18] == 0) && (headerbuffer[0x19] == 1)) {
                    modename = "8k Game";
                } else {
                    modename = "?";
                }
            break;
            case VICE_MACHINE_C128:
                if ((crtid >= 0) && (crtid <= CARTRIDGE_C128_LAST)) {
                    idname = cart_info_c128[crtid].name;
                }
            break;
            case VICE_MACHINE_CBM5x0:
            case VICE_MACHINE_CBM6x0:
                if ((crtid >= 0) && (crtid <= CARTRIDGE_CBM2_LAST)) {
                    idname = cart_info_cbm2[crtid].name;
                }
            break;
            case VICE_MACHINE_VIC20:
                if ((crtid >= 0) && (crtid <= CARTRIDGE_VIC20_LAST)) {
                    idname = cart_info_vic20[crtid].name;
                }
            break;
            case VICE_MACHINE_PLUS4:
                if ((crtid >= 0) && (crtid <= CARTRIDGE_PLUS4_LAST)) {
                    idname = cart_info_plus4[crtid].name;
                }
            break;
        }
        checkgameexrom(name, crtid, &game_warning, &exrom_warning);

        memcpy(systemname, &headerbuffer[0], 0x10); systemname[0x10] = 0;
        if (systemname[0x0f] == 0x20) { systemname[0x0f] = 0; }
        if (systemname[0x0e] == 0x20) { systemname[0x0e] = 0; }
        memcpy(cartname, &headerbuffer[0x20], 0x20); cartname[0x20] = 0;

        printf("CRT Version: %d.%d (%s)\n", headerbuffer[0x14], headerbuffer[0x15], systemname);
        printf("Name: %s\n", cartname);
        printf("Hardware ID: %d (%s)\n", crtid, idname);
        printf("Hardware Revision: %d\n", headerbuffer[0x1a]);

        if (machine_class == VICE_MACHINE_C64) {
            printf("Mode: exrom: %d game: %d (%s)\n", headerbuffer[0x18], headerbuffer[0x19], modename);
        }

        if (exrom_warning) {
            printf("%s.\n", exrom_warning);
        }
        if (game_warning) {
            printf("%s.\n", game_warning);
        }

        if (load_input_file(name) < 0) {
            printf("Error: this file seems broken.\n\n");
        }
        printbanks(name, 0);
    }

    exit (0);
}

static void check_file(char *name)
{
    int crtid;
    char *errormsg = NULL;

    if (detect_input_file(name) < 0) {
        errormsg = "can not detect file type";
    } else {

        if (loadfile_is_crt == 1) {
            crtid = headerbuffer[0x17] + (headerbuffer[0x16] << 8);
            if (headerbuffer[0x17] & 0x80) {
                /* handle our negative test IDs */
                crtid -= 0x10000;
            }

            switch (machine_class) {
                case VICE_MACHINE_C64:
                    if ((crtid >= 0) && (crtid <= CARTRIDGE_LAST)) {
                        /* crt id is valid */
                    } else {
                        errormsg = "invalid CRT ID";
                    }
                    checkgameexrom(name, crtid, &errormsg, &errormsg);
                break;
                case VICE_MACHINE_C128:
                    if ((crtid >= 0) && (crtid <= CARTRIDGE_C128_LAST)) {
                        /* crt id is valid */
                    } else {
                        errormsg = "invalid CRT ID";
                    }
                break;
                case VICE_MACHINE_CBM5x0:
                case VICE_MACHINE_CBM6x0:
                    if ((crtid >= 0) && (crtid <= CARTRIDGE_CBM2_LAST)) {
                        /* crt id is valid */
                    } else {
                        errormsg = "invalid CRT ID";
                    }
                break;
                case VICE_MACHINE_VIC20:
                    if ((crtid >= 0) && (crtid <= CARTRIDGE_VIC20_LAST)) {
                        /* crt id is valid */
                    } else {
                        errormsg = "invalid CRT ID";
                    }
                break;
                case VICE_MACHINE_PLUS4:
                    if ((crtid >= 0) && (crtid <= CARTRIDGE_PLUS4_LAST)) {
                        /* crt id is valid */
                    } else {
                        errormsg = "invalid CRT ID";
                    }
                break;
            }

            if (load_input_file(name) < 0) {
                errormsg = "this file seems broken";
            }
        } else {
            /* TODO: check .bin/.prg */
        }
    }

    if (errormsg) {
        fprintf(stderr, "Error: %s.\n", errormsg);
        exit (-1);
    }

    exit (0);
}


static void printoptions(char *inputname, char *optionsname)
{
    const cart_t *cartinfo;
    char cartname[0x20 + 1];
    FILE *f;
    int crtid;
    int i;

    f = fopen(optionsname, "w");

#if 1
    if (detect_input_file(inputname) < 0) {
        printf("Error: can not detect file type.\n\n");
    }
#endif

    LOG(("printoptions '%s' '%s' %d\n", inputname, optionsname, loadfile_is_crt));

    crtid = headerbuffer[0x17] + (headerbuffer[0x16] << 8);
    if (headerbuffer[0x17] & 0x80) {
        /* handle our negative test IDs */
        crtid -= 0x10000;
    }

    /* cartinfo = find_cartinfo_from_crtid(cart_type, machine_class); */
    cartinfo = find_cartinfo_from_crtid(crtid, machine_class);

    memcpy(cartname, &headerbuffer[0x20], 0x20); cartname[0x20] = 0;

    LOG(("crtid: %d exrom: %d game: %d machine: %d name '%s'\n",
            crtid, headerbuffer[CRT_HEADER_EXROM], headerbuffer[CRT_HEADER_GAME], machine_class, inputname));

    if (loadfile_is_crt == 1) {
        if (crtid > 0) {
            if (cartinfo) {
                fprintf(f, "-t,%s,", cartinfo->opt);
            }
        } else {
            /* generic cartridges */
            if (machine_class == VICE_MACHINE_C64) {
                if ((headerbuffer[CRT_HEADER_EXROM] == 1) && (headerbuffer[CRT_HEADER_GAME] == 0)) {
                    fprintf(f,"-t,ulti,");
                } else if ((headerbuffer[CRT_HEADER_EXROM] == 0) && (headerbuffer[CRT_HEADER_GAME] == 0)) {
                    fprintf(f,"-t,normal,"); /* 16k */
                } else if ((headerbuffer[CRT_HEADER_EXROM] == 0) && (headerbuffer[CRT_HEADER_GAME] == 1)) {
                    fprintf(f,"-t,normal,"); /* 8k */
                } else {
                    fprintf(f,"-t,normal,"); /* invalid (off) */
                }
            } else if (machine_class == VICE_MACHINE_C128) {
                fprintf(f,"-t,c128,");
                if (convert_to_bin) {
                    unsigned int loadaddr = 0, firstbank = 0;
                    detect_load_address(inputname, &loadaddr, &firstbank);
                    fprintf(f,"-l,%u,", loadaddr);
                }
            } else if ((machine_class == VICE_MACHINE_CBM5x0) ||
                       (machine_class == VICE_MACHINE_CBM6x0)) {
                fprintf(f,"-t,cbm2,");
                if (convert_to_bin) {
                    unsigned int loadaddr = 0, firstbank = 0;
                    detect_load_address(inputname, &loadaddr, &firstbank);
                    fprintf(f,"-l,%u,", loadaddr);
                }
            } else if (machine_class == VICE_MACHINE_VIC20) {
                fprintf(f,"-t,vic20,");
                if (convert_to_bin) {
                    unsigned int loadaddr = 0, firstbank = 0;
                    detect_load_address(inputname, &loadaddr, &firstbank);
                    fprintf(f,"-l,%u,", loadaddr);
                }
            } else if (machine_class == VICE_MACHINE_PLUS4) {
                unsigned int loadaddr = 0, firstbank = 0;
                detect_load_address(inputname, &loadaddr, &firstbank);
                if (firstbank == 1) {
                    if (loadaddr >= 0xc000) {
                        fprintf(f,"-t,c2hi,");
                    } else {
                        fprintf(f,"-t,c2lo,");
                    }
                } else {
                    fprintf(f,"-t,plus4,");
                }
                if (convert_to_bin) {
                    fprintf(f,"-l,%u,", loadaddr);
                }
            }
        }
        if (headerbuffer[0x1a]) {
            /* subtype */
            fprintf(f, "-s,%d,", headerbuffer[0x1a]);
        }
        fprintf(f, "-n,%s,", cartname);
        for (i = 0; i < output_filenames; i++) {
            fprintf(f, "-i,%s,", output_filename[i]);
        }
        /* fprintf(f, "-o %s", input_filename); */
        fprintf(f, "\n");
    }
    fclose(f);
}

/*****************************************************************************/

typedef struct sorted_cart_s {
    char *opt;
    char *name;
    int crt_id;
    int insertion;
    int exrom, game;
} sorted_cart_t;

static unsigned int count_valid_option_elements(const cart_t *info)
{
    unsigned int i = 1;
    unsigned int amount = 0;

    while (info[i].name) {
        if (info[i].opt) {
            amount++;
        }
        i++;
    }
    return amount;
}

static int compare_elements(const void *op1, const void *op2)
{
    const sorted_cart_t *p1 = (const sorted_cart_t *)op1;
    const sorted_cart_t *p2 = (const sorted_cart_t *)op2;

    return strcmp(p1->opt, p2->opt);
}

static void print_types(int machine, const cart_t *info)
{
    unsigned int i = 1;
    int n = 0;
    unsigned int amount;
    sorted_cart_t *sorted_option_elements;

    /* get the amount of valid options, excluding crt id 0 */
    amount = count_valid_option_elements(info);
    sorted_option_elements = malloc(amount * sizeof(sorted_cart_t));

    /* fill in the array with the information needed */
    while (info[i].name) {
        if (info[i].opt) {
            sorted_option_elements[n].opt = info[i].opt;
            sorted_option_elements[n].name = info[i].name;
            sorted_option_elements[n].exrom = info[i].exrom;
            sorted_option_elements[n].game = info[i].game;
            sorted_option_elements[n].crt_id = (int)i;
            sorted_option_elements[n].insertion = 0;
            /* FIXME: put this info into the cartridge list */
            if (machine == VICE_MACHINE_C64) {
                switch (i) {
                    case CARTRIDGE_DELA_EP7x8:
                    case CARTRIDGE_DELA_EP64:
                    case CARTRIDGE_REX_EP256:
                    case CARTRIDGE_DELA_EP256:
                        sorted_option_elements[n].insertion = 1;
                        break;
                }
            }
            n++;
        }
        i++;
    }

    qsort(sorted_option_elements, amount, sizeof(sorted_cart_t), compare_elements);

    /* output the sorted list */
    for (i = 0; i < amount; i++) {
        n = sorted_option_elements[i].insertion;
        printf("%-8s %2d ",
               sorted_option_elements[i].opt,
               sorted_option_elements[i].crt_id);
        if (verbose) {
            if (machine == VICE_MACHINE_C64) {
                if ((sorted_option_elements[i].exrom == 1) && (sorted_option_elements[i].game == 0)) {
                    printf("  ultimax  ");
                } else if ((sorted_option_elements[i].exrom == 0) && (sorted_option_elements[i].game == 0)) {
                    printf(" 16k Game  ");
                } else if ((sorted_option_elements[i].exrom == 0) && (sorted_option_elements[i].game == 1)) {
                    printf("  8k Game  ");
                } else {
                    printf("      off  ");
                }
            } else {
                    printf("           ");
            }
        }
        printf("%s .crt file%s\n",
               sorted_option_elements[i].name,
               n ? ", extra files can be inserted" : "");
    }
    free(sorted_option_elements);
}

static void usage_types(void)
{
    cleanup();
    printf("supported cartridge types:\n\n"
           "bin      Binary .bin file (Default crt->bin)\n"
           "prg      Binary CBM .prg file with load-address\n\n"
           "C64 cartridge types:\n\n"
           "normal   Generic 8KiB/12KiB/16KiB .crt file (Default bin->crt)\n"
           "ulti     Ultimax mode 4KiB/8KiB/16KiB .crt file\n\n"
    );

    print_types(VICE_MACHINE_C64, cart_info);

    printf("\nC128 cartridge types:\n\n"
           "c128     Generic 8KiB/16KiB .crt file\n"
    );
    print_types(VICE_MACHINE_C128, cart_info_c128);

    printf("\nCBM2 cartridge types:\n\n"
           "cbm2     Generic 4KiB/8KiB/16KiB/24KiB/32KiB .crt file\n"
    );
    print_types(VICE_MACHINE_CBM6x0, cart_info_cbm2);

    printf("\nVIC20 cartridge types:\n\n"
           "vic20    Generic 8KiB/12KiB/16KiB .crt file\n"
    );
    print_types(VICE_MACHINE_VIC20, cart_info_vic20);

    printf("\nPlus4 cartridge types:\n\n"
           "plus4    Generic 4KiB/8KiB/16KiB/32KiB .crt file\n"
           "c1lo     Generic 4KiB/8KiB/16KiB C1LO .crt file\n"
           "c1hi     Generic 4KiB/8KiB/16KiB C1HI .crt file\n"
           "c2lo     Generic 4KiB/8KiB/16KiB C2LO .crt file\n"
           "c2hi     Generic 4KiB/8KiB/16KiB C2HI .crt file\n"
    );
    print_types(VICE_MACHINE_PLUS4, cart_info_plus4);

    exit(1);
}

static void usage(void)
{
    cleanup();
    printf( "convert:    cartconv [-r] [-q|-v] [-t cart type] [-s cart revision] -i \"input name\" -o \"output name\" [-n \"cart name\"] [-l load address]\n"
            "print info: cartconv [-r] [-q|-v] -f \"input name\"\n\n"
            "check file: cartconv [-r] [-q|-v] -c \"input name\"\n\n"
            "-f <name>                   print info on file\n"
            "-c --check <name>           check file\n"
            "-r                          repair mode (accept broken/invalid input files)\n"
            "-p                          accept non padded binaries as input\n"
            "-b                          output all banks (do not optimize the .crt file)\n"
            "-t <type> or <crtid>        output cart type\n"
            "-s <rev>                    output cart revision/subtype\n"
            "-i <name>                   input filename\n"
            "-o <name>                   output filename\n"
            "-n <name>                   crt cart name\n"
            "-l <addr>                   load address (decimal, use 0x-prefix for hexadecimal, e.g. 0xc000)\n"
            "-q                          quiet\n"
            "-v --verbose                verbose\n"
            "--types                     show the supported cart types\n"
            "--version                   print cartconv version\n"
            "--options-file <filename>   write options for reverting the conversion into a file (for test script)\n");
    exit(1);
}

/*****************************************************************************/

static void unknown(char *opt)
{
    fprintf(stderr, "unknown option: '%s'\n", opt);
}

static void checkarg(char *arg)
{
    if (arg == NULL) {
        usage();
    }
}

/* returns number of arguments to skip */
static int checkflag(char *flg, char *arg)
{
    if (flg[0] == 0) {
        /* skip empty options */
        return 1;
    }

    if (flg[0] != '-') {
        unknown(flg);
        usage();
    }

    if (flg[2] == 0) {
        switch (flg[1]) {
            case 'f':
                printinfo(arg);
                return 2;
            case 'c':
                checkarg(arg);
                check_file(arg);
                return 2;
            case 'r':
                repair_mode = 1;
                return 1;
            case 'b':
                omit_empty_banks = 0;
                return 1;
            case 'q':
                quiet_mode = 1;
                return 1;
            case 'p':
                input_padding = 1;
                return 1;
            case 'v':
                verbose = 1;
                return 1;
            case 'o':
                checkarg(arg);
                if (output_filenames == MAX_OUTPUT_FILES) {
                    usage();
                }
                output_filename[output_filenames] = strdup(arg);
                output_filenames++;
                return 2;
            case 'n':
                checkarg(arg);
                if (cart_name == NULL) {
                    cart_name = strdup(arg);
                } else {
                    usage();
                }
                return 2;
            case 'l':
            {
                char *endptr = NULL;
                checkarg(arg);
                load_address = (int)strtoul(arg, &endptr, 0);
                if (strlen(arg) != (endptr - arg)) {
                    fprintf(stderr, "ERROR: invalid characters in number '%s'.\n", arg);
                    exit(-1);
                }
                if (load_address == 0) {
                    fprintf(stderr, "WARNING: load address is 0, are you sure?\n");
                }
                return 2;
            }
            case 's':
                checkarg(arg);
                if (cart_subtype == 0) {
                    cart_subtype = atoi(arg);
                } else {
                    usage();
                }
                return 2;
            /* set cartridge type (and machine) */
            case 't':
                checkarg(arg);
                if ((cart_type != -1) ||
                    (convert_to_bin != 0) ||
                    (convert_to_prg != 0) ||
                    (convert_to_ultimax != 0)) {
                    usage();
                } else {
                    cart_type = strtoul(arg, NULL, 0);
                    if ((cart_type > 0) && (cart_type <= CARTRIDGE_LAST)) {
                        /* accept numeric id for C64 cartridges */
                    } else {
                        cart_type = find_crtid_from_type(cart_info, arg);
                        if (cart_type == -1) {
                            cart_type = find_crtid_from_type(cart_info_vic20, arg);
                            if (cart_type != -1) {
                                /* found vic20 cartridge */
                                machine_class = VICE_MACHINE_VIC20;
                            }
                        }
                        if (cart_type == -1) {
                            cart_type = find_crtid_from_type(cart_info_c128, arg);
                            if (cart_type != -1) {
                                /* found c128 cartridge */
                                machine_class = VICE_MACHINE_C128;
                            }
                        }
                        if (cart_type == -1) {
                            cart_type = find_crtid_from_type(cart_info_plus4, arg);
                            if (cart_type != -1) {
                                /* found plus4 cartridge */
                                if (load_address == 0) {
                                    load_address = 0x8000;
                                }
                                machine_class = VICE_MACHINE_PLUS4;
                            }
                        }
                        if (cart_type == -1) {
                            cart_type = find_crtid_from_type(cart_info_cbm2, arg);
                            if (cart_type != -1) {
                                /* found cbm2 cartridge */
                                machine_class = VICE_MACHINE_CBM6x0;
                            }
                        }
                    }

                    if (cart_type == -1) {
                        if (!strcmp(arg, "bin")) {
                            convert_to_bin = 1;
                        } else if (!strcmp(arg, "normal")) {
                            cart_type = CARTRIDGE_CRT;
                        } else if (!strcmp(arg, "prg")) {
                            convert_to_prg = 1;
                        } else if (!strcmp(arg, "ulti")) {
                            cart_type = CARTRIDGE_CRT;
                            convert_to_ultimax = 1;
                        } else if (!strcmp(arg, "c128")) {
                            cart_type = CARTRIDGE_CRT;
                            machine_class = VICE_MACHINE_C128;
                        } else if (!strcmp(arg, "cbm2")) {
                            cart_type = CARTRIDGE_CRT;
                            machine_class = VICE_MACHINE_CBM6x0;
                        } else if (!strcmp(arg, "vic20")) {
                            cart_type = CARTRIDGE_CRT;
                            machine_class = VICE_MACHINE_VIC20;
                        } else if (!strcmp(arg, "plus4")) {
                            cart_type = CARTRIDGE_CRT;
                            if (load_address == 0) {
                                load_address = 0x8000;
                            }
                            machine_class = VICE_MACHINE_PLUS4;
                        } else if (!strcmp(arg, "c1lo")) {
                            cart_type = CARTRIDGE_CRT;
                            if (load_address == 0) {
                                load_address = 0x8000;
                            }
                            machine_class = VICE_MACHINE_PLUS4;
                        } else if (!strcmp(arg, "c1hi")) {
                            cart_type = CARTRIDGE_CRT;
                            if (load_address == 0) {
                                load_address = 0xc000;
                            }
                            machine_class = VICE_MACHINE_PLUS4;
                        } else if (!strcmp(arg, "c2lo")) {
                            cart_type = CARTRIDGE_CRT;
                            if (load_address == 0) {
                                load_address = 0x8000;
                            }
                            convert_to_c2 = 1;
                            machine_class = VICE_MACHINE_PLUS4;
                        } else if (!strcmp(arg, "c2hi")) {
                            cart_type = CARTRIDGE_CRT;
                            if (load_address == 0) {
                                load_address = 0xc000;
                            }
                            convert_to_c2 = 1;
                            machine_class = VICE_MACHINE_PLUS4;
                        } else {
                            usage();
                        }
                    } else if (cart_type == CARTRIDGE_MAX_BASIC) {
                        convert_to_ultimax = 1;
                    }
                }
                LOG(("-t cart_type: %d machine_class: %d\n", cart_type, machine_class));
                return 2;
            case 'i':
                checkarg(arg);
                if (input_filenames == MAX_INPUT_FILES) {
                    usage();
                }
                input_filename[input_filenames] = strdup(arg);
                input_filenames++;
                return 2;
            default:
                unknown(flg);
                usage();
                break;
        }
    } else {
        if (!strcmp(flg, "--options-file")) {
            checkarg(arg);
            if (options_filename == NULL) {
                options_filename = strdup(arg);
            } else {
                usage();
            }
            return 2;
        } else if(strcmp(flg, "--types") == 0) {
            usage_types();
            return 1;
        } else if(strcmp(flg, "--check") == 0) {
            checkarg(arg);
            check_file(arg);
            return 2;
        } else if(strcmp(flg, "--verbose") == 0) {
            verbose = 1;
            return 1;
        } else if (strcmp(flg, "--version") == 0) {
#ifdef USE_SVN_REVISION
            printf("cartconv (VICE %s SVN r%d)\n", VERSION, VICE_SVN_REV_NUMBER);
#else
            printf("cartconv (VICE %s)\n", VERSION);
#endif
            return 1;
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int i;
    int arg_counter = 1;
    char *flag, *argument;

    if (argc < 2) {
        usage();
        return EXIT_FAILURE;
    }

    for (i = 0; i < MAX_INPUT_FILES; i++) {
        input_filename[i] = NULL;
    }

    while (arg_counter < argc) {
        flag = argv[arg_counter];
        argument = (arg_counter + 1 < argc) ? argv[arg_counter + 1] : NULL;
        arg_counter += checkflag(flag, argument);
    }

    /* check arguments */

    if (output_filenames == 0) {
        fprintf(stderr, "Error: no output filename\n");
        cleanup();
        exit(1);
    }
    if (input_filenames == 0) {
        fprintf(stderr, "Error: no input filename\n");
        cleanup();
        exit(1);
    }
    /* FIXME: check any input filename vs any output filename */
    if (!strcmp(output_filename[0], input_filename[0])) {
        fprintf(stderr, "Error: output filename = input filename\n");
        cleanup();
        exit(1);
    }

    /* if cart type is not given on cmdline and we are loading a crt file, then
       detect its type */
    if (cart_type == -1) {
        if (detect_input_file(input_filename[0]) < 0) {
            printf("Error: can not detect file type.\n\n");
        }
        loadfile_cart_type = headerbuffer[0x17] + (headerbuffer[0x16] << 8);
        if (headerbuffer[0x17] & 0x80) {
            /* handle our negative test IDs */
            loadfile_cart_type -= 0x10000;
        }
    }

    switch (machine_class) {
        case VICE_MACHINE_C64:
            LOG(("c64 input_filenames: %d output_filenames: %d\n",
                input_filenames, output_filenames));
            /* some formats allow more than one input file */
            if ((input_filenames > 1) &&
                (cart_type != CARTRIDGE_DELA_EP64) && (loadfile_cart_type != CARTRIDGE_DELA_EP64) &&
                (cart_type != CARTRIDGE_DELA_EP256) && (loadfile_cart_type != CARTRIDGE_DELA_EP256) &&
                (cart_type != CARTRIDGE_DELA_EP7x8) && (loadfile_cart_type != CARTRIDGE_DELA_EP7x8) &&
                (cart_type != CARTRIDGE_REX_EP256) && (loadfile_cart_type != CARTRIDGE_REX_EP256)
                ) {
                too_many_inputs();
            }
            if (((cart_type == CARTRIDGE_DELA_EP64) || (loadfile_cart_type == CARTRIDGE_DELA_EP64)) &&
                (input_filenames > 3)) {
                too_many_inputs();
            }
            if (((cart_type == CARTRIDGE_DELA_EP7x8) || (loadfile_cart_type == CARTRIDGE_DELA_EP7x8)) &&
                (input_filenames > 8)) {
                too_many_inputs();
            }
            /* some formats allow more than one output file */
            if (output_filenames > 1) {
                too_many_outputs();
            }
        break;
        case VICE_MACHINE_C128:
            LOG(("c128 input_filenames: %d output_filenames: %d\n",
                input_filenames, output_filenames));
            /* some formats allow more than one output file */
            if (output_filenames > 1) {
                too_many_outputs();
            }
        break;
        case VICE_MACHINE_VIC20:
            LOG(("vic20 input_filenames: %d output_filenames: %d cart_type: %d loadfile_cart_type: %d\n",
                input_filenames, output_filenames, cart_type, loadfile_cart_type));
            /* some formats allow more than one input file */
            if ((input_filenames > 1) && (cart_type != CARTRIDGE_CRT)) {
                too_many_inputs();
            }
            /* some formats allow more than one output file */
            if ((output_filenames > 1) && (loadfile_cart_type != CARTRIDGE_CRT)) {
                too_many_outputs();
            }
        break;
        case VICE_MACHINE_PLUS4:
            LOG(("plus4 input_filenames: %d output_filenames: %d cart_type: %d loadfile_cart_type: %d\n",
                input_filenames, output_filenames, cart_type, loadfile_cart_type));
            /* some formats allow more than one input file */
            if ((input_filenames > 1) && (cart_type != CARTRIDGE_CRT)) {
                too_many_inputs();
            }
            /* some formats allow more than one output file */
            if ((output_filenames > 1) && (loadfile_cart_type != CARTRIDGE_CRT)) {
                too_many_outputs();
            }
        break;
    }

    /* do the conversion */
    if (options_filename) {
        printoptions(input_filename[0], options_filename);
    }

    if (load_input_file(input_filename[0]) < 0) {
        cleanup();
        exit(1);
    }

    if (loadfile_is_crt == 1) {
        /* load a .crt file */
        if (cart_type == CARTRIDGE_DELA_EP64 ||
            cart_type == CARTRIDGE_DELA_EP256 ||
            cart_type == CARTRIDGE_DELA_EP7x8 ||
            cart_type == CARTRIDGE_REX_EP256) {
            cart_info[(unsigned char)cart_type].save(0, 0, 0, 0, 0, 0);
        } else {
            if (cart_type == -1) {
                if (save_binary_output_file() < 0) {
                    cleanup();
                    exit(1);
                }
            } else {
                fprintf(stderr, "Error: File is already .crt format\n");
                cleanup();
                exit(1);
            }
        }
    } else {
        /* load binary files */
        const cart_t *info;

        if (cart_type == -1) {
            fprintf(stderr, "Error: File is already in binary format\n");
            cleanup();
            exit(1);
        }
        /* FIXME: the sizes are used in a bitfield, and also by their absolute values. this
                  check is doomed to fail because of that :)
        */

        info = find_cartinfo_from_crtid(cart_type, machine_class);

        if (input_padding) {
            while ((loadfile_size & info->sizes) != loadfile_size) {
                loadfile_size++;
            }
        } else {
            if ((loadfile_size & info->sizes) != loadfile_size) {
                switch (machine_class) {
                    case VICE_MACHINE_C64:
                        fprintf(stderr, "Error: Input file size (%u) doesn't match %s requirements\n",
                                loadfile_size, cart_info[(unsigned char)cart_type].name);
                        break;
                    case VICE_MACHINE_C128:
                        fprintf(stderr, "Error: Input file size (%u) doesn't match %s requirements\n",
                                loadfile_size, cart_info_c128[(unsigned char)cart_type].name);
                        break;
                    case VICE_MACHINE_CBM5x0:
                    case VICE_MACHINE_CBM6x0:
                        fprintf(stderr, "Error: Input file size (%u) doesn't match %s requirements\n",
                                loadfile_size, cart_info_cbm2[(unsigned char)cart_type].name);
                        break;
                    case VICE_MACHINE_VIC20:
                        fprintf(stderr, "Error: Input file size (%u) doesn't match %s requirements\n",
                                loadfile_size, cart_info_vic20[(unsigned char)cart_type].name);
                        break;
                    case VICE_MACHINE_PLUS4:
                        fprintf(stderr, "Error: Input file size (%u) doesn't match %s requirements\n",
                                loadfile_size, cart_info_plus4[(unsigned char)cart_type].name);
                        break;
                }
                if (!repair_mode) {
                    cleanup();
                    exit(1);
                }
            }
        }
        if (info->save != NULL) {
            info->save(info->bank_size,
                        info->banks,
                        info->load_address,
                        info->data_type,
                        info->game,
                        info->exrom);
        }
    }
    return 0;
}
