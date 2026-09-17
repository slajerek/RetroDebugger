
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "cbm2-saver.h"

extern unsigned int loadfile_size;
extern int loadfile_offset;
extern int load_address;
extern FILE *outfile;
extern char *input_filename[MAX_INPUT_FILES];
extern int input_filenames;
extern char loadfile_is_crt;

static int save_generic_cbm2_bank(unsigned int banklength, unsigned int address)
{
    unsigned int i;
    unsigned int real_banks;

    /* handle the case when a chip of half/4th the regular size
        is used on an otherwise identical hardware (eg 2k/4k
        chip on a 8k cart)
    */
    if (loadfile_size == (banklength / 2)) {
        banklength /= 2;
    } else if (loadfile_size == (banklength / 4)) {
        banklength /= 4;
    }
    real_banks = loadfile_size / banklength;

    for (i = 0; i < real_banks; i++) {
        if (write_chip_package(banklength, 0, address, CRT_CHIP_ROM) < 0) {
            return -1;
        }
        switch (address) {
            case 0x1000:    /* C1 */
                address = 0x2000;
                break;
            case 0x2000:    /* fall through */
            case 0x4000:
                address += 0x2000;
                break;
            default:
                if ((i + 1) < real_banks) {
                    fprintf(stderr, "Error: invalid block address 0x%04x\n", address);
                    return -1;
                }
                break;
        }
    }
    return 0;
}

static void save_generic_cbm2(unsigned int banklength, unsigned int address)
{
    unsigned int i;
/*
    printf("save_generic_cbm2  loadfile_size: %x cart banklength:%x banks:%u load@:%02x isc2:%u\n",
            loadfile_size, banklength, banks, address, isc2);
*/
    if (write_crt_header(0, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (save_generic_cbm2_bank(banklength, address) < 0) {
        goto exiterror;
    }

    /* load and write extra input files */
    if (input_filenames > 1) {
        for (i = 1; i < input_filenames; i++) {
            /* printf("extra input File: %s\n", input_filename[i]); */
            if (load_input_file(input_filename[i]) < 0) {
                goto exiterror;
            }
            if (loadfile_is_crt == 1) {
                fprintf(stderr, "Error: extra file must be a binary\n");
                goto exiterror;
            }
            if (save_generic_cbm2_bank(banklength, load_address) < 0) {
                goto exiterror;
            }
        }
    }

    bin2crt_ok();
exiterror:
    fclose(outfile);
    cleanup();
    exit(0);
}

void save_generic_cbm2_crt(unsigned int banklength, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    printf("save_generic_cbm2_crt size:%04x addr:%04x\n", loadfile_size, (unsigned)load_address);
    switch (loadfile_size) {
        case CARTRIDGE_SIZE_2KB:
            save_generic_cbm2(0x0800, load_address);
            break;
        case CARTRIDGE_SIZE_4KB:
            save_generic_cbm2(0x1000, load_address);
            break;
        case CARTRIDGE_SIZE_8KB:
            save_generic_cbm2(0x2000, load_address);
            break;
        case CARTRIDGE_SIZE_16KB:
            save_generic_cbm2(0x2000, load_address);
            break;
        case CARTRIDGE_SIZE_32KB:
            save_generic_cbm2(0x2000, load_address);
            break;
        default:
            fprintf(stderr, "Error: invalid size for generic PLUS4 cartridge\n");
            cleanup();
            exit(1);
            break;
    }
}
