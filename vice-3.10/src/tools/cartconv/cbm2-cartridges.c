
#include "cartridge.h"

#include "cartconv.h"
#include "crt.h"
#include "cbm2-saver.h"

/* this table must be in correct order so it can be indexed by CRT ID */
/*
    exrom, game, sizes, bank size, load addr, num banks, data type, name, option, saver

    num banks == 0 - take number of banks from input file size

    exrom/game are always 0 for CBM2
*/
const cart_t cart_info_cbm2[] = {

    {0, 0, CARTRIDGE_SIZE_4KB |
           CARTRIDGE_SIZE_8KB |
           CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_32KB,    0x2000, 0x8000,   0, CRT_CHIP_ROM, "Generic CBM2 Cartridge",            "cbm2", save_generic_cbm2_crt},
    {0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
};

