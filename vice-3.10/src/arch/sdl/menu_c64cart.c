/*
 * menu_c64cart.c - Implementation of the C64/C128 cartridge settings menu for the SDL UI.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64cart.h"
#include "cartio.h"
#include "cartridge.h"
#include "clockport.h"
#include "keyboard.h"
#include "lib.h"
#include "ltkernal.h"
#include "machine.h"
#include "menu_c64_common_expansions.h"
#include "menu_common.h"
#include "menu_ethernetcart.h"
#include "ramlink.h"
#include "resources.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"

#include "menu_c64cart.h"


/* attach .bin cartridge image */
static UI_MENU_CALLBACK(attach_c64_cart_callback)
{
    char *name = NULL;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select cartridge image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (cartridge_attach_image(vice_ptr_to_int(param), name) < 0) {
                ui_error("Cannot load cartridge image.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_CART_ATTACH);
    }
    return NULL;
}

static ui_menu_entry_t ui_c64cart_entry = {
    .type     = MENU_ENTRY_DIALOG,
    .callback = (ui_callback_t)attach_c64_cart_callback,
};

static int countgroup(cartridge_info_t *cartlist, int flags)
{
    int num = 0;
    while(cartlist->name) {
        if (cartlist->flags & flags) {
            num++;
        }
        cartlist++;
    }
    return num;
}

static void makegroup(cartridge_info_t *cartlist, ui_menu_entry_t *entry, int flags)
{
    while(cartlist->name) {
        if (cartlist->flags & flags) {
            ui_c64cart_entry.string = cartlist->name;
            ui_c64cart_entry.data = (ui_callback_data_t)(vice_int_to_ptr(cartlist->crtid));
            memcpy(entry, &ui_c64cart_entry, sizeof(ui_menu_entry_t));
            entry++;
        }
        cartlist++;
    }
    memset(entry, 0, sizeof(ui_menu_entry_t));
}

/** \brief  Offset in c64cart_menu of the "Attach raw image" submenu */
#define C64CART_RAW_OFFSET  1

/** \brief  Offset in c128cart_menu of the "Attach raw image" submenu */
#define C128CART_RAW_OFFSET  1

/** \brief  Offset in scpu64cart_menu of the "Attach raw image" submenu */
#define SCPU64CART_RAW_OFFSET  1


static ui_menu_entry_t *attach_raw_cart_menu = NULL;

void uicart_menu_create(void)
{
    int num;
    cartridge_info_t *cartlist = cartridge_get_info_list();

    num = countgroup(cartlist, CARTRIDGE_GROUP_GENERIC | CARTRIDGE_GROUP_FREEZER | CARTRIDGE_GROUP_GAME | CARTRIDGE_GROUP_UTIL);
    attach_raw_cart_menu = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));
    makegroup(cartlist, attach_raw_cart_menu, CARTRIDGE_GROUP_GENERIC | CARTRIDGE_GROUP_FREEZER | CARTRIDGE_GROUP_GAME | CARTRIDGE_GROUP_UTIL);
    if (machine_class == VICE_MACHINE_SCPU64) {
        scpu64cart_menu[SCPU64CART_RAW_OFFSET].data = attach_raw_cart_menu;
    } else if (machine_class == VICE_MACHINE_C128) {
        c128cart_menu[C128CART_RAW_OFFSET].data = attach_raw_cart_menu;
    } else {
        c64cart_menu[C64CART_RAW_OFFSET].data = attach_raw_cart_menu;
    }
}


void uicart_menu_shutdown(void)
{
    if (attach_raw_cart_menu != NULL) {
        lib_free(attach_raw_cart_menu);
    }
}


static UI_MENU_CALLBACK(set_c64_cart_default_callback)
{
    if (activated) {
        cartridge_set_default();
    }
    return NULL;
}

static UI_MENU_CALLBACK(unset_c64_cart_default_callback)
{
    if (activated) {
        cartridge_unset_default();
    }
    return NULL;
}

/* FIXME: we need an error reporting system, so all this
          stuff can go away. */
typedef struct c64_cart_flush_s {
    int cartid;
    char *enable_res;
    char *image_res;
} c64_cart_flush_t;

static c64_cart_flush_t carts[] = {
    { CARTRIDGE_RAMCART,        "RAMCART",                  "RAMCARTfilename" },
    { CARTRIDGE_REU,            "REU",                      "REUfilename" },
    { CARTRIDGE_EXPERT,         "ExpertCartridgeEnabled",   "Expertfilename" },
    { CARTRIDGE_DQBB,           "DQBB",                     "DQBBfilename" },
    { CARTRIDGE_ISEPIC,         "IsepicCartridgeEnabled",   "Isepicfilename" },
    { CARTRIDGE_EASYFLASH,      NULL,                       NULL },
    { CARTRIDGE_GEORAM,         "GEORAM",                   "GEORAMfilename" },
    { CARTRIDGE_MEGABYTER,      NULL,                       NULL, },
    { CARTRIDGE_MMC64,          "MMC64",                    "MMC64BIOSfilename" },
    { CARTRIDGE_MMC_REPLAY,     NULL,                       NULL },
    { CARTRIDGE_GMOD2,          NULL,                       NULL },
    { CARTRIDGE_C128_GMOD2C128, NULL,                       NULL }, /* FIXME: is this correct here? */
    { CARTRIDGE_RETRO_REPLAY,   NULL,                       NULL },
    { 0,                        NULL,                       NULL }
};

static c64_cart_flush_t carts_secondary[] = {
    { CARTRIDGE_MMC_REPLAY,     NULL,       "MMCREEPROMImage" },
    { CARTRIDGE_GMOD2,          NULL,       "GMod2EEPROMImage" },
    { CARTRIDGE_C128_GMOD2C128, NULL,       "GMod128EEPROMImage" }, /* FIXME: is this correct here? */
    { CARTRIDGE_RAMLINK,        "RAMLINK",  "RAMLINKfilename" },
    { CARTRIDGE_REX_RAMFLOPPY,  NULL,       "RRFfilename" },
    { 0,                        NULL,       NULL }
};

static void cartmenu_update_flush(void);
static void cartmenu_update_save(void);

static UI_MENU_CALLBACK(c64_cart_flush_callback)
{
    int i;
    int found = 0;
    int enabled = 1;
    const char *filename = "a";

    if (activated) {
        int cartid = vice_ptr_to_int(param);

        if (cartridge_flush_image(cartid) < 0) {
            /* find cartid in carts */
            for (i = 0; carts[i].cartid != 0 && !found; i++) {
                if (carts[i].cartid == cartid) {
                    found = 1;
                }
            }
            i--;

            /* check if cart was enabled */
            if (found) {
                if (carts[i].enable_res) {
                    resources_get_int(carts[i].enable_res, &enabled);
                }
            }

            /* check if cart has image */
            if (found) {
                if (carts[i].image_res) {
                    resources_get_string(carts[i].image_res, &filename);
                }
            }

            if (!enabled) {
                ui_error("Cartridge is not enabled.");
            } else if (!filename) {
                ui_error("No name defined for cart image.");
            } else if (!*filename) {
                ui_error("No name defined for cart image.");
            } else {
                ui_error("Cannot save cartridge image.");
            }
        }
    } else {
        cartmenu_update_flush();
    }
    return NULL;
}

static UI_MENU_CALLBACK(c64_cart_flush_secondary_callback)
{
    int i;
    int found = 0;
    int enabled = 1;
    const char *filename = "a";

    if (activated) {
        int cartid = vice_ptr_to_int(param);

        if (cartridge_flush_secondary_image(cartid) < 0) {
            /* find cartid in carts_secondary */
            for (i = 0; carts_secondary[i].cartid != 0 && !found; i++) {
                if (carts_secondary[i].cartid == cartid) {
                    found = 1;
                }
            }
            i--;

            /* check if cart was enabled */
            if (found) {
                if (carts_secondary[i].enable_res) {
                    resources_get_int(carts_secondary[i].enable_res, &enabled);
                }
            }

            /* check if cart has image */
            if (found) {
                if (carts_secondary[i].image_res) {
                    resources_get_string(carts_secondary[i].image_res, &filename);
                }
            }

            if (!enabled) {
                ui_error("Cartridge is not enabled.");
            } else if (!filename) {
                ui_error("No name defined for secondary image.");
            } else if (!*filename) {
                ui_error("No name defined for secondary image.");
            } else {
                ui_error("Cannot save secondary image.");
            }
        }
    } else {
        cartmenu_update_flush();
    }
    return NULL;
}

static UI_MENU_CALLBACK(c64_cart_save_callback)
{
    if (activated) {
        int cartid = vice_ptr_to_int(param);
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose cartridge file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (cartridge_save_image(cartid, name) < 0) {
                ui_error("Cannot save cartridge image.");
            }
            lib_free(name);
        }
    } else {
        cartmenu_update_save();
    }
    return NULL;
}

static UI_MENU_CALLBACK(c64_cart_save_secondary_callback)
{
    if (activated) {
        int cartid = vice_ptr_to_int(param);
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose secondary image file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (cartridge_save_secondary_image(cartid, name) < 0) {
                ui_error("Cannot save secondary image.");
            }
            lib_free(name);
        }
    } else {
        cartmenu_update_save();
    }
    return NULL;
}

/* REX Ram-Floppy */

UI_MENU_DEFINE_FILE_STRING(RRFfilename)
UI_MENU_DEFINE_TOGGLE(RRFImageWrite)

#define REXRAMFLOPPY_OFFSET_FLUSH_SECONDARY 3
#define REXRAMFLOPPY_OFFSET_SAVE_SECONDARY  4

static ui_menu_entry_t rexramfloppy_menu[] = {
    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RRFfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_REX_RAMFLOPPY " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RRFImageWrite_callback
    },
    {   .string   = "Save image now", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_REX_RAMFLOPPY
    },
    {   .string   = "Save image as", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_REX_RAMFLOPPY
    },
    SDL_MENU_LIST_END
};

/* RAMCART */

UI_MENU_DEFINE_TOGGLE(RAMCART)
UI_MENU_DEFINE_TOGGLE(RAMCART_RO)
UI_MENU_DEFINE_RADIO(RAMCARTsize)
UI_MENU_DEFINE_FILE_STRING(RAMCARTfilename)
UI_MENU_DEFINE_TOGGLE(RAMCARTImageWrite)

#define RAMCART_OFFSET_FLUSH 10
#define RAMCART_OFFSET_SAVE 11

static ui_menu_entry_t ramcart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_RAMCART,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMCART_callback
    },
    {   .string   = "Read-only",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMCART_RO_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "64KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMCARTsize_callback,
        .data     = (ui_callback_data_t)64
    },
    {   .string   = "128KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMCARTsize_callback,
        .data     = (ui_callback_data_t)128
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RAMCARTfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_RAMCART " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMCARTImageWrite_callback
    },
    {   .string    = "Save image now", /* 10 */
        .type      = MENU_ENTRY_OTHER,
        .callback  = c64_cart_flush_callback,
        .data      = (ui_callback_data_t)CARTRIDGE_RAMCART
    },
    {   .string    = "Save image as", /* 11 */
        .type      = MENU_ENTRY_OTHER,
        .callback  = c64_cart_save_callback,
        .data      = (ui_callback_data_t)CARTRIDGE_RAMCART
    },
    SDL_MENU_LIST_END
};

/* REU */

UI_MENU_DEFINE_TOGGLE(REU)
UI_MENU_DEFINE_RADIO(REUsize)
UI_MENU_DEFINE_FILE_STRING(REUfilename)
UI_MENU_DEFINE_TOGGLE(REUImageWrite)

#define REU_OFFSET_FLUSH 15
#define REU_OFFSET_SAVE 16

static ui_menu_entry_t reu_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_REU,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_REU_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "128KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)256
    },
    {   .string   = "512KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)512
    },
    {   .string   = "1MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)1024
    },
    {   .string   = "2MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)2048
    },
    {   .string   = "4MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)4096
    },
    {   .string   = "8MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)8192
    },
    {   .string   = "16MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_REUsize_callback,
        .data     = (ui_callback_data_t)16384
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_REUfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_REU " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_REUImageWrite_callback
    },
    {   .string   = "Save image now", /* 15 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_REU
    },
    {   .string   = "Save image as", /* 16 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_REU
    },
    SDL_MENU_LIST_END
};


/* Expert cartridge */

UI_MENU_DEFINE_TOGGLE(ExpertCartridgeEnabled)
UI_MENU_DEFINE_RADIO(ExpertCartridgeMode)
UI_MENU_DEFINE_FILE_STRING(Expertfilename)
UI_MENU_DEFINE_TOGGLE(ExpertImageWrite)

#define EXPERT_OFFSET_FLUSH 10
#define EXPERT_OFFSET_SAVE 11

static ui_menu_entry_t expert_cart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_EXPERT,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ExpertCartridgeEnabled_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Mode"),
    {   .string   = "Off",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ExpertCartridgeMode_callback,
        .data     = (ui_callback_data_t)EXPERT_MODE_OFF
    },
    {   .string   = "Prg",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ExpertCartridgeMode_callback,
        .data     = (ui_callback_data_t)EXPERT_MODE_PRG
    },
    {   .string   = "On",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_ExpertCartridgeMode_callback,
        .data     = (ui_callback_data_t)EXPERT_MODE_ON
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_Expertfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_EXPERT " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ExpertImageWrite_callback
    },
    {   .string   = "Save image now", /* 10 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_EXPERT
    },
    {   .string   = "Save image as", /* 11 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_EXPERT
    },
    SDL_MENU_LIST_END
};


/* Double Quick Brown Box */

UI_MENU_DEFINE_TOGGLE(DQBB)
UI_MENU_DEFINE_RADIO(DQBBMode)
UI_MENU_DEFINE_RADIO(DQBBSize)
UI_MENU_DEFINE_FILE_STRING(DQBBfilename)
UI_MENU_DEFINE_TOGGLE(DQBBImageWrite)

#define DQBB_OFFSET_FLUSH 16
#define DQBB_OFFSET_SAVE 17

static ui_menu_entry_t dqbb_cart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_DQBB,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DQBB_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Mode"),
    {   .string   = "C64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBMode_callback,
        .data     = (ui_callback_data_t)DQBB_MODE_C64
    },
    {   .string   = "C128",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBMode_callback,
        .data     = (ui_callback_data_t)DQBB_MODE_C128
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "16KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBSize_callback,
        .data     = (ui_callback_data_t)16
    },
    {   .string   = "32KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBSize_callback,
        .data     = (ui_callback_data_t)32
    },
    {   .string   = "64KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBSize_callback,
        .data     = (ui_callback_data_t)64
    },
    {   .string   = "128KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBSize_callback,
        .data     = (ui_callback_data_t)128
    },
    {   .string   = "256KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DQBBSize_callback,
        .data     = (ui_callback_data_t)256
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_DQBBfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_DQBB " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DQBBImageWrite_callback
    },
    {   .string   = "Save image now", /* 16 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_DQBB
    },
    {   .string   = "Save image as", /* 17 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_DQBB
    },

    SDL_MENU_LIST_END
};


/* ISEPIC */

UI_MENU_DEFINE_TOGGLE(IsepicCartridgeEnabled)
UI_MENU_DEFINE_TOGGLE(IsepicSwitch)
UI_MENU_DEFINE_FILE_STRING(Isepicfilename)
UI_MENU_DEFINE_TOGGLE(IsepicImageWrite)

#define ISEPIC_OFFSET_FLUSH 6
#define ISEPIC_OFFSET_SAVE 7

static ui_menu_entry_t isepic_cart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_ISEPIC,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IsepicCartridgeEnabled_callback
    },
    {   .string   = "Switch",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IsepicSwitch_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_Isepicfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_ISEPIC " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IsepicImageWrite_callback
    },
    {   .string   = "Save image now", /* 6 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_ISEPIC
    },
    {   .string   = "Save image as", /* 7 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_ISEPIC
    },
    SDL_MENU_LIST_END
};


/* EasyFlash */

UI_MENU_DEFINE_TOGGLE(EasyFlashJumper)
UI_MENU_DEFINE_TOGGLE(EasyFlashWriteCRT)
UI_MENU_DEFINE_TOGGLE(EasyFlashOptimizeCRT)

#define EASYFLASH_OFFSET_FLUSH 3
#define EASYFLASH_OFFSET_SAVE 4

static ui_menu_entry_t easyflash_cart_menu[] = {
    {   .string   = "Jumper",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_EasyFlashJumper_callback
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_EasyFlashWriteCRT_callback
    },
    {   .string   = "Optimize image on write",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_EasyFlashOptimizeCRT_callback
    },
    {   .string   = "Save image now", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_EASYFLASH
    },
    {   .string   = "Save image as", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_EASYFLASH
    },
    SDL_MENU_LIST_END
};


/* GEORAM */

UI_MENU_DEFINE_TOGGLE(GEORAM)
UI_MENU_DEFINE_RADIO(GEORAMsize)
UI_MENU_DEFINE_FILE_STRING(GEORAMfilename)
UI_MENU_DEFINE_TOGGLE(GEORAMImageWrite)

#define GEORAM_OFFSET_FLUSH 11
#define GEORAM_OFFSET_SAVE 12

static ui_menu_entry_t georam_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_GEORAM,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GEORAM_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "512KiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_GEORAMsize_callback,
        .data     = (ui_callback_data_t)512
    },
    {   .string   = "1MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_GEORAMsize_callback,
        .data     = (ui_callback_data_t)1024
    },
    {   .string   = "2MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_GEORAMsize_callback,
        .data     = (ui_callback_data_t)2048
    },
    {   .string   = "4MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_GEORAMsize_callback,
        .data     = (ui_callback_data_t)4096
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_GEORAMfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_GEORAM " image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GEORAMImageWrite_callback
    },
    {   .string   = "Save image now", /* 11 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GEORAM
    },
    {   .string   = "Save image as", /* 12 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GEORAM
    },
    SDL_MENU_LIST_END
};


/* Megabyter */

UI_MENU_DEFINE_TOGGLE(MegabyterWriteCRT)
UI_MENU_DEFINE_TOGGLE(MegabyterOptimizeCRT)

#define MEGABYTER_OFFSET_FLUSH 2
#define MEGABYTER_OFFSET_SAVE 3

static ui_menu_entry_t megabyter_cart_menu[] = {
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MegabyterWriteCRT_callback
    },
    {   .string   = "Optimize image on write",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MegabyterOptimizeCRT_callback
    },
    {   .string   = "Save image now", /* 2 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MEGABYTER
    },
    {   .string   = "Save image as", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MEGABYTER
    },
    SDL_MENU_LIST_END
};


/* MMC64 */

UI_MENU_DEFINE_RADIO(MMC64ClockPort)

static ui_menu_entry_t mmc64_clockport_device_menu[CLOCKPORT_MAX_ENTRIES + 1];

UI_MENU_DEFINE_RADIO(MMC64_sd_type)

static const ui_menu_entry_t mmc64_sd_type_menu[] = {
    {   .string   = "Auto",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMC64_sd_type_callback,
        .data     = (ui_callback_data_t)MMC64_TYPE_AUTO
    },
    {   .string   = "MMC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMC64_sd_type_callback,
        .data     = (ui_callback_data_t)MMC64_TYPE_MMC
    },
    {   .string   = "SD",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMC64_sd_type_callback,
        .data     = (ui_callback_data_t)MMC64_TYPE_SD
    },
    {   .string   = "SDHC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMC64_sd_type_callback,
        .data     = (ui_callback_data_t)MMC64_TYPE_SDHC
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(MMC64)
UI_MENU_DEFINE_RADIO(MMC64_revision)
UI_MENU_DEFINE_TOGGLE(MMC64_flashjumper)
UI_MENU_DEFINE_TOGGLE(MMC64_bios_write)
UI_MENU_DEFINE_FILE_STRING(MMC64BIOSfilename)
UI_MENU_DEFINE_TOGGLE(MMC64_RO)
UI_MENU_DEFINE_FILE_STRING(MMC64imagefilename)

#define MMC64_OFFSET_FLUSH 11
#define MMC64_OFFSET_SAVE 12

static ui_menu_entry_t mmc64_cart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_MMC64,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMC64_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Revision"),
    {   .string   = "Rev A",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMC64_revision_callback,
        .data     = (ui_callback_data_t)MMC64_REV_A
    },
    {   .string   = "Rev B",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMC64_revision_callback,
        .data     = (ui_callback_data_t)MMC64_REV_B
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "BIOS flash jumper",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMC64_flashjumper_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("BIOS image"),
    {   .string   = "BIOS image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MMC64BIOSfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_MMC64 " BIOS image"
    },
    {   .string   = "Save image on detach",
        .type     =  MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMC64_bios_write_callback
    },
    {   .string   = "Save image now", /* 11 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MMC64
    },
    {   .string   = "Save image as", /* 12 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MMC64
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("MMC/SD image"),
    {   .string   = "MMC/SD image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MMC64imagefilename_callback,
        .data     = (ui_callback_data_t)"Select MMC/SD image"
    },
    {   .string   = "Image read-only",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMC64_RO_callback
    },
    {   .string   = "Card type",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)mmc64_sd_type_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Clockport device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmc64_clockport_device_menu
    },
    SDL_MENU_LIST_END
};


/* MMC Replay */

UI_MENU_DEFINE_RADIO(MMCRClockPort)

static ui_menu_entry_t mmcreplay_clockport_device_menu[CLOCKPORT_MAX_ENTRIES + 1];

UI_MENU_DEFINE_RADIO(MMCRSDType)

static const ui_menu_entry_t mmcreplay_sd_type_menu[] = {
    {   .string   =  "Auto",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMCRSDType_callback,
        .data     = (ui_callback_data_t)MMCR_TYPE_AUTO
    },
    {   .string   = "MMC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMCRSDType_callback,
        .data     = (ui_callback_data_t)MMCR_TYPE_MMC
    },
    {   .string   = "SD",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMCRSDType_callback,
        .data     = (ui_callback_data_t)MMCR_TYPE_SD
    },
    {   .string   = "SDHC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MMCRSDType_callback,
        .data     = (ui_callback_data_t)MMCR_TYPE_SDHC
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_FILE_STRING(MMCRCardImage)
UI_MENU_DEFINE_FILE_STRING(MMCREEPROMImage)
UI_MENU_DEFINE_TOGGLE(MMCRCardRW)
UI_MENU_DEFINE_TOGGLE(MMCREEPROMRW)
UI_MENU_DEFINE_TOGGLE(MMCRRescueMode)
UI_MENU_DEFINE_TOGGLE(MMCRImageWrite)

#define MMCR_OFFSET_FLUSH 4
#define MMCR_OFFSET_SAVE 5
#define MMCR_OFFSET_FLUSH_SECONDARY 10
#define MMCR_OFFSET_SAVE_SECONDARY 11

static ui_menu_entry_t mmcreplay_cart_menu[] = {
    {   .string   = "Rescue mode",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMCRRescueMode_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("BIOS image"),
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMCRImageWrite_callback
    },
    {   .string   = "Save image now", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MMC_REPLAY
    },
    {   .string   = "Save image as", /* 5 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MMC_REPLAY
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("EEPROM image"),
    {   .string   = "EEPROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MMCREEPROMImage_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_MMC_REPLAY " EEPROM image"
    },
    {   .string   = "Enable writes to EEPROM image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMCREEPROMRW_callback
    },
    {   .string   = "Save image now", /* 10 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MMC_REPLAY
    },
    {   .string   = "Save image as", /* 11 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_MMC_REPLAY
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("MMC/SD image"),
    {   .string   = "Card image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MMCRCardImage_callback,
        .data     = (ui_callback_data_t)"Select card image"
    },
    {   .string   = "Enable writes to card image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MMCRCardRW_callback
    },
    {   .string   = "Card type",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)mmcreplay_sd_type_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Clockport device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmcreplay_clockport_device_menu
    },
    SDL_MENU_LIST_END
};


/* Retro Replay */

UI_MENU_DEFINE_RADIO(RRClockPort)

static ui_menu_entry_t retroreplay_clockport_device_menu[CLOCKPORT_MAX_ENTRIES + 1];

UI_MENU_DEFINE_TOGGLE(RRBankJumper)
UI_MENU_DEFINE_RADIO(RRRevision)
UI_MENU_DEFINE_TOGGLE(RRFlashJumper)
UI_MENU_DEFINE_TOGGLE(RRBiosWrite)

static const ui_menu_entry_t retroreplay_revision_menu[] = {
    {   .string   = CARTRIDGE_NAME_RETRO_REPLAY,
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RRRevision_callback,
        .data     = (ui_callback_data_t)RR_REV_RETRO_REPLAY
    },
    {   .string   = CARTRIDGE_NAME_NORDIC_REPLAY,
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RRRevision_callback,
        .data     = (ui_callback_data_t)RR_REV_NORDIC_REPLAY
    },
    SDL_MENU_LIST_END
};

#define RETROREPLAY_OFFSET_FLUSH 4
#define RETROREPLAY_OFFSET_SAVE 5

static ui_menu_entry_t retroreplay_cart_menu[] = {
    {   .string   = "Revision",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)retroreplay_revision_menu
    },
    {   .string   = "Bank jumper",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RRBankJumper_callback,
    },
    {   .string   = "BIOS flash jumper",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RRFlashJumper_callback
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RRBiosWrite_callback
    },
    {   .string   = "Save image now", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_RETRO_REPLAY
    },
    {   .string   = "Save image as", /* 5 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_RETRO_REPLAY
    },

    SDL_MENU_ITEM_SEPARATOR,
    {   .string   = "Clockport device",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)retroreplay_clockport_device_menu
    },
    SDL_MENU_LIST_END
};


/* GMod2 */

UI_MENU_DEFINE_FILE_STRING(GMod2EEPROMImage)
UI_MENU_DEFINE_TOGGLE(GMOD2EEPROMRW)
UI_MENU_DEFINE_TOGGLE(GMod2FlashWrite)

#define GMOD2_OFFSET_FLUSH_SECONDARY 3
#define GMOD2_OFFSET_SAVE_SECONDARY 4
#define GMOD2_OFFSET_FLUSH 8
#define GMOD2_OFFSET_SAVE 9

static ui_menu_entry_t gmod2_cart_menu[] = {
    SDL_MENU_ITEM_TITLE("EEPROM image"),
    {   .string   = "EEPROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_GMod2EEPROMImage_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_GMOD2 " EEPROM image"
    },
    {   .string   = "Enable writes to EEPROM image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GMOD2EEPROMRW_callback
    },
    {   .string   = "Save EEPROM image now", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GMOD2
    },
    {   .string   = "Save EEPROM image as", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GMOD2
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Flash image"),
    {   .string   = "Enable writes to flash image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GMod2FlashWrite_callback
    },
    {   .string   = "Save image now", /* 8 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GMOD2
    },
    {   .string   = "Save image as", /* 9 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GMOD2
    },
    SDL_MENU_LIST_END
};

/* GMod2-C128 */

UI_MENU_DEFINE_FILE_STRING(GMod128EEPROMImage)
UI_MENU_DEFINE_TOGGLE(GMOD128EEPROMRW)
UI_MENU_DEFINE_TOGGLE(GMod128FlashWrite)

#define GMOD2C128_OFFSET_FLUSH_SECONDARY 3
#define GMOD2C128_OFFSET_SAVE_SECONDARY 4
#define GMOD2C128_OFFSET_FLUSH 8
#define GMOD2C128_OFFSET_SAVE 9

static ui_menu_entry_t gmod2c128_cart_menu[] = {
    SDL_MENU_ITEM_TITLE("EEPROM image"),
    {   .string   = "EEPROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_GMod128EEPROMImage_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_C128_NAME_GMOD2C128 " EEPROM image"
    },
    {   .string   = "Enable writes to EEPROM image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GMOD128EEPROMRW_callback
    },
    {   .string   = "Save EEPROM image now", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_C128_GMOD2C128
    },
    {   .string   = "Save EEPROM image as", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_C128_GMOD2C128
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Flash image"),
    {   .string   = "Enable writes to flash image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GMod128FlashWrite_callback
    },
    {   .string   = "Save image now", /* 8 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_C128_GMOD2C128
    },
    {   .string   = "Save image as", /* 9 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_C128_GMOD2C128
    },
    SDL_MENU_LIST_END
};

/* GMod3 */

UI_MENU_DEFINE_TOGGLE(GMod3FlashWrite)

#define GMOD3_OFFSET_FLUSH 1
#define GMOD3_OFFSET_SAVE 2

static ui_menu_entry_t gmod3_cart_menu[] = {
    {   .string   = "Enable writes to flash image",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GMod3FlashWrite_callback
    },
    {   .string   = "Save image now", /* 1 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GMOD3
    },
    {   .string   = "Save image as", /* 2 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_GMOD3
    },
    SDL_MENU_LIST_END
};

/* LT.Kernal */

UI_MENU_DEFINE_FILE_STRING(LTKimage0)
UI_MENU_DEFINE_FILE_STRING(LTKimage1)
UI_MENU_DEFINE_FILE_STRING(LTKimage2)
UI_MENU_DEFINE_FILE_STRING(LTKimage3)
UI_MENU_DEFINE_FILE_STRING(LTKimage4)
UI_MENU_DEFINE_FILE_STRING(LTKimage5)
UI_MENU_DEFINE_FILE_STRING(LTKimage6)
UI_MENU_DEFINE_STRING(LTKserial)
UI_MENU_DEFINE_RADIO(LTKport)
UI_MENU_DEFINE_RADIO(LTKio)

static ui_menu_entry_t ltk_cart_menu[] = {
    SDL_MENU_ITEM_TITLE("HD images"),
    {   .string   = "HD0 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage0_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    {   .string   = "HD1 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage1_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    {   .string   = "HD2 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage2_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    {   .string   = "HD3 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage3_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    {   .string   = "HD4 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage4_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    {   .string   = "HD5 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage5_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    {   .string   = "HD6 image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_LTKimage6_callback,
        .data     = (ui_callback_data_t)"Select HD image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Serial Number"),
    {   .string   = "Serial",
        .type     = MENU_ENTRY_DIALOG,
        .callback = string_LTKserial_callback,
        .data     = (ui_callback_data_t)"Enter serial"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("I/O address"),
    {   .string   = "$dexx",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKio_callback,
        .data     = (ui_callback_data_t)LTKIO_DE00
    },
    {   .string   = "$dfxx",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKio_callback,
        .data     = (ui_callback_data_t)LTKIO_DF00
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Port Number"),
    {   .string   = "0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)2
    },
    {   .string   = "3",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)3
    },
    {   .string   = "4",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "5",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)5
    },
    {   .string   = "6",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)6
    },
    {   .string   = "7",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)7
    },
    {   .string   = "8",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "9",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)9
    },
    {   .string   = "10",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)10
    },
    {   .string   = "11",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)11
    },
    {   .string   = "12",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)12
    },
    {   .string   = "13",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)13
    },
    {   .string   = "14",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)14
    },
    {   .string   = "15",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_LTKport_callback,
        .data     = (ui_callback_data_t)15
    },
    SDL_MENU_LIST_END
};

/* RAMLINK */

UI_MENU_DEFINE_TOGGLE(RAMLINK)
UI_MENU_DEFINE_RADIO(RAMLINKmode)
UI_MENU_DEFINE_TOGGLE(RAMLINKrtcsave)
UI_MENU_DEFINE_RADIO(RAMLINKsize)
UI_MENU_DEFINE_FILE_STRING(RAMLINKfilename)
UI_MENU_DEFINE_FILE_STRING(RAMLINKBIOSfilename)
UI_MENU_DEFINE_TOGGLE(RAMLINKImageWrite)

#define RAMLINK_OFFSET_FLUSH_SECONDARY 31
#define RAMLINK_OFFSET_SAVE_SECONDARY  32

static ui_menu_entry_t ramlink_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_RAMLINK,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMLINK_callback
    },
    {   .string   = "RTC Save",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMLINKrtcsave_callback
    },
    SDL_MENU_ITEM_TITLE("Mode"),
    {   .string   = "Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKmode_callback,
        .data     = (ui_callback_data_t)RL_MODE_NORMAL
    },
    {   .string   = "Direct",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKmode_callback,
        .data     = (ui_callback_data_t)RL_MODE_DIRECT
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Memory size"),
    {   .string   = "0MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "1MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "2MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)2
    },
    {   .string   = "3MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)3
    },
    {   .string   = "4MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)4
    },
    {   .string   = "5MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)5
    },
    {   .string   = "6MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)6
    },
    {   .string   = "7MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)7
    },
    {   .string   = "8MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)8
    },
    {   .string   = "9MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)9
    },
    {   .string   = "10MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)10
    },
    {   .string   = "11MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)11
    },
    {   .string   = "12MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)12
    },
    {   .string   = "13MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)13
    },
    {   .string   = "14MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)14
    },
    {   .string   = "15MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)15
    },
    {   .string   = "16MiB",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_RAMLINKsize_callback,
        .data     = (ui_callback_data_t)16
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("ROM image"),
    {   .string   = "ROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RAMLINKBIOSfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_RAMLINK " ROM image"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("RAM image"),
    {   .string   = "Specify RAM Image file and load",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_RAMLINKfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_RAMLINK " RAM image"
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RAMLINKImageWrite_callback
    },
    {   .string   = "Save image now",  /* 31 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_RAMLINK
    },
    {   .string   = "Save image as",   /* 32 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_RAMLINK
    },
    SDL_MENU_LIST_END
};


/* RRNET MK3 */
UI_MENU_DEFINE_TOGGLE(RRNETMK3_flashjumper)
UI_MENU_DEFINE_TOGGLE(RRNETMK3_bios_write)

#define RRNETMK3_OFFSET_FLUSH 2
#define RRNETMK3_OFFSET_SAVE 3

static ui_menu_entry_t rrnet_mk3_cart_menu[] = {
    {   .string   = "BIOS flash jumper",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RRNETMK3_flashjumper_callback
    },
    {   .string   = "Save image on detach",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_RRNETMK3_bios_write_callback
    },
    {   .string   = "Save image now", /* 2 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_RRNETMK3
    },
    {   .string   = "Save image as", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = c64_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_RRNETMK3
    },
    SDL_MENU_LIST_END
};


/* Magic Voice */

UI_MENU_DEFINE_TOGGLE(MagicVoiceCartridgeEnabled)
UI_MENU_DEFINE_FILE_STRING(MagicVoiceImage)

static const ui_menu_entry_t magicvoice_cart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_MAGIC_VOICE,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MagicVoiceCartridgeEnabled_callback
    },
    {   .string   = "ROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MagicVoiceImage_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_MAGIC_VOICE " ROM image"
    },
    SDL_MENU_LIST_END
};


/* SFX Sound Expander */

UI_MENU_DEFINE_TOGGLE(SFXSoundExpander)
UI_MENU_DEFINE_RADIO(SFXSoundExpanderChip)

static const ui_menu_entry_t soundexpander_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundExpander_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("YM chip type"),
    {   .string   = "3526",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SFXSoundExpanderChip_callback,
        .data     = (ui_callback_data_t)3526
    },
    {   .string   = "3812",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_SFXSoundExpanderChip_callback,
        .data     = (ui_callback_data_t)3812
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(IEEEFlash64)
UI_MENU_DEFINE_FILE_STRING(IEEEFlash64Image)
UI_MENU_DEFINE_TOGGLE(IEEEFlash64Dev8)
UI_MENU_DEFINE_TOGGLE(IEEEFlash64Dev910)
UI_MENU_DEFINE_TOGGLE(IEEEFlash64Dev4)

static const ui_menu_entry_t ieeeflash64_menu[] = {
    SDL_MENU_ITEM_TITLE("IEEE Flash! 64 settings"),
    {   .string   = "Enable " CARTRIDGE_NAME_IEEEFLASH64,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IEEEFlash64_callback
    },
    {   .string   = "Route device 8 to IEEE",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IEEEFlash64Dev8_callback
    },
    {   .string   = "Route devices 9/10 to IEEE",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IEEEFlash64Dev910_callback
    },
    {   .string   = "Route device 4 to IEEE",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IEEEFlash64Dev4_callback
    },
    {   .string   = "ROM image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_IEEEFlash64Image_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_NAME_IEEEFLASH64 " ROM image"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(IOCollisionHandling)

static const ui_menu_entry_t iocollision_menu[] = {
    {   .string   = "Detach all",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOCollisionHandling_callback,
        .data     = (ui_callback_data_t)IO_COLLISION_METHOD_DETACH_ALL
    },
    {   .string   = "Detach last",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOCollisionHandling_callback,
        .data     = (ui_callback_data_t)IO_COLLISION_METHOD_DETACH_LAST
    },
    {   .string   = "AND values",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_IOCollisionHandling_callback,
        .data     = (ui_callback_data_t)IO_COLLISION_METHOD_AND_WIRES
    },
    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(iocollision_show_type_callback)
{
    int type;

    resources_get_int("IOCollisionHandling", &type);
    switch (type) {
        case IO_COLLISION_METHOD_DETACH_ALL:
            return MENU_SUBMENU_STRING " detach all";
            break;
        case IO_COLLISION_METHOD_DETACH_LAST:
            return MENU_SUBMENU_STRING " detach last";
            break;
        case IO_COLLISION_METHOD_AND_WIRES:
            return MENU_SUBMENU_STRING " AND values";
            break;
    }
    return "n/a";
}


/** \brief  Data for cartridge menu update functions
 */
typedef struct {
    ui_menu_entry_t *menu;      /**< cartridge menu */
    int              offset;    /**< offset in \a menu */
    int              cart_id;   /**< cartridge ID */
} cartmenu_state_t;

/** \brief  Menu items to be updated using `cartridge_can_flush_image()` */
static const cartmenu_state_t flush_primary_states[] = {
    { ramcart_menu,             RAMCART_OFFSET_FLUSH,       CARTRIDGE_RAMCART },
    { reu_menu,                 REU_OFFSET_FLUSH,           CARTRIDGE_REU },
    { expert_cart_menu,         EXPERT_OFFSET_FLUSH,        CARTRIDGE_EXPERT },
    { dqbb_cart_menu,           DQBB_OFFSET_FLUSH,          CARTRIDGE_DQBB },
    { isepic_cart_menu,         ISEPIC_OFFSET_FLUSH,        CARTRIDGE_ISEPIC },
    { easyflash_cart_menu,      EASYFLASH_OFFSET_FLUSH,     CARTRIDGE_EASYFLASH },
    { georam_menu,              GEORAM_OFFSET_FLUSH,        CARTRIDGE_GEORAM },
    { megabyter_cart_menu,      MEGABYTER_OFFSET_FLUSH,     CARTRIDGE_MEGABYTER },
    { mmc64_cart_menu,          MMC64_OFFSET_FLUSH,         CARTRIDGE_MMC64 },
    { mmcreplay_cart_menu,      MMCR_OFFSET_FLUSH,          CARTRIDGE_MMC_REPLAY },
    { retroreplay_cart_menu,    RETROREPLAY_OFFSET_FLUSH,   CARTRIDGE_RETRO_REPLAY },
    { gmod2_cart_menu,          GMOD2_OFFSET_FLUSH,         CARTRIDGE_GMOD2 },
    { gmod2c128_cart_menu,      GMOD2C128_OFFSET_FLUSH,     CARTRIDGE_C128_GMOD2C128 },
    { rrnet_mk3_cart_menu,      RRNETMK3_OFFSET_FLUSH,      CARTRIDGE_RRNETMK3 },
    { NULL,                     0,                          0 }
};

/** \brief  Menu items to be updated using `cartridge_can_flush_secondary_image()` */
static const cartmenu_state_t flush_secondary_states[] = {
    { mmcreplay_cart_menu,      MMCR_OFFSET_FLUSH_SECONDARY,            CARTRIDGE_MMC_REPLAY },
    { ramlink_menu,             RAMLINK_OFFSET_FLUSH_SECONDARY,         CARTRIDGE_RAMLINK },
    { gmod2_cart_menu,          GMOD2_OFFSET_FLUSH_SECONDARY,           CARTRIDGE_GMOD2 },
    { gmod2c128_cart_menu,      GMOD2C128_OFFSET_FLUSH_SECONDARY,       CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128) },
    { rexramfloppy_menu,        REXRAMFLOPPY_OFFSET_FLUSH_SECONDARY,    CARTRIDGE_REX_RAMFLOPPY },
    { NULL,                     0,                                      0 }
};

/** \brief  Menu items to be updated using `cartridge_can_save_image()` */
static const cartmenu_state_t save_primary_states[] = {
    { ramcart_menu,             RAMCART_OFFSET_SAVE,        CARTRIDGE_RAMCART },
    { reu_menu,                 REU_OFFSET_SAVE,            CARTRIDGE_REU },
    { expert_cart_menu,         EXPERT_OFFSET_SAVE,         CARTRIDGE_REU },
    { dqbb_cart_menu,           DQBB_OFFSET_SAVE,           CARTRIDGE_DQBB },
    { isepic_cart_menu,         ISEPIC_OFFSET_SAVE,         CARTRIDGE_ISEPIC },
    { easyflash_cart_menu,      EASYFLASH_OFFSET_SAVE,      CARTRIDGE_EASYFLASH },
    { georam_menu,              GEORAM_OFFSET_SAVE,         CARTRIDGE_GEORAM },
    { megabyter_cart_menu,      MEGABYTER_OFFSET_SAVE,      CARTRIDGE_MEGABYTER },
    { mmc64_cart_menu,          MMC64_OFFSET_SAVE,          CARTRIDGE_MMC64 },
    { mmcreplay_cart_menu,      MMCR_OFFSET_SAVE,           CARTRIDGE_MMC_REPLAY },
    { retroreplay_cart_menu,    RETROREPLAY_OFFSET_SAVE,    CARTRIDGE_RETRO_REPLAY },
    { gmod2_cart_menu,          GMOD2_OFFSET_SAVE,          CARTRIDGE_GMOD2 },
    { gmod2c128_cart_menu,      GMOD2C128_OFFSET_SAVE,      CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128) },
    { gmod3_cart_menu,          GMOD3_OFFSET_SAVE,          CARTRIDGE_GMOD3 },
    { rrnet_mk3_cart_menu,      RRNETMK3_OFFSET_SAVE,       CARTRIDGE_RRNETMK3 },
    { NULL,                     0,                          0 }
};

/** \brief  Menu items to be updated using `cartridge_can_save_secondary_image()` */
static const cartmenu_state_t save_secondary_states[] = {
    { mmcreplay_cart_menu,  MMCR_OFFSET_SAVE_SECONDARY,         CARTRIDGE_MMC_REPLAY },
    { ramlink_menu,         RAMLINK_OFFSET_SAVE_SECONDARY,      CARTRIDGE_RAMLINK },
    { gmod2_cart_menu,      GMOD2_OFFSET_SAVE_SECONDARY,        CARTRIDGE_GMOD2 },
    { gmod2c128_cart_menu,  GMOD2C128_OFFSET_SAVE_SECONDARY,    CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GMOD2C128) },
    { rexramfloppy_menu,    REXRAMFLOPPY_OFFSET_SAVE_SECONDARY, CARTRIDGE_REX_RAMFLOPPY },
    { NULL,                 0,                                  0 }
};

/** \brief  Update cartridge menu `status` fields using a cartridge API function
 *
 * Set cartridge menu items enabled/disabled state by iterating \a states and
 * calling \a state_func for each element in \a states.
 * The \a state_func function is expected to return 0 for `false` and non-0 for
 * true.
 *
 * \param[in]   states      list of menu items to change the `status` field of
 * \param[in]   state_func  function to call on the cartridge ID in \a states
 */
static void update_cartmenu_states(const cartmenu_state_t *states, int (*state_func)(int))
{
    int i;

    for (i = 0; states[i].menu != NULL; i++) {
        ui_menu_entry_t       *menu    = states[i].menu;
        int                    offset  = states[i].offset;
        int                    cart_id = states[i].cart_id;
        ui_menu_status_type_t  status;

        status = state_func(cart_id) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
        menu[offset].status = status;
    }
}

static void cartmenu_update_flush(void)
{
    /* primary */
    update_cartmenu_states(flush_primary_states, cartridge_can_flush_image);
    /* secondary */
    update_cartmenu_states(flush_secondary_states, cartridge_can_flush_secondary_image);
}

static void cartmenu_update_save(void)
{
    /* primary */
    update_cartmenu_states(save_primary_states, cartridge_can_save_image);
    /* secondary */
    update_cartmenu_states(save_secondary_states, cartridge_can_save_secondary_image);
}

/* Cartridge menu */

UI_MENU_DEFINE_TOGGLE(CartridgeReset)
UI_MENU_DEFINE_TOGGLE(SFXSoundSampler)
UI_MENU_DEFINE_TOGGLE(CPMCart)
UI_MENU_DEFINE_TOGGLE(SSRamExpansion)


ui_menu_entry_t c64cart_menu[] = {
    {   .action   = ACTION_CART_ATTACH,
        .string   = "Attach CRT image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_c64_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CRT
    },
    /* CAUTION: the position of this item is hardcoded above */
    {   .string   = "Attach raw image",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = NULL    /* set by uicart_menu_create() */
    },
    {   .action   = ACTION_CART_DETACH,
        .string   = "Detach cartridge image",
        .type     = MENU_ENTRY_OTHER,
    },

    SDL_MENU_ITEM_SEPARATOR,
    {   .action   = ACTION_CART_FREEZE,
        .string   = "Cartridge freeze",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Set current cartridge as default",
        .type     = MENU_ENTRY_OTHER,
        .callback = set_c64_cart_default_callback
    },
    {   .string   = "Unset default cartridge",
        .type     = MENU_ENTRY_OTHER,
        .callback = unset_c64_cart_default_callback
    },
    {   .string   = "I/O collision handling ($D000-$DFFF)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    {   .string   = "Reset on cartridge change",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CartridgeReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Cartridge specific settings"),
#ifdef HAVE_RAWNET
    {   .string   = CARTRIDGE_NAME_ETHERNETCART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernetcart_menu
    },
#endif
    {   .string   = CARTRIDGE_NAME_RAMCART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ramcart_menu
    },
    {   .string   = CARTRIDGE_NAME_REU,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)reu_menu
    },
    {   .string   = CARTRIDGE_NAME_GEORAM,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)georam_menu
    },
    {   .string   = CARTRIDGE_NAME_IDE64,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu
    },
    {   .string   = CARTRIDGE_NAME_EXPERT,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)expert_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_ISEPIC,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)isepic_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_DQBB,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)dqbb_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_EASYFLASH,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)easyflash_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MMC64,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmc64_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MMC_REPLAY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmcreplay_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_RETRO_REPLAY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)retroreplay_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_GMOD2,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod2_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_GMOD3,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod3_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_LT_KERNAL,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ltk_cart_menu
    },
    {   .string   =  CARTRIDGE_NAME_RAMLINK,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ramlink_menu
    },
    {   .string   = CARTRIDGE_NAME_REX_RAMFLOPPY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rexramfloppy_menu
    },
    {   .string   = CARTRIDGE_NAME_RRNETMK3,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)rrnet_mk3_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MAGIC_VOICE,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)magicvoice_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_EXPANDER " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)soundexpander_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundSampler_callback
    },
    {   .string   = CARTRIDGE_NAME_CPM,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CPMCart_callback
    },
    {   .string   = "Super Snapshot 32KiB RAM",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SSRamExpansion_callback
    },
    {   .string   = CARTRIDGE_NAME_IEEEFLASH64 " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ieeeflash64_menu
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t c128cart_menu[] = {
    {   .action   = ACTION_CART_ATTACH,
        .string   = "Attach CRT image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_c64_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CRT
    },
    /* CAUTION: the position of this item is hardcoded above */
    {   .string   = "Attach raw image",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = NULL    /* set by uicart_menu_create() */
    },
    {   .action   = ACTION_CART_DETACH,
        .string   = "Detach cartridge image",
        .type     = MENU_ENTRY_OTHER,
    },
    SDL_MENU_ITEM_SEPARATOR,
    {   .action   = ACTION_CART_FREEZE,
        .string   = "Cartridge freeze",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Set current cartridge as default",
        .type     = MENU_ENTRY_OTHER,
        .callback = set_c64_cart_default_callback
    },
    {   .string   = "I/O collision handling ($D000-$DFFF)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    {   .string   = "Reset on cartridge change",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CartridgeReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Cartridge specific settings"),
#ifdef HAVE_RAWNET
    {   .string   = CARTRIDGE_NAME_ETHERNETCART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernetcart_menu
    },
#endif
    {   .string   = CARTRIDGE_NAME_RAMCART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ramcart_menu
    },
    {   .string   = CARTRIDGE_NAME_REU,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)reu_menu
    },
    {   .string   = CARTRIDGE_NAME_GEORAM,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)georam_menu
    },
    {   .string   = CARTRIDGE_NAME_IDE64,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu
    },
    {   .string   = CARTRIDGE_NAME_EXPERT,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)expert_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_ISEPIC,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)isepic_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_DQBB,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)dqbb_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_EASYFLASH,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)easyflash_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MMC64,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmc64_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MMC_REPLAY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmcreplay_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_RETRO_REPLAY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)retroreplay_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_GMOD2,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod2_cart_menu
    },
    {   .string   = CARTRIDGE_C128_NAME_GMOD2C128,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod2c128_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_GMOD3,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod3_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MAGIC_VOICE,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)magicvoice_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_LT_KERNAL,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ltk_cart_menu
    },
    {   .string   =  CARTRIDGE_NAME_RAMLINK,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ramlink_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_EXPANDER " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)soundexpander_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundSampler_callback
    },
    {   .string   = CARTRIDGE_NAME_IEEEFLASH64 " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ieeeflash64_menu
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t scpu64cart_menu[] = {
    {   .action   = ACTION_CART_ATTACH,
        .string   = "Attach CRT image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_c64_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CRT
    },
    /* CAUTION: the position of this item is hardcoded above */
    {   .string   = "Attach raw image",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = NULL    /* set by uicart_menu_create() */
    },
    {   .action   = ACTION_CART_DETACH,
        .string   = "Detach cartridge image",
        .type     = MENU_ENTRY_OTHER,
    },
    SDL_MENU_ITEM_SEPARATOR,
    {   .string   = "Set current cartridge as default",
        .type     = MENU_ENTRY_OTHER,
        .callback = set_c64_cart_default_callback
    },
    {   .string   = "I/O collision handling ($D000-$DFFF)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    {   .string   = "Reset on cartridge change",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CartridgeReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Cartridge specific settings"),
#ifdef HAVE_RAWNET
    {   .string   = CARTRIDGE_NAME_ETHERNETCART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ethernetcart_menu
    },
#endif
    {   .string   = CARTRIDGE_NAME_RAMCART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ramcart_menu
    },
    {   .string   = CARTRIDGE_NAME_REU,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)reu_menu
    },
    {   .string   = CARTRIDGE_NAME_GEORAM,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)georam_menu
    },
    {   .string   = CARTRIDGE_NAME_IDE64,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ide64_menu
    },
    {   .string   = CARTRIDGE_NAME_EXPERT,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)expert_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_ISEPIC,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)isepic_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_DQBB,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)dqbb_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_EASYFLASH,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)easyflash_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MMC64,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmc64_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MMC_REPLAY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)mmcreplay_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_RAMLINK,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ramlink_menu
    },
    {   .string   = CARTRIDGE_NAME_RETRO_REPLAY,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)retroreplay_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_GMOD2,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod2_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_GMOD3,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)gmod3_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_MAGIC_VOICE,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)magicvoice_cart_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_EXPANDER " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)soundexpander_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundSampler_callback
    },
    SDL_MENU_LIST_END
};

void uiclockport_rr_mmc_menu_create(void)
{
    int i;

    for (i = 0; clockport_supported_devices[i].name; ++i) {
        mmc64_clockport_device_menu[i].action   = ACTION_NONE;
        mmc64_clockport_device_menu[i].string   = clockport_supported_devices[i].name;
        mmc64_clockport_device_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        mmc64_clockport_device_menu[i].callback = radio_MMC64ClockPort_callback;
        mmc64_clockport_device_menu[i].data     = (ui_callback_data_t)vice_int_to_ptr(clockport_supported_devices[i].id);

        mmcreplay_clockport_device_menu[i].action   = ACTION_NONE;
        mmcreplay_clockport_device_menu[i].string   = clockport_supported_devices[i].name;
        mmcreplay_clockport_device_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        mmcreplay_clockport_device_menu[i].callback = radio_MMCRClockPort_callback;
        mmcreplay_clockport_device_menu[i].data     = (ui_callback_data_t)vice_int_to_ptr(clockport_supported_devices[i].id);

        retroreplay_clockport_device_menu[i].action   = ACTION_NONE;
        retroreplay_clockport_device_menu[i].string   = clockport_supported_devices[i].name;
        retroreplay_clockport_device_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        retroreplay_clockport_device_menu[i].callback = radio_RRClockPort_callback;
        retroreplay_clockport_device_menu[i].data     = (ui_callback_data_t)vice_int_to_ptr(clockport_supported_devices[i].id);
    }

    mmc64_clockport_device_menu[i].action   = ACTION_NONE;
    mmc64_clockport_device_menu[i].string   = NULL;
    mmc64_clockport_device_menu[i].type     = MENU_ENTRY_TEXT;
    mmc64_clockport_device_menu[i].callback = NULL;
    mmc64_clockport_device_menu[i].data     = NULL;

    mmcreplay_clockport_device_menu[i].action   = ACTION_NONE;
    mmcreplay_clockport_device_menu[i].string   = NULL;
    mmcreplay_clockport_device_menu[i].type     = MENU_ENTRY_TEXT;
    mmcreplay_clockport_device_menu[i].callback = NULL;
    mmcreplay_clockport_device_menu[i].data     = NULL;

    retroreplay_clockport_device_menu[i].action   = ACTION_NONE;
    retroreplay_clockport_device_menu[i].string   = NULL;
    retroreplay_clockport_device_menu[i].type     = MENU_ENTRY_TEXT;
    retroreplay_clockport_device_menu[i].callback = NULL;
    retroreplay_clockport_device_menu[i].data     = NULL;
}
