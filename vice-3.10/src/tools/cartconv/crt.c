
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cartridge.h"
#include "machine.h"

#include "crt.h"

#include "cartconv.h"
extern unsigned char cart_subtype;
extern signed char cart_type;
extern char *cart_name;
extern FILE *outfile;
extern char *output_filename[MAX_OUTPUT_FILES];
extern unsigned char filebuffer[CARTRIDGE_SIZE_MAX + 2];
extern int loadfile_offset;
extern int machine_class;

unsigned char headerbuffer[0x40];

/* FIXME: infile is global */
int read_crt_header(char *filename)
{
    FILE *infile = fopen(filename, "rb");
    if (infile == NULL) {
        fprintf(stderr, "Error: Can't open %s\n", filename);
        return -1;
    }

    if (fread(headerbuffer, 1, CRT_HEADER_LEN, infile) != CRT_HEADER_LEN) {
        fprintf(stderr, "Error: Can't read %s\n", filename);
        fclose(infile);
        return -1;
    }

#if 0
    if (headerbuffer[0x10] != 0 ||
        headerbuffer[0x11] != 0 ||
        headerbuffer[0x12] != 0 ||
        headerbuffer[0x13] != 0x40) {
        fprintf(stderr, "Error: Illegal header size in %s\n", filename);
        if (!repair_mode) {
            fclose(infile);
            return -1;
        }
    }
#endif
#if 0
    if (headerbuffer[0x18] == 1 && headerbuffer[0x19] == 0) {
        loadfile_is_ultimax = 1;
    } else {
        loadfile_is_ultimax = 0;
    }

    loadfile_cart_type = headerbuffer[0x17] + (headerbuffer[0x16] << 8);
    if (headerbuffer[0x17] & 0x80) {
        /* handle our negative test IDs */
        loadfile_cart_type -= 0x10000;
    }
    if (!((loadfile_cart_type >= 0) && (loadfile_cart_type <= CARTRIDGE_LAST))) {
        fprintf(stderr, "Error: Unknown CRT ID: %d\n", loadfile_cart_type);
        fclose(infile);
        return -1;
    }
#endif
    fclose(infile);
    return 0;
}

int write_crt_header(unsigned char gameline, unsigned char exromline)
{
    const cart_t *cartinfo;
    unsigned char crt_header[CRT_HEADER_LEN] = "C64 CARTRIDGE   ";
    int endofname = 0;
    int version_hi = 1;
    int version_lo = 0;
    int i;

    if (machine_class == VICE_MACHINE_C64) {
        /* crt version low */
        if (cart_subtype > 0) {
            version_lo = 1;
        }
    } else if (machine_class == VICE_MACHINE_C128) {
        memcpy(crt_header, "C128 CARTRIDGE  ", CRT_NAME_LEN);
        version_hi = 2;
    } else if (machine_class == VICE_MACHINE_CBM5x0 ||
               machine_class == VICE_MACHINE_CBM6x0) {
        memcpy(crt_header, "CBM2 CARTRIDGE  ", CRT_NAME_LEN);
        version_hi = 2;
    } else if (machine_class == VICE_MACHINE_VIC20) {
        memcpy(crt_header, "VIC20 CARTRIDGE ", CRT_NAME_LEN);
        version_hi = 2;
    } else if (machine_class == VICE_MACHINE_PLUS4) {
        memcpy(crt_header, "PLUS4 CARTRIDGE ", CRT_NAME_LEN);
        version_hi = 2;
    }

    /* header length */
    crt_header[0x10] = 0;
    crt_header[0x11] = 0;
    crt_header[0x12] = 0;
    crt_header[0x13] = CRT_HEADER_LEN;

    crt_header[0x14] = version_hi;
    crt_header[0x15] = version_lo;

    crt_header[0x16] = 0;   /* cart type high */
    crt_header[0x17] = (unsigned char)cart_type;

    crt_header[0x18] = exromline;
    crt_header[0x19] = gameline;

    crt_header[0x1a] = cart_subtype;

    /* unused/reserved */
    crt_header[0x1b] = 0;
    crt_header[0x1c] = 0;
    crt_header[0x1d] = 0;
    crt_header[0x1e] = 0;
    crt_header[0x1f] = 0;

    if (cart_name == NULL) {
        /* cart_name = strdup("VICE CART"); */
        cartinfo = find_cartinfo_from_crtid(cart_type, machine_class);
        cart_name = strdup(cartinfo->name);
        if (strlen(cart_name) > 32) {
            cart_name[32] = 0;
        }
    }

    for (i = 0; i < 32; i++) {
        if (endofname == 1) {
            crt_header[0x20 + i] = 0;
        } else {
            if (cart_name[i] == 0) {
                endofname = 1;
            } else {
                crt_header[0x20 + i] = cart_name[i];
            }
        }
    }

    outfile = fopen(output_filename[0], "wb");
    if (outfile == NULL) {
        fprintf(stderr, "Error: Can't open output file %s\n", output_filename[0]);
        return -1;
    }
    if (fwrite(crt_header, 1, 0x40, outfile) != 0x40) {
        fprintf(stderr, "Error: Can't write crt header to file %s\n", output_filename[0]);
        fclose(outfile);
        unlink(output_filename[0]);
        return -1;
    }
    return 0;
}

/* FIXME: loadfile_offset should be a parameter to this function, and not get modified by it */
int write_chip_package(unsigned int length, unsigned int bank, unsigned int address, unsigned char type)
{
    unsigned char chip_header[0x10] = "CHIP";

    chip_header[4] = 0;
    chip_header[5] = 0;
    chip_header[6] = (unsigned char)((length + 0x10) >> 8);
    chip_header[7] = (unsigned char)((length + 0x10) & 0xff);

    chip_header[CRT_CHIP_OFFS_TYPE_HI] = 0;
    chip_header[CRT_CHIP_OFFS_TYPE_LO] = type;

    chip_header[CRT_CHIP_OFFS_BANK_HI] = (unsigned char)(bank >> 8);
    chip_header[CRT_CHIP_OFFS_BANK_LO] = (unsigned char)(bank & 0xff);

    chip_header[CRT_CHIP_OFFS_LOAD_HI] = (unsigned char)(address >> 8);
    chip_header[CRT_CHIP_OFFS_LOAD_LO] = (unsigned char)(address & 0xff);

    chip_header[CRT_CHIP_OFFS_SIZE_HI] = (unsigned char)(length >> 8);
    chip_header[CRT_CHIP_OFFS_SIZE_LO] = (unsigned char)(length & 0xff);
    if (fwrite(chip_header, 1, 0x10, outfile) != 0x10) {
        fprintf(stderr, "Error: Can't write chip header to file %s\n", output_filename[0]);
        fclose(outfile);
        unlink(output_filename[0]);
        return -1;
    }
    if (fwrite(filebuffer + loadfile_offset, 1, length, outfile) != length) {
        fprintf(stderr, "Error: Can't write data to file %s\n", output_filename[0]);
        fclose(outfile);
        unlink(output_filename[0]);
        return -1;
    }
    loadfile_offset += (int)length;
    return 0;
}

