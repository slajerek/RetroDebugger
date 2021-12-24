/*
 * menu_tape.c - Tape menu for SDL UI.
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
#include "types.h"

#include "attach.h"
#include "cbmimage.h"
#include "datasette.h"
#include "diskimage.h"
#include "lib.h"
#include "menu_common.h"
#include "tape.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "uimsgbox.h"
#include "util.h"

static UI_MENU_CALLBACK(attach_tape_callback)
{
    char *name;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select tape image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (tape_image_attach(1, name) < 0) {
                ui_error("Cannot attach tape image.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(detach_tape_callback)
{
    if (activated) {
        tape_image_detach(1);
    }
    return NULL;
}

static UI_MENU_CALLBACK(custom_datasette_control_callback)
{
    if (activated) {
        datasette_control(vice_ptr_to_int(param));
    }
    return NULL;
}

UI_MENU_CALLBACK(create_tape_image_callback)
{
    char *name = NULL;
    int overwrite = 1;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select tape image name", FILEREQ_MODE_SAVE_FILE);
        if (name != NULL) {
            if (util_file_exists(name)) {
                if (message_box("VICE QUESTION", "File exists, do you want to overwrite?", MESSAGE_YESNO) == 1) {
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
    }
    return NULL;
}

UI_MENU_DEFINE_INT(DatasetteSpeedTuning)
UI_MENU_DEFINE_INT(DatasetteZeroGapDelay)
UI_MENU_DEFINE_TOGGLE(DatasetteResetWithCPU)

const ui_menu_entry_t tape_menu[] = {
    { "Attach tape image",
      MENU_ENTRY_DIALOG,
      attach_tape_callback,
      NULL },
    { "Detach tape image",
      MENU_ENTRY_OTHER,
      detach_tape_callback,
      NULL },
    { "Create new tape image",
      MENU_ENTRY_DIALOG,
      create_tape_image_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Datasette control"),
    { "Stop",
      MENU_ENTRY_OTHER,
      custom_datasette_control_callback,
      (ui_callback_data_t)DATASETTE_CONTROL_STOP },
    { "Play",
      MENU_ENTRY_OTHER,
      custom_datasette_control_callback,
      (ui_callback_data_t)DATASETTE_CONTROL_START },
    { "Forward",
      MENU_ENTRY_OTHER,
      custom_datasette_control_callback,
      (ui_callback_data_t)DATASETTE_CONTROL_FORWARD },
    { "Rewind",
      MENU_ENTRY_OTHER,
      custom_datasette_control_callback,
      (ui_callback_data_t)DATASETTE_CONTROL_REWIND },
    { "Record",
      MENU_ENTRY_OTHER,
      custom_datasette_control_callback,
      (ui_callback_data_t)DATASETTE_CONTROL_RECORD },
    { "Reset",
      MENU_ENTRY_OTHER,
      custom_datasette_control_callback,
      (ui_callback_data_t)DATASETTE_CONTROL_RESET },
    SDL_MENU_ITEM_SEPARATOR,
    { "Datasette speed tuning",
      MENU_ENTRY_RESOURCE_INT,
      int_DatasetteSpeedTuning_callback,
      (ui_callback_data_t)"Set datasette speed tuning" },
    { "Datasette zero gap delay",
      MENU_ENTRY_RESOURCE_INT,
      int_DatasetteZeroGapDelay_callback,
      (ui_callback_data_t)"Set datasette zero gap delay" },
    { "Reset Datasette on CPU Reset",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_DatasetteResetWithCPU_callback,
      NULL },
    SDL_MENU_LIST_END
};
