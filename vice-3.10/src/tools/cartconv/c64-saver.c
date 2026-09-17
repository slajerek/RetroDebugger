
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "c64-saver.h"

extern unsigned int loadfile_size;
extern unsigned char filebuffer[CARTRIDGE_SIZE_MAX + 2];
extern FILE *outfile;
extern int loadfile_offset;
extern int omit_empty_banks;
extern char *output_filename[MAX_OUTPUT_FILES];
extern char *input_filename[MAX_INPUT_FILES];
extern int input_filenames;
extern char loadfile_is_crt;
extern int load_address;
extern char loadfile_is_ultimax;
extern int quiet_mode;
extern int loadfile_cart_type;
extern unsigned char extra_buffer_32kb[0x8000];
extern char convert_to_ultimax;

void save_fcplus_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom)
{
    unsigned int i;
    unsigned int real_banks = banks;

    /* printf("save_fcplus_crt length: %d banks:%d address: %d\n", length, banks, address); */

    if (write_crt_header(game, exrom) < 0) {
        cleanup();
        exit(1);
    }

    if (real_banks == 0) {
        real_banks = loadfile_size / length;
    }

    if (loadfile_size != 0x8000) {
        memmove(filebuffer + 0x2000, filebuffer, 0x6000);
        memset(filebuffer, 0xff, 0x2000);
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

void save_2_blocks_crt(unsigned int l1, unsigned int l2, unsigned int a1, unsigned int a2, unsigned char game, unsigned char exrom)
{
    if (write_crt_header(game, exrom) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, (a2 == 0xe000) ? 0xe000 : 0xa000, 0) < 0) {
        cleanup();
        exit(1);
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

static int check_empty_easyflash(void)
{
    int i;

    for (i = 0; i < 0x2000; i++) {
        if (filebuffer[loadfile_offset + i] != 0xff) {
            return 0;
        }
    }
    return 1;
}

void save_easyflash_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    unsigned int i, j;

    if (write_crt_header(0, 1) < 0) {
        cleanup();
        exit(1);
    }

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 2; j++) {
            if ((omit_empty_banks == 1) && (check_empty_easyflash() == 1)) {
                loadfile_offset += 0x2000;
            } else {
                if (write_chip_package(0x2000, i, (j == 0) ? 0x8000 : 0xa000, 2) < 0) {
                    cleanup();
                    exit(1);
                }
            }
        }
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_ocean_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    unsigned int i;
    int banks = loadfile_size / 0x2000;

    if (loadfile_size == CARTRIDGE_SIZE_512KB) {
        /* the 512k type (64 banks) starts uses 8k game mode */
        if (write_crt_header(1, 0) < 0) {
            cleanup();
            exit(1);
        }
    } else {
        /* the other types use 16k game mode */
        if (write_crt_header(0, 0) < 0) {
            cleanup();
            exit(1);
        }
    }

    if (banks == 32) {
        /* for 256k type write the second half with a000 load address */
        for (i = 0; i < 16; i++) {
            if (write_chip_package(0x2000, i, 0x8000, 0) < 0) {
                cleanup();
                exit(1);
            }
        }

        for (i = 0; i < 16; i++) {
            if (write_chip_package(0x2000, i + 16, 0xa000, 0) < 0) {
                cleanup();
                exit(1);
            }
        }
    } else {
        for (i = 0; i < banks; i++) {
            if (write_chip_package(0x2000, i, 0x8000, 0) < 0) {
                cleanup();
                exit(1);
            }
        }
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_funplay_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    unsigned int i = 0;

    if (write_crt_header(1, 0) < 0) {
        cleanup();
        exit(1);
    }

    while (i != 0x41) {
        if (write_chip_package(0x2000, i, 0x8000, 0) < 0) {
            cleanup();
            exit(1);
        }
        i += 8;
        if (i == 0x40) {
            i = 1;
        }
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_easycalc_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    if (write_crt_header(1, 1) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0xa000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 1, 0xa000, 0) < 0) {
        cleanup();
        exit(1);
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_zaxxon_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    if (write_crt_header(0, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x1000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0xa000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 1, 0xa000, 0) < 0) {
        cleanup();
        exit(1);
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

/* 8k ROML at $8000, 8k ROMH at $e000 */
void save_stardos_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char game, unsigned char exrom)
{
    if (write_crt_header(game, exrom) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0xe000, 0) < 0) {
        cleanup();
        exit(1);
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

static void close_output_cleanup(void)
{
    fclose(outfile);
    unlink(output_filename[0]);
    cleanup();
    exit(1);
}

void save_delaep64_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    unsigned int i;

    if (loadfile_size != CARTRIDGE_SIZE_8KB) {
        fprintf(stderr, "Error: wrong size of Dela EP64 base file %s (%u)\n",
                input_filename[0], loadfile_size);
        cleanup();
        exit(1);
    }

    if (write_crt_header(1, 0) < 0) {
        cleanup();
        exit(1);
    }

    /* write base file */
    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (input_filenames > 1) {
        /* write user eproms */
        for (i = 0; i < input_filenames; i++) {
            if (load_input_file(input_filename[i]) < 0) {
                close_output_cleanup();
            }
            if (loadfile_is_crt == 1) {
                fprintf(stderr, "Error: to be inserted file can only be a binary for Dela EP64\n");
                close_output_cleanup();
            }
            if (loadfile_size != CARTRIDGE_SIZE_32KB) {
                fprintf(stderr, "Error: to be inserted file can only be 32KiB in size for Dela EP64\n");
                close_output_cleanup();
            }
            if (write_chip_package(0x8000, i + 1, 0x8000, 0) < 0) {
                close_output_cleanup();
            }
        }
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_delaep256_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    unsigned int i, j;
    unsigned int insert_size = 0;

    if (loadfile_size != CARTRIDGE_SIZE_8KB) {
        fprintf(stderr, "Error: wrong size of Dela EP256 base file %s (%u)\n",
                input_filename[0], loadfile_size);
        cleanup();
        exit(1);
    }

    if (input_filenames == 1) {
        fprintf(stderr, "Error: no files to insert into Dela EP256 .crt\n");
        cleanup();
        exit(1);
    }

    if (write_crt_header(1, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    for (i = 0; i < (unsigned int)input_filenames - 1; i++) {
        if (load_input_file(input_filename[i + 1]) < 0) {
            close_output_cleanup();
        }

        if (loadfile_size != CARTRIDGE_SIZE_32KB && loadfile_size != CARTRIDGE_SIZE_8KB) {
            fprintf(stderr, "Error: only 32KiB binary files or 8KiB bin/crt files can be inserted in Dela EP256\n");
            close_output_cleanup();
        }

        if (insert_size == 0) {
            insert_size = loadfile_size;
        }

        if (insert_size == CARTRIDGE_SIZE_32KB && input_filenames > 8) {
            fprintf(stderr, "Error: a maximum of 8 32KiB images can be inserted\n");
            close_output_cleanup();
        }

        if (insert_size != loadfile_size) {
            fprintf(stderr, "Error: only one type of insertion is allowed at this time for Dela EP256\n");
            close_output_cleanup();
        }

        if (loadfile_is_crt == 1 && (loadfile_size != CARTRIDGE_SIZE_8KB || load_address != 0x8000 || loadfile_is_ultimax == 1)) {
            fprintf(stderr, "Error: you can only insert generic 8KiB .crt files for Dela EP256\n");
            close_output_cleanup();
        }

        if (insert_size == CARTRIDGE_SIZE_32KB) {
            for (j = 0; j < 4; j++) {
                if (write_chip_package(0x2000, (i * 4) + j + 1, 0x8000, 0) < 0) {
                    close_output_cleanup();
                }
            }
            if (!quiet_mode) {
                printf("inserted %s in banks %u-%u of the Dela EP256 .crt\n",
                        input_filename[i + 1], (i * 4) + 1, (i * 4) + 4);
            }
        } else {
            if (write_chip_package(0x2000, i + 1, 0x8000, 0) < 0) {
                close_output_cleanup();
            }
            if (!quiet_mode) {
                printf("inserted %s in bank %u of the Dela EP256 .crt\n",
                        input_filename[i + 1], i + 1);
            }
        }
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_delaep7x8_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    int inserted_size = 0;
    int name_counter = 1;
    unsigned int chip_counter = 1;

    if (loadfile_size != CARTRIDGE_SIZE_8KB) {
        fprintf(stderr, "Error: wrong size of Dela EP7x8 base file %s (%u)\n",
                input_filename[0], loadfile_size);
        cleanup();
        exit(1);
    }

    if (input_filenames == 1) {
        fprintf(stderr, "Error: no files to insert into Dela EP7x8 .crt\n");
        cleanup();
        exit(1);
    }

    if (write_crt_header(1, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    while (name_counter != input_filenames) {
        if (load_input_file(input_filename[name_counter]) < 0) {
            close_output_cleanup();
        }

        if (loadfile_size == CARTRIDGE_SIZE_32KB) {
            if (loadfile_is_crt == 1) {
                fprintf(stderr, "Error: (%s) only binary 32KiB images can be inserted into a Dela EP7x8 .crt\n",
                        input_filename[name_counter]);
                close_output_cleanup();
            } else {
                if (inserted_size != 0) {
                    fprintf(stderr, "Error: (%s) only the first inserted image can be a 32KiB image for Dela EP7x8\n",
                            input_filename[name_counter]);
                    close_output_cleanup();
                } else {
                    if (write_chip_package(0x2000, chip_counter, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (write_chip_package(0x2000, chip_counter + 1, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (write_chip_package(0x2000, chip_counter + 2, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (write_chip_package(0x2000, chip_counter + 3, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (!quiet_mode) {
                        printf("inserted %s in banks %u-%u of the Dela EP7x8 .crt\n",
                               input_filename[name_counter], chip_counter,
                               chip_counter + 3);
                    }
                    chip_counter += 4;
                    inserted_size += 0x8000;
                }
            }
        }

        if (loadfile_size == CARTRIDGE_SIZE_16KB) {
            if (loadfile_is_crt == 1 && (loadfile_cart_type != 0 || loadfile_is_ultimax == 1)) {
                fprintf(stderr, "Error: (%s) only generic 16KiB .crt images can be inserted into a Dela EP7x8 .crt\n",
                        input_filename[name_counter]);
                close_output_cleanup();
            } else {
                if (inserted_size >= 0xc000) {
                    fprintf(stderr, "Error: (%s) no room to insert a 16KiB binary file into the Dela EP7x8 .crt\n",
                            input_filename[name_counter]);
                    close_output_cleanup();
                } else {
                    if (write_chip_package(0x2000, chip_counter, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (write_chip_package(0x2000, chip_counter + 1, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (!quiet_mode) {
                        printf("inserted %s in banks %u and %u of the Dela EP7x8 .crt\n",
                               input_filename[name_counter], chip_counter, chip_counter + 1);
                    }
                    chip_counter += 2;
                    inserted_size += 0x4000;
                }
            }
        }

        if (loadfile_size == CARTRIDGE_SIZE_8KB) {
            if (loadfile_is_crt == 1 && (loadfile_cart_type != 0 || loadfile_is_ultimax == 1)) {
                fprintf(stderr, "Error: (%s) only generic 8KiB .crt images can be inserted into a Dela EP7x8 .crt\n",
                        input_filename[name_counter]);
                close_output_cleanup();
            } else {
                if (inserted_size >= 0xe000) {
                    fprintf(stderr, "Error: (%s) no room to insert a 8KiB binary file into the Dela EP7x8 .crt\n",
                            input_filename[name_counter]);
                    close_output_cleanup();
                } else {
                    if (write_chip_package(0x2000, chip_counter, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (!quiet_mode) {
                        printf("inserted %s in bank %u of the Dela EP7x8 .crt\n",
                                input_filename[name_counter], chip_counter);
                    }
                    chip_counter++;
                    inserted_size += 0x2000;
                }
            }
        }

        name_counter++;
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_rexep256_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    int eprom_size_for_8kb = 0;
    int images_of_8kb_started = 0;
    int name_counter = 1;
    unsigned int chip_counter = 1;
    int subchip_counter = 1;

    if (loadfile_size != CARTRIDGE_SIZE_8KB) {
        fprintf(stderr, "Error: wrong size of Rex EP256 base file %s (%u)\n",
                input_filename[0], loadfile_size);
        cleanup();
        exit(1);
    }

    if (input_filenames == 1) {
        fprintf(stderr, "Error: no files to insert into Rex EP256 .crt\n");
        cleanup();
        exit(1);
    }

    if (write_crt_header(1, 0) < 0) {
        cleanup();
        exit(1);
    }

    if (write_chip_package(0x2000, 0, 0x8000, 0) < 0) {
        cleanup();
        exit(1);
    }

    while (name_counter != input_filenames) {
        if (load_input_file(input_filename[name_counter]) < 0) {
            close_output_cleanup();
        }

        if (chip_counter > 8) {
            fprintf(stderr, "Error: no more room for %s in the Rex EP256 .crt\n", input_filename[name_counter]);
        }

        if (loadfile_size == CARTRIDGE_SIZE_32KB) {
            if (loadfile_is_crt == 1) {
                fprintf(stderr, "Error: (%s) only binary 32KiB images can be inserted into a Rex EP256 .crt\n",
                        input_filename[name_counter]);
                close_output_cleanup();
            } else {
                if (images_of_8kb_started != 0) {
                    fprintf(stderr, "Error: (%s) only the first inserted images can be a 32KiB image for Rex EP256\n",
                            input_filename[name_counter]);
                    close_output_cleanup();
                } else {
                    if (write_chip_package(0x8000, chip_counter, 0x8000, 0) < 0) {
                        close_output_cleanup();
                    }
                    if (!quiet_mode) {
                        printf("inserted %s in bank %u as a 32KiB eprom of the Rex EP256 .crt\n",
                               input_filename[name_counter], chip_counter);
                    }
                    chip_counter++;
                }
            }
        }

        if (loadfile_size == CARTRIDGE_SIZE_8KB) {
            if (loadfile_is_crt == 1 && (loadfile_cart_type != 0 || loadfile_is_ultimax == 1)) {
                fprintf(stderr, "Error: (%s) only generic 8KiB .crt images can be inserted into a Rex EP256 .crt\n",
                        input_filename[name_counter]);
                close_output_cleanup();
            } else {
                if (images_of_8kb_started == 0) {
                    images_of_8kb_started = 1;
                    if ((9 - chip_counter) * 4 < (unsigned int)(input_filenames - name_counter)) {
                        fprintf(stderr, "Error: no room for the amount of input files given\n");
                        close_output_cleanup();
                    }
                    eprom_size_for_8kb = 1;
                    if ((9 - chip_counter) * 2 < (unsigned int)(input_filenames - name_counter)) {
                        eprom_size_for_8kb = 4;
                    }
                    if (9 - chip_counter < (unsigned int)(input_filenames - name_counter)) {
                        eprom_size_for_8kb = 2;
                    }
                }

                if (eprom_size_for_8kb == 1) {
                    if (write_chip_package(0x2000, chip_counter, 0x8000, 0) < 0) {
                        close_output_cleanup();
                        if (!quiet_mode) {
                            printf("inserted %s as an 8KiB eprom in bank %u of the Rex EP256 .crt\n",
                                   input_filename[name_counter], chip_counter);
                        }
                        chip_counter++;
                    }

                    if (eprom_size_for_8kb == 4 && (subchip_counter == 4 || name_counter == input_filenames - 1)) {
                        memcpy(extra_buffer_32kb + ((subchip_counter - 1) * 0x2000), filebuffer + loadfile_offset, 0x2000);
                        memcpy(filebuffer, extra_buffer_32kb, 0x8000);
                        loadfile_offset = 0;
                        if (write_chip_package(0x8000, chip_counter, 0x8000, 0) < 0) {
                            close_output_cleanup();
                        }
                        if (!quiet_mode) {
                            if (subchip_counter == 1) {
                                printf("inserted %s as a 32KiB eprom in bank %u of the Rex EP256 .crt\n",
                                       input_filename[name_counter], chip_counter);
                            } else {
                                printf(" and %s as a 32KiB eprom in bank %u of the Rex EP256 .crt\n",
                                       input_filename[name_counter], chip_counter);
                            }
                        }
                        chip_counter++;
                        subchip_counter = 1;
                    }

                    if (eprom_size_for_8kb == 4 && (subchip_counter == 3 || subchip_counter == 2) &&
                        name_counter != input_filenames) {
                        memcpy(extra_buffer_32kb + ((subchip_counter - 1) * 0x2000), filebuffer + loadfile_offset, 0x2000);
                        if (!quiet_mode) {
                            printf(", %s", input_filename[name_counter]);
                        }
                        subchip_counter++;
                    }

                    if (eprom_size_for_8kb == 2) {
                        if (subchip_counter == 2 || name_counter == input_filenames - 1) {
                            memcpy(extra_buffer_32kb + ((subchip_counter - 1) * 0x2000),
                                   filebuffer + loadfile_offset, 0x2000);
                            memcpy(filebuffer, extra_buffer_32kb, 0x4000);
                            loadfile_offset = 0;
                            if (write_chip_package(0x4000, chip_counter, 0x8000, 0) < 0) {
                                close_output_cleanup();
                            }
                            if (!quiet_mode) {
                                if (subchip_counter == 1) {
                                    printf("inserted %s as a 16KiB eprom in bank %u of the Rex EP256 .crt\n",
                                           input_filename[name_counter], chip_counter);
                                } else {
                                    printf(" and %s as a 16KiB eprom in bank %u of the Rex EP256 .crt\n",
                                           input_filename[name_counter], chip_counter);
                                }
                            }
                            chip_counter++;
                            subchip_counter = 1;
                        } else {
                            memcpy(extra_buffer_32kb, filebuffer + loadfile_offset, 0x2000);
                            if (!quiet_mode) {
                                printf("inserted %s", input_filename[name_counter]);
                            }
                            subchip_counter++;
                        }
                    }

                    if (eprom_size_for_8kb == 4 && subchip_counter == 1 && name_counter != input_filenames) {
                        memcpy(extra_buffer_32kb, filebuffer + loadfile_offset, 0x2000);
                        if (!quiet_mode) {
                            printf("inserted %s", input_filename[name_counter]);
                        }
                        subchip_counter++;
                    }
                }
            }
        }
        name_counter++;
    }

    fclose(outfile);
    bin2crt_ok();
    cleanup();
    exit(0);
}

void save_generic_crt(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned char p5, unsigned char p6)
{
    /* printf("save_generic_crt ultimax: %d size: %08x\n", convert_to_ultimax, loadfile_size); */
    if (convert_to_ultimax == 1) {
        switch (loadfile_size) {
            case CARTRIDGE_SIZE_2KB:
                save_regular_crt(0x0800, 1, 0xf800, 0, 0, 1);
                break;
            case CARTRIDGE_SIZE_4KB:
                save_regular_crt(0x1000, 1, 0xf000, 0, 0, 1);
                break;
            case CARTRIDGE_SIZE_8KB:
                save_regular_crt(0x2000, 1, 0xe000, 0, 0, 1);
                break;
            case CARTRIDGE_SIZE_16KB:
                save_2_blocks_crt(0x2000, 0x2000, 0x8000, 0xe000, 0, 1);
                break;
            default:
                fprintf(stderr, "Error: invalid size for generic ultimax cartridge\n");
                cleanup();
                exit(1);
                break;
        }
    } else {
        switch (loadfile_size) {
            case CARTRIDGE_SIZE_2KB:
                save_regular_crt(0x0800, 0, 0x8000, 0, 1, 0);
                break;
            case CARTRIDGE_SIZE_4KB:
                save_regular_crt(0x1000, 0, 0x8000, 0, 1, 0);
                break;
            case CARTRIDGE_SIZE_8KB:
                save_regular_crt(0x2000, 0, 0x8000, 0, 1, 0);
                break;
            case CARTRIDGE_SIZE_12KB:
                save_regular_crt(0x3000, 1, 0x8000, 0, 0, 0);
                break;
            case CARTRIDGE_SIZE_16KB:
                save_regular_crt(0x4000, 1, 0x8000, 0, 0, 0);
                break;
            default:
                fprintf(stderr, "Error: invalid size for generic C64 cartridge\n");
                cleanup();
                exit(1);
                break;
        }
    }
}

