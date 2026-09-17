/*
 * menu_c64model.c - C64 model menu for SDL UI.
 *
 * Written by
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

#include "types.h"

#include "c64gluelogic.h"
#include "c64iec.h"
#include "c64model.h"
#include "c64rom.h"
#include "cia.h"
#include "machine.h"
#include "menu_c64hw.h"
#include "menu_common.h"
#include "menu_drive.h"
#include "menu_sid.h"
#include "menu_tape.h"
#include "resources.h"
#include "tapeport.h"
#include "uimenu.h"
#include "vicii.h"

/* ------------------------------------------------------------------------- */
/* common */

static UI_MENU_CALLBACK(custom_C64Model_callback)
{
    int model, selected;
    int has_iec;
    int has_tapeport;

    selected = vice_ptr_to_int(param);

    if (activated) {
        c64model_set(selected);
        c64_create_machine_menu();
        has_iec = c64iec_get_active_state();
        uidrive_menu_create(has_iec);
        has_tapeport = tapeport_get_active_state();
        uitape_menu_create(has_tapeport);
    } else {
        model = c64model_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

static const ui_menu_entry_t c64_model_submenu[] = {
    {   .string   = "C64 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_PAL
    },
    {   .string   = "C64C PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64C_PAL
    },
    {   .string   = "C64 old PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_OLD_PAL
    },
    {   .string   = "C64 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_NTSC
    },
    {   .string   = "C64C NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64C_NTSC
    },
    {   .string   = "C64 old NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_OLD_NTSC
    },
    {   .string   = "Drean",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_PAL_N
    },
    {   .string   = "C64 SX PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64SX_PAL
    },
    {   .string   = "C64 SX NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64SX_NTSC
    },
    {   .string   = "Japanese",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_JAP
    },
    {   .string   = "C64 GS",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_GS
    },
    {   .string   = "PET64 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_PET64_PAL
    },
    {   .string   = "PET64 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_PET64_NTSC
    },
    {   .string   = "MAX Machine",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_ULTIMAX
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t scpu64_model_submenu[] = {
    {   .string   = "C64 PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_PAL
    },
    {   .string   = "C64C PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64C_PAL
    },
    {   .string   = "C64 old PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_OLD_PAL
    },
    {   .string   = "C64 NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_NTSC
    },
    {   .string   = "C64C NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64C_NTSC
    },
    {   .string   = "C64 old NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_OLD_NTSC
    },
    {   .string   = "Drean",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_PAL_N
    },
    {   .string   = "C64 SX PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64SX_PAL
    },
    {   .string   = "C64 SX NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64SX_NTSC
    },
    {   .string   = "Japanese",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_JAP
    },
    {   .string   = "C64 GS",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = custom_C64Model_callback,
        .data     = (ui_callback_data_t)C64MODEL_C64_GS
    },
    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(custom_sidsubmenu_callback)
{
    /* Display the SID model by using the submenu radio callback
       on the first submenu (SID model) of the SID settings. */
    return submenu_radio_callback(0, sid_c64_menu[0].data);
}

#define CIA_MODEL_MENU(xyz)                                     \
    UI_MENU_DEFINE_RADIO(CIA##xyz##Model)                       \
    static const ui_menu_entry_t cia##xyz##_model_submenu[] = { \
        {   .string   = "6526  (old)",                          \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
            .callback = radio_CIA##xyz##Model_callback,         \
            .data     = (ui_callback_data_t)CIA_MODEL_6526      \
        },                                                      \
        {   .string   = "8521 (new)",                           \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
            .callback = radio_CIA##xyz##Model_callback,         \
            .data     = (ui_callback_data_t)CIA_MODEL_6526A     \
        },                                                      \
        SDL_MENU_LIST_END                                       \
    };

CIA_MODEL_MENU(1)
CIA_MODEL_MENU(2)

UI_MENU_DEFINE_TOGGLE(IECReset)

UI_MENU_DEFINE_RADIO(KernalRev)

UI_MENU_DEFINE_RADIO(MachinePowerFrequency)

static const ui_menu_entry_t power_freq_submenu[] = {
    {   .string   = "50Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachinePowerFrequency_callback,
        .data     = (ui_callback_data_t)50
    },
    {   .string   = "60Hz",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachinePowerFrequency_callback,
        .data     = (ui_callback_data_t)60
    },
    SDL_MENU_LIST_END
};


static const ui_menu_entry_t kernal_rev_submenu[] = {
    {   .string   = "Rev 1",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_REV1
    },
    {   .string   = "Rev 2",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_REV2
    },
    {   .string   = "Rev 3",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_REV3
    },
    {   .string   = "SX-64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_SX64
    },
    {   .string   = "C64 GS",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_GS64
    },
    {   .string   = "4064",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_4064
    },
    {   .string   = "Japanese",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KernalRev_callback,
        .data     = (ui_callback_data_t)C64_KERNAL_JAP
    },
    SDL_MENU_LIST_END
};

/* ------------------------------------------------------------------------- */
/* x64sc */

UI_MENU_DEFINE_RADIO(VICIIModel)

static const ui_menu_entry_t viciisc_model_submenu[] = {
    {   .string   = "6569 (PAL)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_6569
    },
    {   .string   = "8565 (PAL)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_8565
    },
    {   .string   = "6569R1 (old PAL)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_6569R1
    },
    {   .string   = "6567 (NTSC)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_6567
    },
    {   .string   = "8562 (NTSC)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_8562
    },
    {   .string   = "6567R56A (old NTSC)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_6567R56A
    },
    {   .string   = "6572 (PAL-N)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VICIIModel_callback,
        .data     = (ui_callback_data_t)VICII_MODEL_6572
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(GlueLogic)

const ui_menu_entry_t c64sc_model_menu[] = {
    {   .string   = "C64 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)c64_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,
    {   .string   = "VICII model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)viciisc_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "SID settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = custom_sidsubmenu_callback,
        .data     = (ui_callback_data_t)sid_c64_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("CIA models"),
    {   .string   = "CIA 1 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia1_model_submenu
    },
    {   .string   = "CIA 2 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia2_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Glue logic"),
    {   .string   = "Discrete",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = radio_GlueLogic_callback,
        .data     = (ui_callback_data_t)GLUE_LOGIC_DISCRETE
    },
    {   .string   = "Custom IC",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = radio_GlueLogic_callback,
        .data     = (ui_callback_data_t)GLUE_LOGIC_CUSTOM_IC
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Reset goes to IEC",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IECReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Kernal revision",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)kernal_rev_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Power grid frequency",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)power_freq_submenu
    },
    SDL_MENU_LIST_END
};

/* ------------------------------------------------------------------------- */
/* xscpu64 */

const ui_menu_entry_t scpu64_model_menu[] = {
    {   .string   = "C64 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)scpu64_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)viciisc_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "SID settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = custom_sidsubmenu_callback,
        .data     = (ui_callback_data_t)sid_c64_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("CIA models"),
    {   .string   = "CIA 1 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia1_model_submenu
    },
    {   .string   = "CIA 2 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia2_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Glue logic"),
    {   .string   = "Discrete",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = radio_GlueLogic_callback,
        .data     = (ui_callback_data_t)GLUE_LOGIC_DISCRETE
    },
    {   .string   = "Custom IC",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = radio_GlueLogic_callback,
        .data     = (ui_callback_data_t)GLUE_LOGIC_CUSTOM_IC
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Reset goes to IEC",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IECReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Power grid frequency",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)power_freq_submenu
    },
    SDL_MENU_LIST_END
};

/* ------------------------------------------------------------------------- */
/* x64 */

#define VICMODEL_UNKNOWN -1
#define VICMODEL_NUM 5

struct vicmodel_s {
    int video;
    int luma;
};

static struct vicmodel_s vicmodels[] = {
    { MACHINE_SYNC_PAL,     1 }, /* VICII_MODEL_PALG */
    { MACHINE_SYNC_PAL,     0 }, /* VICII_MODEL_PALG_OLD */
    { MACHINE_SYNC_NTSC,    1 }, /* VICII_MODEL_NTSCM */
    { MACHINE_SYNC_NTSCOLD, 0 }, /* VICII_MODEL_NTSCM_OLD */
    { MACHINE_SYNC_PALN,    1 }  /* VICII_MODEL_PALN */
};

static int vicmodel_get_temp(int video)
{
    int i;

    for (i = 0; i < VICMODEL_NUM; ++i) {
        if (vicmodels[i].video == video) {
            return i;
        }
    }

    return VICMODEL_UNKNOWN;
}

static int vicmodel_get(void)
{
    int video;

    if (resources_get_int("MachineVideoStandard", &video) < 0) {
        return -1;
    }

    return vicmodel_get_temp(video);
}

static void vicmodel_set(int model)
{
    int old_model;

    old_model = vicmodel_get();

    if ((model == old_model) || (model == VICMODEL_UNKNOWN)) {
        return;
    }

    resources_set_int("MachineVideoStandard", vicmodels[model].video);
}

static const char *x64_ui_vicii_model(int activated, ui_callback_data_t param)
{
    int val = vice_ptr_to_int(param);

    if (activated) {
        vicmodel_set(val);
    } else {
        int v = vicmodel_get();

        if (v == val) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static const ui_menu_entry_t vicii_model_submenu[] = {
    {   .string   = "PAL-G",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = x64_ui_vicii_model,
        .data     = (ui_callback_data_t)VICII_MODEL_PALG
    },
    {   .string   = "Old PAL-G",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = x64_ui_vicii_model,
        .data     = (ui_callback_data_t)VICII_MODEL_PALG_OLD
    },
    {   .string   = "NTSC-M",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = x64_ui_vicii_model,
        .data     = (ui_callback_data_t)VICII_MODEL_NTSCM
    },
    {   .string   = "Old NTSC-M",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = x64_ui_vicii_model,
        .data     = (ui_callback_data_t)VICII_MODEL_NTSCM_OLD
    },
    {   .string   = "PAL-N",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = x64_ui_vicii_model,
        .data     = (ui_callback_data_t)VICII_MODEL_PALN
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t c64_model_menu[] = {
    {   .string   = "C64 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)c64_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "SID settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = custom_sidsubmenu_callback,
        .data     = (ui_callback_data_t)sid_c64_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("CIA models"),
    {   .string   = "CIA 1 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia1_model_submenu
    },
    {   .string   = "CIA 2 model",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)cia2_model_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Reset goes to IEC",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_IECReset_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Kernal revision",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)kernal_rev_submenu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Power grid frequency",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)power_freq_submenu
    },
    SDL_MENU_LIST_END
};
