
#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "c128-saver.h"

/* this table must be in correct order so it can be indexed by CRT ID */
/*
    exrom, game, sizes, bank size, load addr, num banks, data type, name, option, saver

    num banks == 0 - take number of banks from input file size

    exrom/game are always 0 for c128
*/
const cart_t cart_info_c128[] = {

    {0, 0, CARTRIDGE_SIZE_8KB |
           CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_32KB,    0x4000, 0x8000,   0, CRT_CHIP_ROM, "Generic C128 Cartridge",              "c128", save_generic_c128_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, CRT_CHIP_ROM, CARTRIDGE_C128_NAME_WARPSPEED128,     "ws128", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, CRT_CHIP_ROM, CARTRIDGE_C128_NAME_PARTNER128,  "partner128", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_96KB,    0x4000, 0xc000,   6, CRT_CHIP_ROM, CARTRIDGE_C128_NAME_COMAL80,       "comal128", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB |
           CARTRIDGE_SIZE_1MB,     0x4000, 0x8000,   0, CRT_CHIP_ROM, CARTRIDGE_C128_NAME_MAGICDESK128,     "md128", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_512KB,   0x4000, 0x8000,  32, CRT_CHIP_ROM, CARTRIDGE_C128_NAME_GMOD2C128,      "gmod128", save_regular_crt},
    {0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
};
