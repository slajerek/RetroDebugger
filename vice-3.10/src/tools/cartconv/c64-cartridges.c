
#include "cartridge.h"

#include "cartconv.h"
#include "c64-saver.h"

/* this table must be in correct order so it can be indexed by CRT ID */
/*
    exrom, game, sizes, bank size, load addr, num banks, data type, name, option, saver

    num banks == 0 - take number of banks from input file size

    exrom / game
    0       0    - 16k Game
    0       1    -  8k Game
    1       0    -  ultimax
    1       1    -  ram/off
*/
const cart_t cart_info[] = {
/*  {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, "Generic 8KiB", NULL, NULL}, */ /* 8k game config */
/*  {0, 0, CARTRIDGE_SIZE_12KB,    0x3000, 0x8000,   1, 0, "Generic 12KiB", NULL, NULL}, */ /* 16k game config */
/*  {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, "Generic 16KiB", NULL, NULL}, */ /* 16k game config */
/*  {1, 0, CARTRIDGE_SIZE_4KB |
           CARTRIDGE_SIZE_16KB,         0,      0,   1, 0, "Ultimax", NULL, NULL}, */ /* ultimax config */

/* FIXME: initial exrom/game values are often wrong in this table
          don't forget to also update vice.texi accordingly */

    {0, 1, CARTRIDGE_SIZE_4KB |
           CARTRIDGE_SIZE_8KB |
           CARTRIDGE_SIZE_12KB |
           CARTRIDGE_SIZE_16KB,         0,      0,   0, 0, "Generic C64 Cartridge",              NULL, save_generic_crt},

    {0, 1, CARTRIDGE_SIZE_32KB,    0x2000, 0x8000,   4, 0, CARTRIDGE_NAME_ACTION_REPLAY,        "ar5", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x2000,      0,   2, 0, CARTRIDGE_NAME_KCS_POWER,            "kcs", save_2_blocks_crt},
    {0, 0, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_256KB,   0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_FINAL_III,            "fc3", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x2000,      0,   2, 0, CARTRIDGE_NAME_SIMONS_BASIC,       "simon", save_2_blocks_crt},
    {0, 0, CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB,   0x2000,      0,   0, 0, CARTRIDGE_NAME_OCEAN,              "ocean", save_ocean_crt},
    {1, 0, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 1, CARTRIDGE_NAME_EXPERT,            "expert", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_128KB,   0x2000, 0x8000,  16, 0, CARTRIDGE_NAME_FUNPLAY,               "fp", save_funplay_crt},
    {0, 0, CARTRIDGE_SIZE_64KB,    0x4000, 0x8000,   4, 0, CARTRIDGE_NAME_SUPER_GAMES,           "sg", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_32KB,    0x2000, 0x8000,   4, 0, CARTRIDGE_NAME_ATOMIC_POWER,          "ap", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_EPYX_FASTLOAD,       "epyx", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_WESTERMANN,            "wl", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_REX,                   "ru", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_FINAL_I,              "fc1", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_96KB |
           CARTRIDGE_SIZE_128KB,   0x2000, 0xe000,   0, 0, CARTRIDGE_NAME_MAGIC_FORMEL,          "mf", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_512KB,   0x2000, 0x8000,  64, 0, CARTRIDGE_NAME_GS,                    "gs", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_WARPSPEED,             "ws", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_128KB,   0x2000, 0x8000,  16, 0, CARTRIDGE_NAME_DINAMIC,              "din", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_20KB,         0,      0,   3, 0, CARTRIDGE_NAME_ZAXXON,            "zaxxon", save_zaxxon_crt},
    {0, 1, CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB |
           CARTRIDGE_SIZE_1024KB,  0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_MAGIC_DESK,            "md", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB,   0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_SUPER_SNAPSHOT_V5,    "ss5", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB,   0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_COMAL80,            "comal", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB,    0x2000, 0x8000,   2, 0, CARTRIDGE_NAME_STRUCTURED_BASIC,      "sb", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_32KB,    0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_ROSS,                "ross", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,          0, 0x8000,   0, 0, CARTRIDGE_NAME_DELA_EP64,          "dep64", save_delaep64_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_DELA_EP7x8,        "dep7x8", save_delaep7x8_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_DELA_EP256,        "dep256", save_delaep256_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,          0, 0x8000,   0, 0, CARTRIDGE_NAME_REX_EP256,         "rep256", save_rexep256_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_MIKRO_ASSEMBLER,    "mikro", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_24KB |
           CARTRIDGE_SIZE_32KB,    0x8000, 0x0000,   1, 0, CARTRIDGE_NAME_FINAL_PLUS,           "fcp", save_fcplus_crt},
    {0, 1, CARTRIDGE_SIZE_32KB,    0x2000, 0x8000,   4, 0, CARTRIDGE_NAME_ACTION_REPLAY4,       "ar4", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_16KB,    0x2000,      0,   4, 0, CARTRIDGE_NAME_STARDOS,             "star", save_stardos_crt},
    {1, 0, CARTRIDGE_SIZE_1024KB,  0x2000,      0, 128, 2, CARTRIDGE_NAME_EASYFLASH,           "easy", save_easyflash_crt},
    {0, 0, 0, 0, 0, 0, 0, CARTRIDGE_NAME_EASYFLASH_XBANK, NULL, NULL}, /* TODO ?? */
    {1, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0xe000,   1, 0, CARTRIDGE_NAME_CAPTURE,              "cap", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB,    0x2000, 0x8000,   2, 0, CARTRIDGE_NAME_ACTION_REPLAY3,       "ar3", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB,   0x2000, 0x8000,   0, 2, CARTRIDGE_NAME_RETRO_REPLAY,          "rr", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 2, CARTRIDGE_NAME_MMC64,              "mmc64", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_512KB,   0x2000, 0x8000,   0, 2, CARTRIDGE_NAME_MMC_REPLAY,          "mmcr", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_512KB,   0x4000, 0x8000,   0, 2, CARTRIDGE_NAME_IDE64,              "ide64", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_32KB,    0x4000, 0x8000,   2, 0, CARTRIDGE_NAME_SUPER_SNAPSHOT,       "ss4", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_4KB,     0x1000, 0x8000,   1, 0, CARTRIDGE_NAME_IEEE488,             "ieee", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_8KB,     0x2000, 0xe000,   1, 0, CARTRIDGE_NAME_GAME_KILLER,           "gk", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_256KB,   0x2000, 0x8000,  32, 0, CARTRIDGE_NAME_P64,                  "p64", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_8KB,     0x2000, 0xe000,   1, 0, CARTRIDGE_NAME_EXOS,                "exos", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_FREEZE_FRAME,          "ff", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_32KB,    0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_FREEZE_MACHINE,        "fm", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_4KB,     0x1000, 0xe000,   1, 0, CARTRIDGE_NAME_SNAPSHOT64,           "s64", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB,    0x2000, 0x8000,   2, 0, CARTRIDGE_NAME_SUPER_EXPLODE_V5,     "se5", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_MAGIC_VOICE,           "mv", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB,    0x2000, 0x8000,   2, 0, CARTRIDGE_NAME_ACTION_REPLAY2,       "ar2", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_4KB |
           CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_MACH5,              "mach5", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_DIASHOW_MAKER,        "dsm", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_64KB,    0x4000, 0x8000,   4, 0, CARTRIDGE_NAME_PAGEFOX,               "pf", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_24KB,    0x2000, 0x8000,   3, 0, CARTRIDGE_NAME_KINGSOFT,              "ks", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_128KB,   0x2000, 0x8000,  16, 0, CARTRIDGE_NAME_SILVERROCK_128,    "silver", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_32KB,    0x2000, 0xe000,   4, 0, CARTRIDGE_NAME_FORMEL64,             "f64", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_16KB |
           CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_RGCD,                "rgcd", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 2, CARTRIDGE_NAME_RRNETMK3,           "rrnet", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_24KB,         0,      0,   3, 0, CARTRIDGE_NAME_EASYCALC,             "ecr", save_easycalc_crt},
    {0, 1, CARTRIDGE_SIZE_512KB,   0x2000, 0x8000,  64, 2, CARTRIDGE_NAME_GMOD2,              "gmod2", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_16KB,    0x2000,      0,   0, 0, CARTRIDGE_NAME_MAX_BASIC,            "max", save_generic_crt},
    {0, 1, CARTRIDGE_SIZE_2048KB |
           CARTRIDGE_SIZE_4096KB |
           CARTRIDGE_SIZE_8192KB |
           CARTRIDGE_SIZE_16384KB, 0x2000, 0x8000,   0, 2, CARTRIDGE_NAME_GMOD3,              "gmod3", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_ZIPPCODE48,          "zipp", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_64KB,    0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_BLACKBOX8,            "bb8", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_BLACKBOX3,            "bb3", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_BLACKBOX4,            "bb4", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_REX_RAMFLOPPY,        "rrf", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_2KB |
           CARTRIDGE_SIZE_4KB |
           CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_BISPLUS,              "bis", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_128KB,   0x4000, 0x8000,   8, 0, CARTRIDGE_NAME_SDBOX,              "sdbox", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_1024KB,  0x4000, 0x8000,  64, 0, CARTRIDGE_NAME_MULTIMAX,              "mm", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_32KB,    0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_BLACKBOX9,            "bb9", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_8KB,     0x2000, 0x8000,   1, 0, CARTRIDGE_NAME_LT_KERNAL,            "ltk", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_64KB,    0x2000, 0x8000,   8, 0, CARTRIDGE_NAME_RAMLINK,               "rl", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_32KB,    0x2000, 0x8000,   4, 0, CARTRIDGE_NAME_DREAN,              "drean", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_8KB,     0x2000, 0xe000,   1, 0, CARTRIDGE_NAME_IEEEFLASH64,  "ieeeflash64", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB,    0x2000, 0x8000,   2, 0, CARTRIDGE_NAME_TURTLE_GRAPHICS_II,"turtle", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_FREEZE_FRAME_MK2,     "ff2", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_16KB,    0x2000, 0x8000,   2, 0, CARTRIDGE_NAME_PARTNER64,      "partner64", save_stardos_crt},
    {0, 1, CARTRIDGE_SIZE_64KB,    0x2000, 0x8000,   8, 0, CARTRIDGE_NAME_HYPERBASIC,         "hyper", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB,   0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_UC1,                  "uc1", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB,   0x4000, 0x8000,   0, 0, CARTRIDGE_NAME_UC15,                "uc15", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB,   0x4000, 0x8000,   0, 2, CARTRIDGE_NAME_UC2,                  "uc2", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_16KB,    0x4000, 0x8000,   1, 0, CARTRIDGE_NAME_BMPDATATURBO,         "bdt", save_regular_crt},
    {1, 0, CARTRIDGE_SIZE_16KB,    0x2000, 0xe000,   2, 0, CARTRIDGE_NAME_PROFIDOS,              "pd", save_regular_crt},
    {0, 0, CARTRIDGE_SIZE_32KB |
           CARTRIDGE_SIZE_64KB |
           CARTRIDGE_SIZE_128KB |
           CARTRIDGE_SIZE_256KB |
           CARTRIDGE_SIZE_512KB |
           CARTRIDGE_SIZE_1024KB |
           CARTRIDGE_SIZE_2048KB,  0x2000, 0x8000,   0, 0, CARTRIDGE_NAME_MAGIC_DESK_16,        "md16", save_regular_crt},
    {0, 1, CARTRIDGE_SIZE_1024KB,  0x2000, 0x8000, 128, 2, CARTRIDGE_NAME_MEGABYTER,              "mb", save_regular_crt},
    {0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
};

/* exrom, game, sizes, bank size, load addr, num banks, data type, name, option, saver */
