
#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "plus4-saver.h"

/* this table must be in correct order so it can be indexed by CRT ID */
/*
    exrom, game, sizes, bank size, load addr, num banks, data type, name, option, saver

    num banks == 0 - take number of banks from input file size

    exrom/game are always 0 for plus4
*/
const cart_t cart_info_plus4[] = {

    {0, 0, CARTRIDGE_SIZE_4KB |
           CARTRIDGE_SIZE_8KB |
           CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_32KB,    0x4000, 0x8000,   0, CRT_CHIP_ROM, "Generic Plus4 Cartridge",            "plus4", save_generic_plus4_crt},
    {0, 0, CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB |
           CARTRIDGE_SIZE_1MB |
           CARTRIDGE_SIZE_2MB,     0x4000, 0x8000,   0, CRT_CHIP_ROM, CARTRIDGE_PLUS4_NAME_MAGIC,           "magic", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_2MB |
           CARTRIDGE_SIZE_4MB,     0x4000, 0x8000,   0, CRT_CHIP_ROM, CARTRIDGE_PLUS4_NAME_MULTI,           "multi", save_multicart_crt},
    {0, 0, CARTRIDGE_SIZE_1MB,     0x4000, 0x8000,  64, CRT_CHIP_ROM, CARTRIDGE_PLUS4_NAME_JACINT1MB,      "jacint", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, CRT_CHIP_ROM, CARTRIDGE_PLUS4_NAME_SPEEDY,         "speedy", save_regular_crt},
    {0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
};
