
/* #define DEBUG_VIC20_SAVER */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "vic20-saver.h"

#ifdef DEBUG_VIC20_SAVER
#define LOG(x)  printf x
#else
#define LOG(x)
#endif

extern unsigned int loadfile_size;
extern int load_address;
extern FILE *outfile;
extern char *input_filename[MAX_INPUT_FILES];
extern int input_filenames;
extern char loadfile_is_crt;

static int save_generic_vic20_bank(unsigned int length, unsigned int banks, unsigned int address, unsigned int type)
{
    unsigned int i;
    unsigned int real_banks = banks;

    LOG(("save_generic_vic20_bank len:%04x banks:%d address:%04x type:%d\n",
           length, banks, address, type));

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
        if (write_chip_package(length, 0, address, 0) < 0) {
            return -1;
        }
        address &= ~0x1fff;
        switch (address) {
            case 0x2000:    /* block 1 */
            case 0x4000:    /* block 2 */
                address += 0x2000;
                break;
            case 0x6000:    /* block 3 */
                address += 0x4000;
                break;
            case 0xa000:    /* block 5 */
            default:
                if ((i + 1) < real_banks) {
                    fprintf(stderr, "Error: Block %u has invalid address: 0x%04x\n",
                            (i + 1), address);
                    return -1;
                }
                break;
        }
    }
    return 0;
}

static void save_generic_vic20(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    unsigned int i;
    unsigned int last_loadfile_size;

    LOG(("save_generic_vic20  loadfile_size: %x cart length:%x banks:%u load@:%02x chiptype:%u\n",
            loadfile_size, length, banks, address, type));

    if (write_crt_header(0, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (save_generic_vic20_bank(length, banks, address, type) < 0) {
        goto exiterror;
    }

    /* load and write extra input files */
    if (input_filenames > 1) {
        for (i = 1; i < input_filenames; i++) {
            last_loadfile_size = loadfile_size;
            LOG(("extra input File: %s\n", input_filename[i]));
            if (load_input_file(input_filename[i]) < 0) {
                goto exiterror;
            }
            LOG(("extra loadfile size: %04x\n", loadfile_size));
            if (loadfile_is_crt == 1) {
                fprintf(stderr, "Error: extra file must be a binary\n");
                goto exiterror;
            }
            if (loadfile_size != last_loadfile_size) {
                fprintf(stdout, "Warning: block size changed from 0x%04x to 0x%04x\n",
                        last_loadfile_size, loadfile_size);
                length = loadfile_size;
            }
            if (save_generic_vic20_bank(length, banks, load_address, type) < 0) {
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

void save_generic_vic20_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    LOG(("save_generic_vic20_crt size:%04x addr:%04x\n", loadfile_size, load_address));
    switch (loadfile_size) {
        case CARTRIDGE_SIZE_2KB:
            save_generic_vic20(0x0800, 0, load_address, 0, 0, 0);
            break;
        case CARTRIDGE_SIZE_4KB:
            save_generic_vic20(0x1000, 0, load_address, 0, 0, 0);
            break;
        case CARTRIDGE_SIZE_8KB:
            save_generic_vic20(0x2000, 1, load_address, 0, 0, 0);
            break;
        case CARTRIDGE_SIZE_12KB:
            save_generic_vic20(0x2000, 2, load_address, 0, 0, 0);
            break;
        case CARTRIDGE_SIZE_16KB:
            save_generic_vic20(0x2000, 2, load_address, 0, 0, 0);
            break;
        case CARTRIDGE_SIZE_24KB:
            save_generic_vic20(0x2000, 3, load_address, 0, 0, 0);
            break;
        case CARTRIDGE_SIZE_32KB:
            save_generic_vic20(0x2000, 4, load_address, 0, 0, 0);
            break;
        default:
            fprintf(stderr, "Error: invalid size for generic VIC20 cartridge\n");
            cleanup();
            exit(1);
            break;
    }
}

