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

#include "c64model.h"
#include "cia.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_sid.h"
#include "uimenu.h"
#include "vicii.h"

/* ------------------------------------------------------------------------- */
/* common */

static UI_MENU_CALLBACK(custom_C64Model_callback)
{
    int model, selected;

    selected = vice_ptr_to_int(param);

    if (activated) {
        c64model_set(selected);
    } else {
        model = c64model_get();

        if (selected == model) {
            return sdl_menu_text_tick;
        }
    }

    return NULL;
}

static const ui_menu_entry_t c64_model_submenu[] = {
    { "C64 PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64_PAL },
    { "C64C PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64C_PAL },
    { "C64 old PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64_OLD_PAL },
    { "C64 NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64_NTSC },
    { "C64C NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64C_NTSC },
    { "C64 old NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64_OLD_NTSC },
    { "Drean",
      MENU_ENTRY_RESOURCE_RADIO,
      custom_C64Model_callback,
      (ui_callback_data_t)C64MODEL_C64_PAL_N },
	{ "C64 SX PAL",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_C64SX_PAL },
	{ "C64 SX NTSC",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_C64SX_NTSC },
	{ "Japanese",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_C64_JAP },
	{ "C64 GS",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_C64_GS },
	{ "PET64 PAL",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_PET64_PAL },
	{ "PET64 NTSC",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_PET64_NTSC },
	{ "MAX Machine",
		MENU_ENTRY_RESOURCE_RADIO,
		custom_C64Model_callback,
		(ui_callback_data_t)C64MODEL_ULTIMAX },
	SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(custom_sidsubmenu_callback)
{
    /* Display the SID model by using the submenu radio callback
       on the first submenu (SID model) of the SID settings. */
    return submenu_radio_callback(0, sid_c64_menu[0].data);
}

UI_MENU_DEFINE_TOGGLE(VICIINewLuminances)

#define CIA_MODEL_MENU(xyz)           \
UI_MENU_DEFINE_RADIO(CIA##xyz##Model) \
static const ui_menu_entry_t cia##xyz##_model_submenu[] = { \
    { "6526  (old)",                                        \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      radio_CIA##xyz##Model_callback,                       \
      (ui_callback_data_t)CIA_MODEL_6526 },                 \
    { "6526A (new)",                                        \
      MENU_ENTRY_RESOURCE_TOGGLE,                           \
      radio_CIA##xyz##Model_callback,                       \
      (ui_callback_data_t)CIA_MODEL_6526A },                \
    SDL_MENU_LIST_END                                       \
};

CIA_MODEL_MENU(1)
CIA_MODEL_MENU(2)

/* ------------------------------------------------------------------------- */
/* x64sc */

UI_MENU_DEFINE_RADIO(VICIIModel)

static const ui_menu_entry_t viciisc_model_submenu[] = {
    { "6569 (PAL)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_6569 },
    { "8565 (PAL)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_8565 },
    { "6569R1 (old PAL)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_6569R1 },
    { "6567 (NTSC)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_6567 },
    { "8562 (NTSC)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_8562 },
    { "6567R56A (old NTSC)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_6567R56A },
    { "6572 (PAL-N)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIModel_callback,
      (ui_callback_data_t)VICII_MODEL_6572 },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_RADIO(GlueLogic)

const ui_menu_entry_t c64sc_model_menu[] = {
    { "C64 model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)c64_model_submenu },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)viciisc_model_submenu },
    { "New luminances",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIINewLuminances_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "SID settings",
      MENU_ENTRY_SUBMENU,
      custom_sidsubmenu_callback,
      (ui_callback_data_t)sid_c64_menu },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("CIA models"),
    { "CIA 1 model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)cia1_model_submenu },
    { "CIA 2 model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)cia2_model_submenu },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Glue logic"),
    { "Discrete",
      MENU_ENTRY_RESOURCE_TOGGLE,
      radio_GlueLogic_callback,
      (ui_callback_data_t)0 },
    { "Custom IC",
      MENU_ENTRY_RESOURCE_TOGGLE,
      radio_GlueLogic_callback,
      (ui_callback_data_t)1 },
    SDL_MENU_LIST_END
};

/* ------------------------------------------------------------------------- */
/* x64 */

UI_MENU_DEFINE_RADIO(MachineVideoStandard)

static const ui_menu_entry_t video_standard_submenu[] = {
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    { "Old NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSCOLD },
    { "PAL-N",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PALN },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t c64_model_menu[] = {
    { "C64 model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)c64_model_submenu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video standard",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)video_standard_submenu },
    { "New luminances",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIINewLuminances_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "SID settings",
      MENU_ENTRY_SUBMENU,
      custom_sidsubmenu_callback,
      (ui_callback_data_t)sid_c64_menu },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("CIA models"),
    { "CIA 1 model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)cia1_model_submenu },
    { "CIA 2 model",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)cia2_model_submenu },
    SDL_MENU_LIST_END
};
