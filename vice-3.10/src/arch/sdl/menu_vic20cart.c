/*
 * menu_vic20cart.c - Implementation of the vic20 cartridge settings menu for the SDL UI.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "cartio.h"
#include "cartridge.h"
#include "keyboard.h"
#include "lib.h"
#include "menu_common.h"
#include "resources.h"
#include "uiactions.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"

#include "menu_vic20cart.h"

/* FIXME: we need an error reporting system, so all this
          stuff can go away. */
typedef struct vic20_cart_flush_s {
    int cartid;
    char *enable_res;
    char *image_res;
} vic20_cart_flush_t;

static vic20_cart_flush_t carts[] = {
    { CARTRIDGE_VIC20_FP,              "VicFlashPluginWriteBack", NULL },
    { CARTRIDGE_VIC20_UM,              "UltiMemWriteBack",        NULL },
    { CARTRIDGE_VIC20_FINAL_EXPANSION, "FinalExpansionWriteBack", NULL },
    { CARTRIDGE_VIC20_MINIMON,         "MinimonImageWrite",       "MinimonFilename" },
    { 0,                               NULL,                      NULL }
};

static vic20_cart_flush_t carts_secondary[] = {
    { CARTRIDGE_VIC20_MEGACART,        "MegaCartNvRAMWriteBack",  "MegaCartNvRAMfilename" },
    { 0,                               NULL,                      NULL }
};

static void cartmenu_update_flush(void);
static void cartmenu_update_save(void);

static UI_MENU_CALLBACK(vic20_cart_flush_callback)
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

static UI_MENU_CALLBACK(vic20_cart_flush_secondary_callback)
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

static UI_MENU_CALLBACK(vic20_cart_save_callback)
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

static UI_MENU_CALLBACK(vic20_cart_save_secondary_callback)
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

/* FIXME: this should be dynamic, like in x64 */
static UI_MENU_CALLBACK(attach_cart_callback)
{
    const char *title;
    char       *name   = NULL;
    int         action = ACTION_NONE;

    if (activated) {
        switch (vice_ptr_to_int(param)) {
            case CARTRIDGE_CRT:
                title  = "Select cartridge image";
                action = ACTION_CART_ATTACH;
                break;
            case CARTRIDGE_VIC20_BEHRBONZ:
                title  = "Select " CARTRIDGE_VIC20_NAME_BEHRBONZ " image";
                action = ACTION_CART_ATTACH_RAW_BEHRBONZ;
                break;
            case CARTRIDGE_VIC20_UM:
                title  = "Select " CARTRIDGE_VIC20_NAME_UM " image";
                action = ACTION_CART_ATTACH_RAW_ULTIMEM;
                break;
            case CARTRIDGE_VIC20_FP:
                title  = "Select " CARTRIDGE_VIC20_NAME_FP " image";
                action = ACTION_CART_ATTACH_RAW_VICFP;
                break;
            case CARTRIDGE_VIC20_MEGACART:
                title  = "Select " CARTRIDGE_VIC20_NAME_MEGACART " image";
                action = ACTION_CART_ATTACH_RAW_MEGACART;
                break;
            case CARTRIDGE_VIC20_MINIMON:
                title  = "Select " CARTRIDGE_VIC20_NAME_MINIMON " image";
                action = ACTION_CART_ATTACH_RAW_MINIMON;
                break;
            case CARTRIDGE_VIC20_FINAL_EXPANSION:
                title  = "Select " CARTRIDGE_VIC20_NAME_FINAL_EXPANSION " image";
                action = ACTION_CART_ATTACH_RAW_FINAL;
                break;
            case CARTRIDGE_VIC20_DETECT:    /* fall through */
            case CARTRIDGE_VIC20_GENERIC:
                title = "Select cartridge image";
                break;
            case CARTRIDGE_VIC20_16KB_2000:
                title  = "Select 4/8/16KiB image";
                action = ACTION_CART_ATTACH_RAW_2000;
                break;
            case CARTRIDGE_VIC20_16KB_4000:
                title  = "Select 4/8/16KiB image";
                action = ACTION_CART_ATTACH_RAW_4000;
                break;
            case CARTRIDGE_VIC20_16KB_6000:
                title  = "Select 4/8/16KiB image";
                action = ACTION_CART_ATTACH_RAW_6000;
                break;
            case CARTRIDGE_VIC20_8KB_A000:
                title  = "Select 4/8KiB image";
                action = ACTION_CART_ATTACH_RAW_A000;
                break;
            case CARTRIDGE_VIC20_4KB_B000:
                action = ACTION_CART_ATTACH_RAW_B000;   /* fall through */
            default:
                title = "Select 4KiB image";
                break;
        }
        name = sdl_ui_file_selection_dialog(title, FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (cartridge_attach_image(vice_ptr_to_int(param), name) < 0) {
                ui_error("Cannot load cartridge image.");
            }
            lib_free(name);
        }
        if (action > ACTION_NONE) {
            ui_action_finish(action);
        }
    }
    return NULL;
}

/* FIXME: this should be dynamic, like in x64 */
static UI_MENU_CALLBACK(add_cart_callback)
{
    const char *title;
    char       *name   = NULL;
    int         action = ACTION_NONE;

    if (activated) {
        switch (vice_ptr_to_int(param)) {
            case CARTRIDGE_VIC20_DETECT:    /* fall through */
            case CARTRIDGE_VIC20_GENERIC:
                title = "Select cartridge image";
                break;
            case CARTRIDGE_VIC20_16KB_2000:
                title  = "Select 4/8/16KiB image";
                action = ACTION_CART_ATTACH_RAW_2000;
                break;
            case CARTRIDGE_VIC20_16KB_4000:
                title  = "Select 4/8/16KiB image";
                action = ACTION_CART_ATTACH_RAW_4000;
                break;
            case CARTRIDGE_VIC20_16KB_6000:
                title  = "Select 4/8/16KiB image";
                action = ACTION_CART_ATTACH_RAW_6000;
                break;
            case CARTRIDGE_VIC20_8KB_A000:
                title  = "Select 4/8KiB image";
                action = ACTION_CART_ATTACH_RAW_A000;
                break;
            case CARTRIDGE_VIC20_4KB_B000:
                action = ACTION_CART_ATTACH_RAW_B000;   /* fall through */
            default:
                title = "Select 4KiB image";
                break;
        }
        name = sdl_ui_file_selection_dialog(title, FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (cartridge_attach_add_image(vice_ptr_to_int(param), name) < 0) {
                ui_error("Cannot load cartridge image.");
            }
            lib_free(name);
        }
        if (action > ACTION_NONE) {
            ui_action_finish(action);
        }
    }
    return NULL;
}

/* TODO:    Create UI action IDs/names for these items
 *          Smart-attach can probably use `ACTION_CART_ATTACH`, the rest needs
 *          new IDs like `ACTION_CART_ATTACH_2000`.
 */
static const ui_menu_entry_t attach_generic_cart_submenu[] = {
    {   .string   = "Smart-attach cartridge image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_DETECT
    },
    {   .action   = ACTION_CART_ATTACH_RAW_2000,
        .string   = "Attach 4/8/16KiB image at $2000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_16KB_2000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_4000,
        .string   = "Attach 4/8/16KiB image at $4000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_16KB_4000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_6000,
        .string   = "Attach 4/8/16KiB image at $6000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_16KB_6000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_A000,
        .string   = "Attach 4/8KiB image at $A000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_8KB_A000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_B000,
        .string   = "Attach 4KiB image at $B000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_4KB_B000
    },
    SDL_MENU_LIST_END
};

/* TODO:    Create UI action IDs/names for these items
 *          Smart-attach can probably use `ACTION_CART_ATTACH_ADD`, the rest needs
 *          new IDs like `ACTION_CART_ATTACH_ADD_2000`.
 */
static const ui_menu_entry_t add_to_generic_cart_submenu[] = {
    {   .string   = "Smart-attach cartridge image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = add_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_DETECT
    },
    {   .action   = ACTION_CART_ATTACH_RAW_2000,
        .string   = "Attach 4/8/16KiB image at $2000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = add_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_16KB_2000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_4000,
        .string   = "Attach 4/8/16KiB image at $4000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = add_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_16KB_4000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_6000,
        .string   = "Attach 4/8/16KiB image at $6000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = add_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_16KB_6000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_A000,
        .string   = "Attach 4/8KiB image at $A000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = add_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_8KB_A000
    },
    {   .action   = ACTION_CART_ATTACH_RAW_B000,
        .string   = "Attach 4KiB image at $B000",
        .type     = MENU_ENTRY_DIALOG,
        .callback = add_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_4KB_B000
    },
    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(set_cart_default_callback)
{
    if (activated) {
        cartridge_set_default();
    }
    return NULL;
}

static UI_MENU_CALLBACK(unset_cart_default_callback)
{
    if (activated) {
        cartridge_unset_default();
    }
    return NULL;
}

UI_MENU_DEFINE_TOGGLE(FinalExpansionWriteBack)
UI_MENU_DEFINE_TOGGLE(UltiMemWriteBack)
UI_MENU_DEFINE_TOGGLE(VicFlashPluginWriteBack)
UI_MENU_DEFINE_TOGGLE(MegaCartNvRAMWriteBack)
UI_MENU_DEFINE_FILE_STRING(MegaCartNvRAMfilename)

UI_MENU_DEFINE_TOGGLE(MinimonEnabled)
UI_MENU_DEFINE_TOGGLE(MinimonImageWrite)
UI_MENU_DEFINE_FILE_STRING(MinimonFilename)
UI_MENU_DEFINE_TOGGLE(MinimonIoSwitch)
UI_MENU_DEFINE_TOGGLE(MinimonPgmSwitch)

#define OFFS_FLUSH_MINI 8
#define OFFS_FLUSH_FE   2
#define OFFS_FLUSH_UM   2
#define OFFS_FLUSH_FP   2
#define OFFS_FLUSH_MEGACART   3

#define OFFS_SAVE_MINI  9
#define OFFS_SAVE_FE    3
#define OFFS_SAVE_UM    3
#define OFFS_SAVE_FP    3
#define OFFS_SAVE_MEGACART   4

static ui_menu_entry_t minimon_cart_menu[] = {
    {   .string   = "Enable " CARTRIDGE_VIC20_NAME_MINIMON,     /* 0 */
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MinimonEnabled_callback
    },
    SDL_MENU_ITEM_SEPARATOR,                                    /* 1 */

    {   .string   = "enable ROM in IO2/3",                      /* 2 */
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MinimonIoSwitch_callback
    },
    {   .string   = "enable PGM switch",                        /* 3 */
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MinimonPgmSwitch_callback
    },
    SDL_MENU_ITEM_SEPARATOR,                                    /* 4 */

    SDL_MENU_ITEM_TITLE(CARTRIDGE_VIC20_NAME_MINIMON " image"), /* 5 */
    {   .string   = "Image file",                               /* 6 */
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MinimonFilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_VIC20_NAME_MINIMON " ROM image"
    },
    {   .string   = "Save image on detach.",                    /* 7 */
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MinimonImageWrite_callback
    },
    {   .string   = "Save image now",                           /* 8 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_MINIMON
    },
    {   .string   = "Save image as",                            /* 9 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_MINIMON
    },
    SDL_MENU_LIST_END
};

static ui_menu_entry_t finalexpansion_cart_menu[] = {
    SDL_MENU_ITEM_TITLE(CARTRIDGE_VIC20_NAME_FINAL_EXPANSION " settings"),
    {   .string   = "Save image on detach.",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_FinalExpansionWriteBack_callback
    },
    {   .string   = "Save image now", /* 2 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_FINAL_EXPANSION
    },
    {   .string   = "Save image as", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_FINAL_EXPANSION
    },
    SDL_MENU_LIST_END
};

static ui_menu_entry_t vic_um_cart_menu[] = {
    SDL_MENU_ITEM_TITLE(CARTRIDGE_VIC20_NAME_UM " settings"),
    {   .string   = "Save image on detach.",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_UltiMemWriteBack_callback
    },
    {   .string   = "Save image now", /* 2 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_UM
    },
    {   .string   = "Save image as", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_UM
    },
    SDL_MENU_LIST_END
};

static ui_menu_entry_t vic_fp_cart_menu[] = {
    SDL_MENU_ITEM_TITLE(CARTRIDGE_VIC20_NAME_FP " settings"),
    {   .string   = "Save image on detach.",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VicFlashPluginWriteBack_callback
    },
    {   .string   = "Save image now", /* 2 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_FP
    },
    {   .string   = "Save image as", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_FP
    },
    SDL_MENU_LIST_END
};

static ui_menu_entry_t megacart_cart_menu[] = {
    SDL_MENU_ITEM_TITLE(CARTRIDGE_VIC20_NAME_MEGACART " settings"),
    {   .string   = "NvRAM Image file",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_MegaCartNvRAMfilename_callback,
        .data     = (ui_callback_data_t)"Select " CARTRIDGE_VIC20_NAME_MEGACART " NvRAM image"
    },
    {   .string   = "Save NvRAM image on detach.",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_MegaCartNvRAMWriteBack_callback
    },
    {   .string   = "Save NvRAM image now", /* 3 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_flush_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_MEGACART
    },
    {   .string   = "Save NvRAM image as", /* 4 */
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_save_secondary_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_MEGACART
    },
    SDL_MENU_LIST_END
};

/* GEORAM */

UI_MENU_DEFINE_TOGGLE(GEORAM)
UI_MENU_DEFINE_TOGGLE(GEORAMIOSwap)
UI_MENU_DEFINE_RADIO(GEORAMsize)
UI_MENU_DEFINE_FILE_STRING(GEORAMfilename)
UI_MENU_DEFINE_TOGGLE(GEORAMImageWrite)

static ui_menu_entry_t georam_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_GEORAM,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GEORAM_callback
    },
    {   .string   = "Swap " CARTRIDGE_NAME_GEORAM " I/O",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_GEORAMIOSwap_callback
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
    {   .string   = "Save image now",
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_flush_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_GEORAM
    },
    {   .string   = "Save image as",
        .type     = MENU_ENTRY_OTHER,
        .callback = vic20_cart_save_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_GEORAM
    },
    SDL_MENU_LIST_END
};


/* SFX Sound Expander */

UI_MENU_DEFINE_TOGGLE(SFXSoundExpander)
UI_MENU_DEFINE_TOGGLE(SFXSoundExpanderIOSwap)
UI_MENU_DEFINE_RADIO(SFXSoundExpanderChip)

static const ui_menu_entry_t soundexpander_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundExpander_callback
    },
    {   .string   = "Swap " CARTRIDGE_NAME_SFX_SOUND_EXPANDER " I/O",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundExpanderIOSwap_callback
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


/* SFX Sound Sampler */

UI_MENU_DEFINE_TOGGLE(SFXSoundSampler)
UI_MENU_DEFINE_TOGGLE(SFXSoundSamplerIOSwap)

static const ui_menu_entry_t soundsampler_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundSampler_callback
    },
    {   .string   = "Swap " CARTRIDGE_NAME_SFX_SOUND_SAMPLER " I/O",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SFXSoundSamplerIOSwap_callback
    },
    SDL_MENU_LIST_END
};


/* DIGIMAX MENU */

UI_MENU_DEFINE_TOGGLE(DIGIMAX)
UI_MENU_DEFINE_RADIO(DIGIMAXbase)

static const ui_menu_entry_t digimax_vic20_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_DIGIMAX,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DIGIMAX_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Base address"),
    {   .string   = "$9800",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9800
    },
    {   .string   = "$9820",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9820
    },
    {   .string   = "$9840",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9840
    },
    {   .string   = "$9860",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9860
    },
    {   .string   = "$9880",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9880
    },
    {   .string   = "$98A0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x98a0
    },
    {   .string   = "$98C0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x98c0
    },
    {   .string   = "$98E0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x98e0
    },
    {   .string   = "$9C00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9c00
    },
    {   .string   = "$9C20",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9c20
    },
    {   .string   = "$9C40",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9c40
    },
    {   .string   = "$9C60",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9c60
    },
    {   .string   = "$9C80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9c80
    },
    {   .string   = "$9CA0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9ca0
    },
    {   .string   = "$9CC0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9cc0
    },
    {   .string   = "$9CE0",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DIGIMAXbase_callback,
        .data     = (ui_callback_data_t)0x9ce0
    },
    SDL_MENU_LIST_END
};


/* DS12C887 RTC MENU */

UI_MENU_DEFINE_TOGGLE(DS12C887RTC)
UI_MENU_DEFINE_TOGGLE(DS12C887RTCRunMode)
UI_MENU_DEFINE_RADIO(DS12C887RTCbase)
UI_MENU_DEFINE_TOGGLE(DS12C887RTCSave)

static const ui_menu_entry_t ds12c887rtc_vic20_menu[] = {
    {   .string   = "Enable " CARTRIDGE_NAME_DS12C887RTC,
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTC_callback
    },
    {   .string   = "Start with running oscillator",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTCRunMode_callback
    },
    {   .string   = "Save RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DS12C887RTCSave_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Base address"),
    {   .string   = "$9800",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0x9800
    },
    {   .string   = "$9C00",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_DS12C887RTCbase_callback,
        .data     = (ui_callback_data_t)0x9c00
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

static void cartmenu_update_flush(void)
{
    /* slot 0 */
    minimon_cart_menu[OFFS_FLUSH_MINI].status = cartridge_can_flush_image(CARTRIDGE_VIC20_MINIMON) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    /* main slot */
    megacart_cart_menu[OFFS_FLUSH_MEGACART].status = cartridge_can_flush_secondary_image(CARTRIDGE_VIC20_MEGACART) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    vic_um_cart_menu[OFFS_FLUSH_UM].status = cartridge_can_flush_image(CARTRIDGE_VIC20_UM) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    vic_fp_cart_menu[OFFS_FLUSH_FP].status = cartridge_can_flush_image(CARTRIDGE_VIC20_FP) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    finalexpansion_cart_menu[OFFS_FLUSH_FE].status = cartridge_can_flush_image(CARTRIDGE_VIC20_FINAL_EXPANSION) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    /* io slot */
    georam_menu[12].status = cartridge_can_flush_image(CARTRIDGE_VIC20_GEORAM) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
}

static void cartmenu_update_save(void)
{
    /* slot 0 */
    minimon_cart_menu[OFFS_SAVE_MINI].status = cartridge_can_save_image(CARTRIDGE_VIC20_MINIMON) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    /* main slot */
    megacart_cart_menu[OFFS_SAVE_FE].status = cartridge_can_save_secondary_image(CARTRIDGE_VIC20_MEGACART) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    vic_um_cart_menu[OFFS_SAVE_UM].status = cartridge_can_save_image(CARTRIDGE_VIC20_UM) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    vic_fp_cart_menu[OFFS_SAVE_FP].status = cartridge_can_save_image(CARTRIDGE_VIC20_FP) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    finalexpansion_cart_menu[OFFS_SAVE_FE].status = cartridge_can_save_image(CARTRIDGE_VIC20_FINAL_EXPANSION) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    /* io slot */
    georam_menu[13].status = cartridge_can_save_image(CARTRIDGE_VIC20_GEORAM) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
}

/* Cartridge menu */

UI_MENU_DEFINE_TOGGLE(CartridgeReset)
UI_MENU_DEFINE_TOGGLE(IO2RAM)
UI_MENU_DEFINE_TOGGLE(IO3RAM)

const ui_menu_entry_t vic20cart_menu[] = {
    {   .action   = ACTION_CART_ATTACH,
        .string   = "Attach CRT image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_CRT
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Attach generic cartridge image",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)attach_generic_cart_submenu
    },
    {   .action   = ACTION_CART_ATTACH_RAW_BEHRBONZ,
        .string   = "Attach " CARTRIDGE_VIC20_NAME_BEHRBONZ " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_BEHRBONZ
    },
    {   .action   = ACTION_CART_ATTACH_RAW_MEGACART,
        .string   = "Attach " CARTRIDGE_VIC20_NAME_MEGACART " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_MEGACART
    },
    {   .action   = ACTION_CART_ATTACH_RAW_MINIMON,
        .string   = "Attach " CARTRIDGE_VIC20_NAME_MINIMON " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_MINIMON
    },
    {   .action   = ACTION_CART_ATTACH_RAW_FINAL,
        .string   = "Attach " CARTRIDGE_VIC20_NAME_FINAL_EXPANSION " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_FINAL_EXPANSION
    },
    {   .action   = ACTION_CART_ATTACH_RAW_ULTIMEM,
        .string   = "Attach " CARTRIDGE_VIC20_NAME_UM " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_UM
    },
    {   .action   = ACTION_CART_ATTACH_RAW_VICFP,
        .string   = "Attach " CARTRIDGE_VIC20_NAME_FP " image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_cart_callback,
        .data     = (ui_callback_data_t)CARTRIDGE_VIC20_FP
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Add to generic cartridge",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)add_to_generic_cart_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_CART_DETACH,
        .string   = "Detach cartridge image",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Set current cartridge as default",
        .type     = MENU_ENTRY_OTHER,
        .callback = set_cart_default_callback
    },
    {   .string   = "Unset default cartridge",
        .type     = MENU_ENTRY_OTHER,
        .callback = unset_cart_default_callback
    },
    {   .string   = "I/O collision handling ($9000-$93FF/$9800-$9FFF)",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = iocollision_show_type_callback,
        .data     = (ui_callback_data_t)iocollision_menu
    },
    {   .string   = "Reset on cartridge change",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CartridgeReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = CARTRIDGE_VIC20_NAME_FINAL_EXPANSION,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)finalexpansion_cart_menu
    },
    {   .string   = CARTRIDGE_VIC20_NAME_UM,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vic_um_cart_menu
    },
    {   .string   = CARTRIDGE_VIC20_NAME_FP,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vic_fp_cart_menu
    },
    {   .string   = CARTRIDGE_VIC20_NAME_MEGACART,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)megacart_cart_menu
    },
    {   .string   = CARTRIDGE_VIC20_NAME_MINIMON,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)minimon_cart_menu
    },
    {   .string   = "I/O-2 RAM",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IO2RAM_callback
    },
    {   .string   = "I/O-3 RAM",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IO3RAM_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("MasC=uerade specific cart settings"),
    {   .string   = CARTRIDGE_NAME_GEORAM,
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)georam_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_EXPANDER " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)soundexpander_menu
    },
    {   .string   = CARTRIDGE_NAME_SFX_SOUND_SAMPLER " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)soundsampler_menu
    },
    {   .string   = CARTRIDGE_NAME_DIGIMAX " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)digimax_vic20_menu
    },
    {   .string   = CARTRIDGE_NAME_DS12C887RTC " settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ds12c887rtc_vic20_menu
    },
    SDL_MENU_LIST_END
};
