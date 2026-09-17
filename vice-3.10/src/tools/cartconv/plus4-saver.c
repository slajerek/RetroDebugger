
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "plus4-saver.h"

extern unsigned int loadfile_size;
extern int loadfile_offset;
extern int load_address;
extern FILE *outfile;
extern char *input_filename[MAX_INPUT_FILES];
extern int input_filenames;
extern char loadfile_is_crt;

extern int convert_to_c2;

static int save_generic_plus4_bank(unsigned int length, unsigned int banks, unsigned int address, unsigned int isc2)
{
    unsigned int i;
    unsigned int real_banks = banks;
    unsigned int chipbank = isc2 ? 1 : 0; /* in generic cart, C2 uses bank 1 */
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
        if (chipbank > 1) {
            fprintf(stderr, "Error: too many banks\n");
            return -1;
        }
        if (write_chip_package(length, chipbank, address, CRT_CHIP_ROM) < 0) {
            return -1;
        }
        switch (address) {
            case 0x8000:    /* lo */
                address = 0xc000;
                break;
            case 0xc000:    /* hi */
                address = 0x8000;
                chipbank++;
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

static void save_generic_plus4(unsigned int length, unsigned int banks, unsigned int address, unsigned int isc2, unsigned char game, unsigned char exrom)
{
    unsigned int i;
/*
    printf("save_generic_plus4  loadfile_size: %x cart length:%x banks:%u load@:%02x isc2:%u\n",
            loadfile_size, length, banks, address, isc2);
*/
    if (write_crt_header(0, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (save_generic_plus4_bank(length, banks, address, isc2) < 0) {
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
            if (save_generic_plus4_bank(length, banks, load_address, isc2) < 0) {
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

void save_generic_plus4_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    printf("save_generic_plus4_crt size:%04x addr:%04x\n", loadfile_size, (unsigned)load_address);
    switch (loadfile_size) {
        case CARTRIDGE_SIZE_2KB:
            save_generic_plus4(0x0800, 0, load_address, convert_to_c2, 0, 0);
            break;
        case CARTRIDGE_SIZE_4KB:
            save_generic_plus4(0x1000, 0, load_address, convert_to_c2, 0, 0);
            break;
        case CARTRIDGE_SIZE_8KB:
            save_generic_plus4(0x2000, 1, load_address, convert_to_c2, 0, 0);
            break;
        case CARTRIDGE_SIZE_16KB:
            save_generic_plus4(0x4000, 1, load_address, convert_to_c2, 0, 0);
            break;
        case CARTRIDGE_SIZE_32KB:
            save_generic_plus4(0x4000, 2, load_address, convert_to_c2, 0, 0);
            break;
        default:
            fprintf(stderr, "Error: invalid size for generic PLUS4 cartridge\n");
            cleanup();
            exit(1);
            break;
    }
}

void save_multicart_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    unsigned int i;
    unsigned int real_banks = loadfile_size / 0x8000;
    unsigned int prg_offset = loadfile_offset;

    /*printf("save_multicart_crt loadfile_size:%04x length:%04x real_banks: %u\n", loadfile_size, length, real_banks);*/

    if (write_crt_header(game, exrom) < 0) {
        cleanup();
        exit(1);
    }

    for (i = 0; i < real_banks; i++) {
        loadfile_offset = prg_offset + (i * 0x4000);
        if (write_chip_package(0x4000, i, 0x8000, CRT_CHIP_ROM) < 0) {
            cleanup();
            exit(1);
        }
        loadfile_offset = prg_offset + (i * 0x4000) + (loadfile_size / 2);
        if (write_chip_package(0x4000, i, 0xc000, CRT_CHIP_ROM) < 0) {
            cleanup();
            exit(1);
        }
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}
