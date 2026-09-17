/** \file   menu_tape.c
 * \brief   Tape menu for SDL UI
 *
 * \author  Hannu Nuotio <hannu.nuotio@tut.fi>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 */

/*
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
#include "types.h"

#include "attach.h"
#include "cbmimage.h"
#include "datasette.h"
#include "diskimage.h"
#include "tapecart.h"
#include "lib.h"
#include "menu_common.h"
#include "menu_tape.h"
#include "tape.h"
#include "tapeport.h"
#include "ui.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "uimsgbox.h"
#include "util.h"

UI_MENU_DEFINE_RADIO(TapePort1Device)
UI_MENU_DEFINE_RADIO(TapePort2Device)

static ui_menu_entry_t tapeport_dyn_menu[TAPEPORT_MAX_PORTS][TAPEPORT_MAX_DEVICES + 1];

static int tapeport_dyn_menu_init[TAPEPORT_MAX_PORTS] = { 0 };

static void sdl_menu_tapeport_free(int port)
{
    int i;

    for (i = 0; tapeport_dyn_menu[port][i].string != NULL; i++) {
        lib_free(tapeport_dyn_menu[port][i].string);
    }
}

static UI_MENU_CALLBACK(TapePort1Device_dynmenu_callback)
{
    tapeport_desc_t *devices = tapeport_get_valid_devices(TAPEPORT_PORT_1, 1);
    int i;

    /* rebuild menu if it already exists. */
    if (tapeport_dyn_menu_init[TAPEPORT_PORT_1] != 0) {
        sdl_menu_tapeport_free(TAPEPORT_PORT_1);
    } else {
        tapeport_dyn_menu_init[TAPEPORT_PORT_1] = 1;
    }

    for (i = 0; devices[i].name; ++i) {
        tapeport_dyn_menu[TAPEPORT_PORT_1][i].action   = ACTION_NONE;
        tapeport_dyn_menu[TAPEPORT_PORT_1][i].string   = lib_strdup(devices[i].name);
        tapeport_dyn_menu[TAPEPORT_PORT_1][i].type     = MENU_ENTRY_RESOURCE_RADIO;
        tapeport_dyn_menu[TAPEPORT_PORT_1][i].callback = radio_TapePort1Device_callback;
        tapeport_dyn_menu[TAPEPORT_PORT_1][i].data     = (ui_callback_data_t)vice_int_to_ptr(devices[i].id);
    }
    tapeport_dyn_menu[TAPEPORT_PORT_1][i].string = NULL;

    lib_free(devices);

    return MENU_SUBMENU_STRING;
}

static UI_MENU_CALLBACK(TapePort2Device_dynmenu_callback)
{
    tapeport_desc_t *devices = tapeport_get_valid_devices(TAPEPORT_PORT_2, 1);
    int i;

    /* rebuild menu if it already exists. */
    if (tapeport_dyn_menu_init[TAPEPORT_PORT_2] != 0) {
        sdl_menu_tapeport_free(TAPEPORT_PORT_2);
    } else {
        tapeport_dyn_menu_init[TAPEPORT_PORT_2] = 1;
    }

    for (i = 0; devices[i].name; ++i) {
        tapeport_dyn_menu[TAPEPORT_PORT_2][i].action   = ACTION_NONE;
        tapeport_dyn_menu[TAPEPORT_PORT_2][i].string   = (char *)lib_strdup(devices[i].name);
        tapeport_dyn_menu[TAPEPORT_PORT_2][i].type     = MENU_ENTRY_RESOURCE_RADIO;
        tapeport_dyn_menu[TAPEPORT_PORT_2][i].callback = radio_TapePort2Device_callback;
        tapeport_dyn_menu[TAPEPORT_PORT_2][i].data     = (ui_callback_data_t)vice_int_to_ptr(devices[i].id);
    }
    tapeport_dyn_menu[TAPEPORT_PORT_2][i].string   = NULL;

    lib_free(devices);

    return MENU_SUBMENU_STRING;
}

static UI_MENU_CALLBACK(attach_tape1_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select tape image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (tape_image_attach(TAPEPORT_PORT_1 + 1, name) < 0) {
                ui_error("Cannot attach tape image.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_TAPE_ATTACH_1);
    }
    return NULL;
}

static UI_MENU_CALLBACK(attach_tape2_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select tape image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (tape_image_attach(TAPEPORT_PORT_2 + 1, name) < 0) {
                ui_error("Cannot attach tape image.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_TAPE_ATTACH_2);
    }
    return NULL;
}

static UI_MENU_CALLBACK(create_tape_image_callback)
{
    char *name = NULL;
    int overwrite = 1;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select tape image name", FILEREQ_MODE_SAVE_FILE);
        if (name != NULL) {
            if (util_file_exists(name)) {
                if (message_box("VICE QUESTION", "File exists, do you want to overwrite?", MESSAGE_YESNO) != 1) {
                    overwrite = 0;
                }
            }
            if (overwrite == 1) {
                if (cbmimage_create_image(name, DISK_IMAGE_TYPE_TAP)) {
                    ui_error("Cannot create tape image");
                }
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_TAPE_CREATE_1);
    }
    return NULL;
}

UI_MENU_DEFINE_INT(DatasetteSpeedTuning)
UI_MENU_DEFINE_INT(DatasetteZeroGapDelay)
UI_MENU_DEFINE_TOGGLE(DatasetteResetWithCPU)
UI_MENU_DEFINE_INT(DatasetteTapeWobbleFrequency)
UI_MENU_DEFINE_INT(DatasetteTapeWobbleAmplitude)
UI_MENU_DEFINE_INT(DatasetteTapeAzimuthError)
UI_MENU_DEFINE_TOGGLE(DatasetteSound)
UI_MENU_DEFINE_TOGGLE(TrapDevice1)

const ui_menu_entry_t tape_pet_menu[] = {
    {   .action   = ACTION_TAPE_ATTACH_1,
        .string   = "Attach tape image to Datasette 1",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_tape1_callback
    },
    {   .action   = ACTION_TAPE_ATTACH_2,
        .string   = "Attach tape image to Datasette 2",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_tape2_callback
    },
    {   .action   = ACTION_TAPE_DETACH_1,
        .string   = "Detach tape image from Datasette 1",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_DETACH_2,
        .string   = "Detach tape image from Datasette 2",
        .type     = MENU_ENTRY_OTHER
    },
    /* unlike the Gtk3 UI, the SDL UI appears to only have a single "Create tape"
     * menu item/action */
    {   .action   = ACTION_TAPE_CREATE_1,
        .string   = "Create new tape image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = create_tape_image_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Datasette 1 control"),
    {   .action   = ACTION_TAPE_STOP_1,
        .string   = "Stop",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_PLAY_1,
        .string   = "Play",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_FFWD_1,
        .string   = "Forward",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_REWIND_1,
        .string   = "Rewind",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_RECORD_1,
        .string   = "Record",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_RESET_1,
        .string   = "Reset",
        .type     = MENU_ENTRY_OTHER
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Datasette 2 control"),
    {   .action   = ACTION_TAPE_STOP_2,
        .string   = "Stop",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_PLAY_2,
        .string   = "Play",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_FFWD_2,
        .string   = "Forward",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_REWIND_2,
        .string   = "Rewind",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_RECORD_2,
        .string   = "Record",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_RESET_2,
        .string   = "Reset",
        .type     = MENU_ENTRY_OTHER
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Datasette speed tuning",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteSpeedTuning_callback,
        .data     = (ui_callback_data_t)"Set datasette speed tuning"
    },
    {   .string   = "Datasette zero gap delay",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteZeroGapDelay_callback,
        .data     = (ui_callback_data_t)"Set datasette zero gap delay"
    },
    {   .string   = "Datasette tape wobble frequency",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteTapeWobbleFrequency_callback,
        .data     = (ui_callback_data_t)"Set datasette tape wobble frequency"
    },
    {   .string   = "Datasette tape wobble amplitude",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteTapeWobbleAmplitude_callback,
        .data     = (ui_callback_data_t)"Set datasette tape wobble amplitude"
    },
    {   .string   = "Datasette tape alignment",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteTapeAzimuthError_callback,
        .data     = (ui_callback_data_t)"Set datasette alignment error"
    },
    {   .string   = "Reset Datasette on CPU Reset",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DatasetteResetWithCPU_callback
    },
    {   .string   = "Enable Datasette sound",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DatasetteSound_callback
    },
    {   .string   = "Enable virtual device (for t64)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TrapDevice1_callback
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t tape_menu_no_tapeport[] = {
    SDL_MENU_ITEM_TITLE("No tapeport on current model"),
    SDL_MENU_LIST_END
};

const ui_menu_entry_t tape_menu_template[] = {
    {   .action   = ACTION_TAPE_ATTACH_1,
        .string   = "Attach tape image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_tape1_callback
    },
    {   .action   = ACTION_TAPE_DETACH_1,
        .string   = "Detach tape image",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_CREATE_1,
        .string   = "Create new tape image",
        .type     = MENU_ENTRY_DIALOG,
        .callback = create_tape_image_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Datasette control"),
    {   .action   = ACTION_TAPE_STOP_1,
        .string   = "Stop",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_PLAY_1,
        .string   = "Play",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_FFWD_1,
        .string   = "Forward",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_REWIND_1,
        .string   = "Rewind",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_RECORD_1,
        .string   = "Record",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_TAPE_RESET_1,
        .string   = "Reset",
        .type     = MENU_ENTRY_OTHER
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Datasette speed tuning",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteSpeedTuning_callback,
        .data     = (ui_callback_data_t)"Set datasette speed tuning"
    },
    {   .string   = "Datasette zero gap delay",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteZeroGapDelay_callback,
        .data     = (ui_callback_data_t)"Set datasette zero gap delay"
    },
    {   .string   = "Datasette tape wobble frequency",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteTapeWobbleFrequency_callback,
        .data     = (ui_callback_data_t)"Set datasette tape wobble frequency"
    },
    {   .string   = "Datasette tape wobble amplitude",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteTapeWobbleAmplitude_callback,
        .data     = (ui_callback_data_t)"Set datasette tape wobble amplitude"
    },
    {   .string   = "Datasette tape alignment",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_DatasetteTapeAzimuthError_callback,
        .data     = (ui_callback_data_t)"Set datasette alignment error"
    },
    {   .string   = "Reset Datasette on CPU Reset",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DatasetteResetWithCPU_callback,
    },
    {   .string   = "Enable Datasette sound",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DatasetteSound_callback
    },
    {   .string   = "Enable virtual device (for t64)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TrapDevice1_callback
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t tape_menu[sizeof(tape_menu_template) / sizeof(ui_menu_entry_t)];

UI_MENU_DEFINE_TOGGLE(CPClockF83Save)

const ui_menu_entry_t cpclockf83_device_menu[] = {
    {   .string   = "Save CP Clock F83 RTC data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CPClockF83Save_callback
    },
    SDL_MENU_LIST_END
};


/*
 * tapecart support (see https://github.com/ikorb/tapecart/)
 */
UI_MENU_DEFINE_TOGGLE(TapecartUpdateTCRT)
UI_MENU_DEFINE_TOGGLE(TapecartOptimizeTCRT)
UI_MENU_DEFINE_INT(TapecartLoglevel)
UI_MENU_DEFINE_FILE_STRING(TapecartTCRTFilename)


/** \brief  Flush tapecart image to disk
 */
static UI_MENU_CALLBACK(tapecart_flush_callback)
{
    if (activated) {
        if (tapecart_flush_tcrt() != 0) {
            /* report error */
            ui_error("Failed to flush tapecart image");
        } else {
            ui_message("Flushed tapecart image to disk");
        }
    }
    return NULL;
}

const ui_menu_entry_t tapecart_submenu[] = {
    {   .string   = "Save tapecart data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TapecartUpdateTCRT_callback
    },
    {   .string   = "Optimize tapecart data when changed",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TapecartOptimizeTCRT_callback
    },
    {   .string   = "tapecart Log level",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_TapecartLoglevel_callback,
        .data     = (ui_callback_data_t)"Set tapecart log level"
    },
    {   .string   = "TCRT filename",
        .type     = MENU_ENTRY_DIALOG,
        .callback = file_string_TapecartTCRTFilename_callback,
        .data     = (ui_callback_data_t)"Select TCRT file"
    },
    {   .string   = "Flush current image",
        .type     = MENU_ENTRY_OTHER,
        .callback = tapecart_flush_callback
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t tapeport_devices_menu[] = {
    {   .string   = "Tapeport devices",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = TapePort1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)tapeport_dyn_menu[TAPEPORT_PORT_1]
    },
    {   .string   = "CP Clock F83 device settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)cpclockf83_device_menu
    },
    {   .string   = "tapecart device settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)tapecart_submenu
    },
    SDL_MENU_LIST_END
};

const ui_menu_entry_t tapeport_pet_devices_menu[] = {
    {   .string   = "Tapeport 1 devices",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = TapePort1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)tapeport_dyn_menu[TAPEPORT_PORT_1]
    },
    {   .string   = "Tapeport 2 devices",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = TapePort2Device_dynmenu_callback,
        .data     = (ui_callback_data_t)tapeport_dyn_menu[TAPEPORT_PORT_2]
    },
    {   .string   = "CP Clock F83 device settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)cpclockf83_device_menu
    },
    SDL_MENU_LIST_END
};

void uitapeport_menu_shutdown(void)
{
    int i;

    for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
        if (tapeport_dyn_menu_init[i]) {
            sdl_menu_tapeport_free(i);
        }
    }
}

void uitape_menu_create(int has_tapeport)
{
    int i;

    if (!has_tapeport) {
        /* copy over 'no tapeport' menu if there is no tapeport */
        tape_menu[0] = tape_menu_no_tapeport[0];
        tape_menu[1] = tape_menu_no_tapeport[1];
    } else {
        for (i = 0; tape_menu_template[i].string != NULL; i++) {
            tape_menu[i] = tape_menu_template[i];
        }
        /* terminate list */
        tape_menu[i].string = NULL;
    }
}
