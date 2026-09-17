/*
 * menu_drive.c - Implementation of the common drive settings menu for the SDL UI.
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

#include "attach.h"
#include "autostart.h"
#include "autostart-prg.h"
#include "charset.h"
#include "drive.h"
#include "drive-check.h"
#include "diskimage.h"
#include "fliplist.h"
#include "iecdrive.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_drive.h"
#ifdef HAVE_REALDEVICE
#include "opencbmlib.h"
#endif
#include "resources.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "uimsgbox.h"
#include "util.h"
#include "vdrive-internal.h"

/* only LOAD and SAVE are valid, rest are handled by UI action handlers */
enum {
    UI_FLIP_ADD,
    UI_FLIP_REMOVE,
    UI_FLIP_NEXT,
    UI_FLIP_PREVIOUS,
    UI_FLIP_LOAD,
    UI_FLIP_SAVE
};

static int had_driveport;

static int check_memory_expansion(int memory, int type)
{
    switch (memory) {
        case 0x2000:
            return drive_check_expansion2000(type);
            break;
        case 0x4000:
            return drive_check_expansion4000(type);
            break;
        case 0x6000:
            return drive_check_expansion6000(type);
            break;
        case 0x8000:
            return drive_check_expansion8000(type);
            break;
        default:
            return drive_check_expansionA000(type);
    }
    return 0;
}

static int is_fs(int type)
{
    return (type == ATTACH_DEVICE_FS || type == ATTACH_DEVICE_REAL);
}

static int check_current_drive_type(int type, int drive)
{
    int tde = 0;
    int vdt = 0;
    int fsdevice = 0;
    int drivetype;

    resources_get_int_sprintf("Drive%iTrueEmulation", &tde, drive);
    resources_get_int_sprintf("TrapDevice%i", &vdt, drive);
    resources_get_int_sprintf("Drive%iType", &drivetype, drive);
    resources_get_int_sprintf("FileSystemDevice%i", &fsdevice, drive);
    if (!tde && vdt) {
        if (type == fsdevice) {
            return 1;
        }
    } else {
        if (type == drivetype) {
            return 1;
        }
    }
    return 0;
}

static char *get_drive_type_string(int drive)
{
    int type;
    int tde = 0;
    int bus = 0;
    int trap = 0;
    int fsdevice = 0;

    type = drive_get_type_by_devnr(drive);
    resources_get_int_sprintf("Drive%iTrueEmulation", &tde, drive);
    resources_get_int_sprintf("BusDevice%i", &bus, drive);
    resources_get_int_sprintf("TrapDevice%i", &trap, drive);
    resources_get_int_sprintf("FileSystemDevice%i", &fsdevice, drive);

    if (!tde && (trap || bus) && fsdevice == ATTACH_DEVICE_FS) {
        return MENU_SUBMENU_STRING " directory";
    }

#ifdef HAVE_REALDEVICE
    if (!tde && (trap || bus) && fsdevice == ATTACH_DEVICE_REAL) {
        return MENU_SUBMENU_STRING " real drive";
    }
#endif

    switch (type) {
        case 0:                  return MENU_SUBMENU_STRING " none";
        case DRIVE_TYPE_1540:    return MENU_SUBMENU_STRING " 1540";
        case DRIVE_TYPE_1541:    return MENU_SUBMENU_STRING " 1541";
        case DRIVE_TYPE_1541II:  return MENU_SUBMENU_STRING " 1541-II";
        case DRIVE_TYPE_1551:    return MENU_SUBMENU_STRING " 1551";
        case DRIVE_TYPE_1570:    return MENU_SUBMENU_STRING " 1570";
        case DRIVE_TYPE_1571:    return MENU_SUBMENU_STRING " 1571";
        case DRIVE_TYPE_1571CR:  return MENU_SUBMENU_STRING " 1571CR";
        case DRIVE_TYPE_1581:    return MENU_SUBMENU_STRING " 1581";
        case DRIVE_TYPE_2000:    return MENU_SUBMENU_STRING " 2000";
        case DRIVE_TYPE_4000:    return MENU_SUBMENU_STRING " 4000";
        case DRIVE_TYPE_CMDHD:   return MENU_SUBMENU_STRING " CMDHD";
        case DRIVE_TYPE_2031:    return MENU_SUBMENU_STRING " 2031";
        case DRIVE_TYPE_2040:    return MENU_SUBMENU_STRING " 2040";
        case DRIVE_TYPE_3040:    return MENU_SUBMENU_STRING " 3040";
        case DRIVE_TYPE_4040:    return MENU_SUBMENU_STRING " 4040";
        case DRIVE_TYPE_1001:    return MENU_SUBMENU_STRING " 1001";
        case DRIVE_TYPE_9000:    return MENU_SUBMENU_STRING " D9090/60";
        case DRIVE_TYPE_8050:    return MENU_SUBMENU_STRING " 8050";
        case DRIVE_TYPE_8250:    return MENU_SUBMENU_STRING " 8250";
        default:                 return MENU_SUBMENU_STRING " ????";
    }
}

UI_MENU_DEFINE_TOGGLE(DriveSoundEmulation)
UI_MENU_DEFINE_TOGGLE(FSDeviceLongNames)
UI_MENU_DEFINE_TOGGLE(FSDeviceOverwrite)

static UI_MENU_CALLBACK(set_hide_p00_files_callback)
{
    int drive;
    int hide_p00;

    drive = vice_ptr_to_int(param);
    if (activated) {
        resources_get_int_sprintf("FSDevice%iHideCBMFiles", &hide_p00, drive);
        resources_set_int_sprintf("FSDevice%iHideCBMFiles", !hide_p00, drive);
    } else {
        resources_get_int_sprintf("FSDevice%iHideCBMFiles", &hide_p00, drive);
        if (hide_p00) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(set_write_p00_files_callback)
{
    int drive;
    int write_p00;

    drive = vice_ptr_to_int(param);
    if (activated) {
        resources_get_int_sprintf("FSDevice%iSaveP00", &write_p00, drive);
        resources_set_int_sprintf("FSDevice%iSaveP00", !write_p00, drive);
    } else {
        resources_get_int_sprintf("FSDevice%iSaveP00", &write_p00, drive);
        if (write_p00) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(set_read_p00_files_callback)
{
    int drive;
    int read_p00;

    drive = vice_ptr_to_int(param);
    if (activated) {
        resources_get_int_sprintf("FSDevice%iConvertP00", &read_p00, drive);
        resources_set_int_sprintf("FSDevice%iConvertP00", !read_p00, drive);
    } else {
        resources_get_int_sprintf("FSDevice%iConvertP00", &read_p00, drive);
        if (read_p00) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(set_directory_callback)
{
    char *name;
    int drive;

    drive = vice_ptr_to_int(param);
    if (activated) {
        if (check_current_drive_type(ATTACH_DEVICE_FS, drive)) {
            name = sdl_ui_file_selection_dialog("Select directory", FILEREQ_MODE_CHOOSE_DIR);
            if (name != NULL) {
                resources_set_string_sprintf("FSDevice%iDir", name, drive);
                lib_free(name);
            }
        }
    } else {
        if (!check_current_drive_type(ATTACH_DEVICE_FS, drive)) {
            return MENU_NOT_AVAILABLE_STRING;
        }
    }
    return NULL;
}

/* We need to keep this around for now since it's the "dialog" implementation.
 * When the hotkeys and joystick also use UI actions we can refactor this into
 * a public function without the `activated` check.
 */
static UI_MENU_CALLBACK(attach_disk_callback)
{
    if (activated)  {
        int   unit  = UNIT_FROM_PTR(param);
        int   drive = DRIVE_FROM_PTR(param);
        char *name  = sdl_ui_file_selection_dialog("Select disk image",
                                                   FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
           if (file_system_attach_disk(unit, drive, name) < 0) {
                ui_error("Cannot attach disk image.");
            }
            lib_free(name);
        }
        /* mark dialog finished so other dialogs can be activated again */
        ui_action_finish(ui_action_id_drive_attach(unit, drive));
    }
    return NULL;
}

#if 0
static UI_MENU_CALLBACK(detach_disk_callback)
{
    if (activated) {
        int unit  = UNIT_FROM_PTR(param);
        int drive = DRIVE_FROM_PTR(param);

        if (unit == 0) {
            /* detach all disks in all drives */
            for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {
                file_system_detach_disk(unit, 0);
                file_system_detach_disk(unit, 1);
            }
        } else {
            file_system_detach_disk(unit, drive);
        }
    }
    return NULL;
}
#endif

/* Only the the UI_FLIP_LOAD/SAVE cases are handled by this function, the rest
 * are handled directly by the UI action handlers.
 */
static UI_MENU_CALLBACK(fliplist_callback)
{
    char *name;

    if (activated) {
        switch (vice_ptr_to_int(param)) {
            case UI_FLIP_LOAD:
                name = sdl_ui_file_selection_dialog("Select fliplist to load", FILEREQ_MODE_CHOOSE_FILE);
                if (name != NULL) {
                    if (fliplist_load_list((unsigned int)-1, name, 0) != 0) {
                        ui_error("Cannot load fliplist.");
                    }
                    lib_free(name);
                }
                ui_action_finish(ACTION_FLIPLIST_LOAD_8_0);
                break;
            case UI_FLIP_SAVE:  /* weird fallthrough? */
                name = sdl_ui_file_selection_dialog("Select fliplist to save", FILEREQ_MODE_SAVE_FILE);
                if (name != NULL) {
                    util_add_extension(&name, "vfl");
                    if (fliplist_save_list((unsigned int)-1, name) != 0) {
                        ui_error("Cannot save fliplist.");
                    }
                    lib_free(name);
                }
                ui_action_finish(ACTION_FLIPLIST_SAVE_8_0);
                break;
            default:
                log_warning(LOG_DEFAULT, "%s(): shouldn't get here!", __func__);
        }
    }
    return NULL;
}

#define DRIVE_SHOW_IDLE_CALLBACK(x)                         \
    static UI_MENU_CALLBACK(drive_##x##_show_idle_callback) \
    {                                                       \
        int type;                                           \
                                                            \
        type = drive_get_type_by_devnr(x);                  \
                                                            \
        if (drive_check_idle_method(type)) {                \
            return MENU_SUBMENU_STRING;                     \
        }                                                   \
        return MENU_NOT_AVAILABLE_STRING;                   \
    }

DRIVE_SHOW_IDLE_CALLBACK(8)
DRIVE_SHOW_IDLE_CALLBACK(9)
DRIVE_SHOW_IDLE_CALLBACK(10)
DRIVE_SHOW_IDLE_CALLBACK(11)

#define DRIVE_SHOW_EXTEND_CALLBACK(x)                         \
    static UI_MENU_CALLBACK(drive_##x##_show_extend_callback) \
    {                                                         \
        int type;                                             \
                                                              \
        type = drive_get_type_by_devnr(x);                    \
                                                              \
        if (drive_check_extend_policy(type)) {                \
            return MENU_SUBMENU_STRING;                       \
        }                                                     \
        return MENU_NOT_AVAILABLE_STRING;                     \
    }

DRIVE_SHOW_EXTEND_CALLBACK(8)
DRIVE_SHOW_EXTEND_CALLBACK(9)
DRIVE_SHOW_EXTEND_CALLBACK(10)
DRIVE_SHOW_EXTEND_CALLBACK(11)

#define DRIVE_SHOW_EXPAND_CALLBACK(x)                         \
    static UI_MENU_CALLBACK(drive_##x##_show_expand_callback) \
    {                                                         \
        int type;                                             \
                                                              \
        type = drive_get_type_by_devnr(x);                    \
                                                              \
        if (drive_check_expansion(type)) {                    \
            return MENU_SUBMENU_STRING;                       \
        }                                                     \
        return MENU_NOT_AVAILABLE_STRING;                     \
    }

DRIVE_SHOW_EXPAND_CALLBACK(8)
DRIVE_SHOW_EXPAND_CALLBACK(9)
DRIVE_SHOW_EXPAND_CALLBACK(10)
DRIVE_SHOW_EXPAND_CALLBACK(11)

#define DRIVE_SHOW_EXBOARD_CALLBACK(x)                                  \
    static UI_MENU_CALLBACK(drive_##x##_show_exboard_callback)          \
    {                                                                   \
        int type;                                                       \
                                                                        \
        type = drive_get_type_by_devnr(x);                              \
                                                                        \
        if (drive_check_profdos(type) || drive_check_supercard(type)) { \
            return MENU_SUBMENU_STRING;                                 \
        }                                                               \
        return MENU_NOT_AVAILABLE_STRING;                               \
    }

DRIVE_SHOW_EXBOARD_CALLBACK(8)
DRIVE_SHOW_EXBOARD_CALLBACK(9)
DRIVE_SHOW_EXBOARD_CALLBACK(10)
DRIVE_SHOW_EXBOARD_CALLBACK(11)

#define DRIVE_SHOW_TYPE_CALLBACK(x)                         \
    static UI_MENU_CALLBACK(drive_##x##_show_type_callback) \
    {                                                       \
        return get_drive_type_string(x);                    \
    }

DRIVE_SHOW_TYPE_CALLBACK(8)
DRIVE_SHOW_TYPE_CALLBACK(9)
DRIVE_SHOW_TYPE_CALLBACK(10)
DRIVE_SHOW_TYPE_CALLBACK(11)

static UI_MENU_CALLBACK(set_idle_callback)
{
    int drive;
    int parameter;
    int current;
    int idle = 0;

    drive = (int)(vice_ptr_to_int(param) >> 8);
    parameter = (int)(vice_ptr_to_int(param) & 0xff);
    current = drive_get_type_by_devnr(drive);

    if (activated) {
        if (drive_check_idle_method(current)) {
            resources_set_int_sprintf("Drive%iIdleMethod", parameter, drive);
        }
    } else {
        if (!drive_check_idle_method(current)) {
            return MENU_NOT_AVAILABLE_STRING;
        } else {
            resources_get_int_sprintf("Drive%iIdleMethod", &idle, drive);
            if (idle == parameter) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(set_extend_callback)
{
    int drive;
    int parameter;
    int current;
    int extend = 0;

    drive = (int)(vice_ptr_to_int(param) >> 8);
    parameter = (int)(vice_ptr_to_int(param) & 0xff);
    current = drive_get_type_by_devnr(drive);

    if (activated) {
        if (drive_check_extend_policy(current)) {
            resources_set_int_sprintf("Drive%iExtendImagePolicy", parameter, drive);
        }
    } else {
        if (!drive_check_extend_policy(current)) {
            return MENU_NOT_AVAILABLE_STRING;
        } else {
            resources_get_int_sprintf("Drive%iExtendImagePolicy", &extend, drive);
            if (extend == parameter) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}

#define DRIVE_SHOW_PARALLEL_CALLBACK(x)                         \
    static UI_MENU_CALLBACK(drive_##x##_show_parallel_callback) \
    {                                                           \
        int type;                                               \
                                                                \
        type = drive_get_type_by_devnr(x);                      \
                                                                \
        if (drive_check_parallel_cable(type)) {                 \
            return MENU_SUBMENU_STRING;                         \
        }                                                       \
        return MENU_NOT_AVAILABLE_STRING;                       \
    }

DRIVE_SHOW_PARALLEL_CALLBACK(8)
DRIVE_SHOW_PARALLEL_CALLBACK(9)
DRIVE_SHOW_PARALLEL_CALLBACK(10)
DRIVE_SHOW_PARALLEL_CALLBACK(11)

static UI_MENU_CALLBACK(set_par_callback)
{
    int drive, type;
    int current;
    int par;

    drive = vice_ptr_to_int(param) >> 16;
    type = vice_ptr_to_int(param) & 0x0f;
    current = drive_get_type_by_devnr(drive);

    if (activated) {
        if (machine_class != VICE_MACHINE_VIC20 && machine_class != VICE_MACHINE_C64DTV && drive_check_parallel_cable(current)) {
            resources_set_int_sprintf("Drive%iParallelCable", type, drive);
        }
    } else {
        if (machine_class == VICE_MACHINE_VIC20 || machine_class == VICE_MACHINE_C64DTV || !drive_check_parallel_cable(current)) {
            return MENU_NOT_AVAILABLE_STRING;
        } else {
            resources_get_int_sprintf("Drive%iParallelCable", &par, drive);
            if (par == type) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}

#define DRIVE_PARALLEL_MENU(x)                                                  \
    static ui_menu_entry_t drive_##x##_parallel_menu[] = {                      \
        {   .string   = "None",                                                 \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_par_callback,                                       \
            .data     = (ui_callback_data_t)(DRIVE_PC_NONE + (x << 16))         \
        },                                                                      \
        {   .string   = "Standard",                                             \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_par_callback,                                       \
            .data     = (ui_callback_data_t)(DRIVE_PC_STANDARD + (x << 16))     \
        },                                                                      \
        {   .string   = "Dolphin DOS",                                          \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_par_callback,                                       \
            .data     = (ui_callback_data_t)(DRIVE_PC_DD3 + (x << 16))          \
        },                                                                      \
        {   .string   = "Formel 64",                                            \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_par_callback,                                       \
            .data     = (ui_callback_data_t)(DRIVE_PC_FORMEL64 + (x << 16))     \
        },                                                                      \
        {   .string   = "21sec backup",                                         \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_par_callback,                                       \
            .data     = (ui_callback_data_t)(DRIVE_PC_21SEC_BACKUP + (x << 16)) \
        },                                                                      \
        SDL_MENU_LIST_END                                                       \
    };

DRIVE_PARALLEL_MENU(8)
DRIVE_PARALLEL_MENU(9)
DRIVE_PARALLEL_MENU(10)
DRIVE_PARALLEL_MENU(11)

UI_MENU_DEFINE_SLIDER(Drive8RPM, DRIVE_RPM_MIN, DRIVE_RPM_MAX)
UI_MENU_DEFINE_SLIDER(Drive9RPM, DRIVE_RPM_MIN, DRIVE_RPM_MAX)
UI_MENU_DEFINE_SLIDER(Drive10RPM, DRIVE_RPM_MIN, DRIVE_RPM_MAX)
UI_MENU_DEFINE_SLIDER(Drive11RPM, DRIVE_RPM_MIN, DRIVE_RPM_MAX)

UI_MENU_DEFINE_SLIDER(Drive8WobbleAmplitude, 0, DRIVE_WOBBLE_AMPLITUDE_MAX)
UI_MENU_DEFINE_SLIDER(Drive9WobbleAmplitude, 0, DRIVE_WOBBLE_AMPLITUDE_MAX)
UI_MENU_DEFINE_SLIDER(Drive10WobbleAmplitude, 0, DRIVE_WOBBLE_AMPLITUDE_MAX)
UI_MENU_DEFINE_SLIDER(Drive11WobbleAmplitude, 0, DRIVE_WOBBLE_AMPLITUDE_MAX)

UI_MENU_DEFINE_SLIDER(Drive8WobbleFrequency, 0, DRIVE_WOBBLE_FREQ_MAX)
UI_MENU_DEFINE_SLIDER(Drive9WobbleFrequency, 0, DRIVE_WOBBLE_FREQ_MAX)
UI_MENU_DEFINE_SLIDER(Drive10WobbleFrequency, 0, DRIVE_WOBBLE_FREQ_MAX)
UI_MENU_DEFINE_SLIDER(Drive11WobbleFrequency, 0, DRIVE_WOBBLE_FREQ_MAX)

extern ui_menu_entry_t reset_menu[];

static UI_MENU_CALLBACK(set_expand_callback)
{
    int drive;
    int parameter;
    int current;
    int memory;

    drive = (int)(vice_ptr_to_int(param) >> 16);
    parameter = (int)(vice_ptr_to_int(param) & 0xffff);

    current = drive_get_type_by_devnr(drive);

    if (activated) {
        if (drive_check_expansion(current) && check_memory_expansion(parameter, current)) {
            resources_get_int_sprintf("Drive%iRAM%X", &memory, drive, (unsigned int)parameter);
            resources_set_int_sprintf("Drive%iRAM%X", !memory, drive, (unsigned int)parameter);
        }
    } else {
        if (!drive_check_extend_policy(current) || !check_memory_expansion(parameter, current)) {
            return MENU_NOT_AVAILABLE_STRING;
        } else {
            resources_get_int_sprintf("Drive%iRAM%X", &memory, drive, (unsigned int)parameter);
            if (memory) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}

#define MENUIDX_PROFDOS     0
#define MENUIDX_STARDOS     1
#define MENUIDX_SUPERCARD   2

/* called with param = (drive << 16) | (menuidx) */
static UI_MENU_CALLBACK(set_exboard_callback)
{
    int drive;
    int parameter;
    int type;
    int memory;
    int available = 0;

    drive = (int)(vice_ptr_to_int(param) >> 16);
    parameter = (int)(vice_ptr_to_int(param) & 0xffff);

    type = drive_get_type_by_devnr(drive);

    switch (parameter) {
        case MENUIDX_PROFDOS:
            available = drive_check_profdos(type);
            resources_get_int_sprintf("Drive%iProfDOS", &memory, drive);
            break;
        case MENUIDX_STARDOS:
            available = drive_check_supercard(type);
            resources_get_int_sprintf("Drive%iStarDOS", &memory, drive);
            break;
        case MENUIDX_SUPERCARD:
            available = drive_check_supercard(type);
            resources_get_int_sprintf("Drive%iSuperCard", &memory, drive);
            break;
    }

    if (activated) {
        if (available) {
            switch (parameter) {
                case MENUIDX_PROFDOS:
                    resources_set_int_sprintf("Drive%iProfDOS", !memory, drive);
                    break;
                case MENUIDX_STARDOS:
                    resources_set_int_sprintf("Drive%iStarDOS", !memory, drive);
                    break;
                case MENUIDX_SUPERCARD:
                    resources_set_int_sprintf("Drive%iSuperCard", !memory, drive);
                    break;
            }
        }
    } else {
        if (!available) {
            return MENU_NOT_AVAILABLE_STRING;
        } else {
            if (memory) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(set_drive_type_callback)
{
    int drive;
    int parameter;
    int support = 0;
    int current;

    drive = (int)(vice_ptr_to_int(param) >> 16);
    parameter = (int)(vice_ptr_to_int(param) & 0xffff);

    if (parameter == ATTACH_DEVICE_REAL) {
#ifdef HAVE_REALDEVICE
        support = opencbmlib_is_available();
#else
        support = 0;
#endif
    } else {
        support = (is_fs(parameter) || drive_check_type(parameter, drive - 8));
        if (support && !had_driveport) {
            if (drive_check_bus(parameter, IEC_BUS_IEC)) {
                support = 0;
            }
        }
    }
    current = check_current_drive_type(parameter, drive);

    if (activated) {
        if (support) {
            if (parameter == ATTACH_DEVICE_REAL) {
#ifdef HAVE_REALDEVICE
                resources_set_int_sprintf("BusDevice%i", 1, drive);
                resources_set_int_sprintf("FileSystemDevice%i", parameter, drive);
#endif
            } else if (parameter == ATTACH_DEVICE_FS) {
                resources_set_int_sprintf("TrapDevice%i", 1, drive);
                resources_set_int_sprintf("Drive%iTrueEmulation", 0, drive);
                resources_set_int_sprintf("FileSystemDevice%i", 0, drive);
                resources_set_int_sprintf("FileSystemDevice%i", parameter, drive);
            } else {
                resources_set_int_sprintf("TrapDevice%i", 0, drive);
                resources_set_int_sprintf("Drive%iTrueEmulation", 1, drive);
                resources_set_int_sprintf("Drive%iType", parameter, drive);
            }
        }
        /* update the drive menu items */
        uidrive_menu_create(had_driveport);
    } else {
        if (!support) {
            return MENU_NOT_AVAILABLE_STRING;
        } else {
            if (current) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}

static int new_disk_image_type = DISK_IMAGE_TYPE_D64;

static UI_MENU_CALLBACK(set_disk_type_callback)
{
    int disk_type;

    disk_type = (int)(vice_ptr_to_int(param));

    if (activated) {
        new_disk_image_type = disk_type;
    } else {
        if (disk_type == new_disk_image_type) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

/* XXX: Can be refactored into a dialog implementation when UI actions are
 *      supported by hotkeys/joymappings
 */
static UI_MENU_CALLBACK(create_disk_image_callback)
{
    char *name = NULL;
    char *format_name;
    int overwrite = 1;
    int result;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select diskimage name", FILEREQ_MODE_SAVE_FILE);
        if (name != NULL) {
            if (util_file_exists(name)) {
                if (message_box("VICE QUESTION", "File exists, do you want to overwrite?", MESSAGE_YESNO) != 0) {
                    overwrite = 0;
                }
            }
            if (overwrite == 1) {
                /* ask user for label,id of new disk */
                format_name = sdl_ui_text_input_dialog(
                        "Enter disk label,id:", NULL);
                if (!format_name) {
                    lib_free(name);
                    return NULL;
                }
                /* convert to PETSCII */
                charset_petconvstring((uint8_t *)format_name, CONVERT_TO_PETSCII);

                /* try to create the new image */
                if (vdrive_internal_create_format_disk_image(name, format_name,
                            new_disk_image_type) < 0) {
                    ui_error("Cannot create disk image");
                }
                result = message_box("Attach new image", "Select unit",
                        MESSAGE_UNIT_SELECT);
                /* 0-3 = unit #8 - unit #11, 4 = SKIP */
                if (result >= 0 && result <= 3) {
                    /* try to attach disk image */
                    if (file_system_attach_disk(result + 8, 0 /* FIXME: drive */, name) < 0) {
                        ui_error("Cannot attach disk image.");
                    }
                }

                lib_free(format_name);
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_DRIVE_CREATE);
    }
    return NULL;
}

static const ui_menu_entry_t create_disk_image_type_menu[] = {
    {   .string   = "D64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D64
    },
    {   .string   = "D67",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D67
    },
    {   .string   = "D71",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D71
    },
    {   .string   = "D80",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D80
    },
    {   .string   = "D81",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D81
    },
    {   .string   = "D82",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D82
    },
    {   .string   = "D90",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D90
    },
    {   .string   = "D1M",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D1M
    },
    {   .string   = "D2M",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D2M
    },
    {   .string   = "D4M",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_D4M
    },
    {   .string   = "G64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_G64
    },
    {   .string   = "G71",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_G71
    },
    {   .string   = "P64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_P64
    },
#ifdef HAVE_X64_IMAGE
    {   .string   = "X64",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = set_disk_type_callback,
        .data     = (ui_callback_data_t)DISK_IMAGE_TYPE_X64
    },
#endif
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t create_disk_image_menu[] = {
    {   .string   = "Image type",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)create_disk_image_type_menu
    },
    {   .action   = ACTION_DRIVE_CREATE,
        .string   = "Create",
        .type     =  MENU_ENTRY_DIALOG,
        .callback = create_disk_image_callback
    },
    SDL_MENU_LIST_END
};

#define DRIVE_TYPE_ITEM(text, _data)              \
    {   .string   = text,                         \
        .type     = MENU_ENTRY_OTHER_TOGGLE,      \
        .callback = set_drive_type_callback,      \
        .data     = (ui_callback_data_t)(_data) },

#ifdef HAVE_REALDEVICE
#define DRIVE_TYPE_ITEM_OPENCBM(text, data) DRIVE_TYPE_ITEM(text, data)
#else
#define DRIVE_TYPE_ITEM_OPENCBM(text, data)
#endif

#define DRIVE_TYPE_MENU(x)                                                      \
    static const ui_menu_entry_t drive_##x##_type_menu[] = {                    \
        DRIVE_TYPE_ITEM("None", x << 16)                                        \
        DRIVE_TYPE_ITEM("Directory", ATTACH_DEVICE_FS + (x << 16))              \
        DRIVE_TYPE_ITEM_OPENCBM("Real drive", ATTACH_DEVICE_REAL + (x << 16))   \
        DRIVE_TYPE_ITEM("1540", DRIVE_TYPE_1540 + (x << 16))                    \
        DRIVE_TYPE_ITEM("1541", DRIVE_TYPE_1541 + (x << 16))                    \
        DRIVE_TYPE_ITEM("1541-II", DRIVE_TYPE_1541II + (x << 16))               \
        DRIVE_TYPE_ITEM("1551", DRIVE_TYPE_1551 + (x << 16))                    \
        DRIVE_TYPE_ITEM("1570", DRIVE_TYPE_1570 + (x << 16))                    \
        DRIVE_TYPE_ITEM("1571", DRIVE_TYPE_1571 + (x << 16))                    \
        DRIVE_TYPE_ITEM("1571CR", DRIVE_TYPE_1571CR + (x << 16))                \
        DRIVE_TYPE_ITEM("1581", DRIVE_TYPE_1581 + (x << 16))                    \
        DRIVE_TYPE_ITEM("2000", DRIVE_TYPE_2000 + (x << 16))                    \
        DRIVE_TYPE_ITEM("4000", DRIVE_TYPE_4000 + (x << 16))                    \
        DRIVE_TYPE_ITEM("CMDHD", DRIVE_TYPE_CMDHD + (x << 16))                  \
        DRIVE_TYPE_ITEM("2031", DRIVE_TYPE_2031 + (x << 16))                    \
        DRIVE_TYPE_ITEM("2040", DRIVE_TYPE_2040 + (x << 16))                    \
        DRIVE_TYPE_ITEM("3040", DRIVE_TYPE_3040 + (x << 16))                    \
        DRIVE_TYPE_ITEM("4040", DRIVE_TYPE_4040 + (x << 16))                    \
        DRIVE_TYPE_ITEM("1001", DRIVE_TYPE_1001 + (x << 16))                    \
        DRIVE_TYPE_ITEM("8050", DRIVE_TYPE_8050 + (x << 16))                    \
        DRIVE_TYPE_ITEM("8250", DRIVE_TYPE_8250 + (x << 16))                    \
        DRIVE_TYPE_ITEM("D9090/60", DRIVE_TYPE_9000 + (x << 16))                \
        SDL_MENU_LIST_END                                                       \
    };

DRIVE_TYPE_MENU(8)
DRIVE_TYPE_MENU(9)
DRIVE_TYPE_MENU(10)
DRIVE_TYPE_MENU(11)

#define DRIVE_FSDIR_MENU(x)                                   \
    static const ui_menu_entry_t drive_##x##_fsdir_menu[] = { \
        {   .string   = "Choose directory",                   \
            .type     = MENU_ENTRY_DIALOG,                    \
            .callback = set_directory_callback,               \
            .data     = (ui_callback_data_t)x                 \
        },                                                    \
        {   .string   = "Read P00 files",                     \
            .type     = MENU_ENTRY_OTHER_TOGGLE,              \
            .callback = set_read_p00_files_callback,          \
            .data     = (ui_callback_data_t)x                 \
        },                                                    \
        {   .string   = "Write P00 files",                    \
            .type     = MENU_ENTRY_OTHER_TOGGLE,              \
            .callback = set_write_p00_files_callback,         \
            .data     = (ui_callback_data_t)x                 \
        },                                                    \
        {   .string   = "Hide non-P00 files",                 \
            .type     = MENU_ENTRY_OTHER_TOGGLE,              \
            .callback = set_hide_p00_files_callback,          \
            .data     = (ui_callback_data_t)x                 \
        },                                                    \
        SDL_MENU_LIST_END                                     \
    };

DRIVE_FSDIR_MENU(8)
DRIVE_FSDIR_MENU(9)
DRIVE_FSDIR_MENU(10)
DRIVE_FSDIR_MENU(11)

#define DRIVE_EXTEND_MENU(x)                                                 \
    static const ui_menu_entry_t drive_##x##_extend_menu[] = {               \
        {   .string   = "Never extend",                                      \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                             \
            .callback = set_extend_callback,                                 \
            .data     =  (ui_callback_data_t)(DRIVE_EXTEND_NEVER + (x << 8)) \
        },                                                                   \
        {   .string   = "Ask on extend",                                     \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                             \
            .callback = set_extend_callback,                                 \
            .data     = (ui_callback_data_t)(DRIVE_EXTEND_ASK + (x << 8))    \
        },                                                                   \
        {   .string   = "Extend on access",                                  \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                             \
            .callback = set_extend_callback,                                 \
            .data     = (ui_callback_data_t)(DRIVE_EXTEND_ACCESS + (x << 8)) \
        },                                                                   \
        SDL_MENU_LIST_END                                         \
    };

DRIVE_EXTEND_MENU(8)
DRIVE_EXTEND_MENU(9)
DRIVE_EXTEND_MENU(10)
DRIVE_EXTEND_MENU(11)

#define DRIVE_EXPAND_MENU(x)                                     \
    static const ui_menu_entry_t drive_##x##_expand_menu[] = {   \
        {   .string   = "RAM at $2000-$3FFF",                    \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                 \
            .callback = set_expand_callback,                     \
            .data     = (ui_callback_data_t)(0x2000 + (x << 16)) \
        },                                                       \
        {   .string   = "RAM at $4000-$5FFF",                    \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                 \
            .callback = set_expand_callback,                     \
            .data     = (ui_callback_data_t)(0x4000 + (x << 16)) \
        },                                                       \
        {   .string   = "RAM at $6000-$7FFF",                    \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                 \
            .callback = set_expand_callback,                     \
            .data     = (ui_callback_data_t)(0x6000 + (x << 16)) \
        },                                                       \
        {   .string   = "RAM at $8000-$9FFF",                    \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                 \
            .callback =  set_expand_callback,                    \
            .data     = (ui_callback_data_t)(0x8000 + (x << 16)) \
        },                                                       \
        {   .string   = "RAM at $A000-$BFFF",                    \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                 \
            .callback = set_expand_callback,                     \
            .data     = (ui_callback_data_t)(0xa000 + (x << 16)) \
        },                                                       \
        SDL_MENU_LIST_END                                        \
    };

DRIVE_EXPAND_MENU(8)
DRIVE_EXPAND_MENU(9)
DRIVE_EXPAND_MENU(10)
DRIVE_EXPAND_MENU(11)

UI_MENU_DEFINE_FILE_STRING(DriveSuperCardName)
UI_MENU_DEFINE_FILE_STRING(DriveStarDOSName)
UI_MENU_DEFINE_FILE_STRING(DriveProfDOS1571Name)

#define DRIVE_EXBOARD_MENU(x)                                                     \
    static const ui_menu_entry_t drive_##x##_exboard_menu[] = {                   \
        {   .string   = "Professional DOS 1571",                                  \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                  \
            .callback = set_exboard_callback,                                     \
            .data     = (ui_callback_data_t)(MENUIDX_PROFDOS + (x << 16))         \
        },                                                                        \
        {   .string   = "Professional DOS 1571 ROM file",                         \
            .type     = MENU_ENTRY_DIALOG,                                        \
            .callback = file_string_DriveProfDOS1571Name_callback,                \
            .data     = (ui_callback_data_t)"Set Professional DOS 1571 ROM image" \
        },                                                                        \
        {   .string   = "StarDOS",                                                \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                  \
            .callback = set_exboard_callback,                                     \
            .data     = (ui_callback_data_t)(MENUIDX_STARDOS + (x << 16))         \
        },                                                                        \
        {   .string   = "StarDOS ROM file",                                       \
            .type     = MENU_ENTRY_DIALOG,                                        \
            .callback = file_string_DriveStarDOSName_callback,                    \
            .data     = (ui_callback_data_t)"Set StarDOS ROM image"               \
        },                                                                        \
        {   .string   = "Supercard+",                                             \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                  \
            .callback = set_exboard_callback,                                     \
            .data     = (ui_callback_data_t)(MENUIDX_SUPERCARD + (x << 16))       \
        },                                                                        \
        {   .string   = "Supercard+ ROM file",                                    \
            .type     = MENU_ENTRY_DIALOG,                                        \
            .callback = file_string_DriveSuperCardName_callback,                  \
            .data     = (ui_callback_data_t)"Set Supercard+ ROM image"            \
        },                                                                        \
        SDL_MENU_LIST_END                                                         \
    };

DRIVE_EXBOARD_MENU(8)
DRIVE_EXBOARD_MENU(9)
DRIVE_EXBOARD_MENU(10)
DRIVE_EXBOARD_MENU(11)

#define DRIVE_IDLE_MENU(x)                                                      \
    static const ui_menu_entry_t drive_##x##_idle_menu[] = {                    \
        {   .string   = "None",                                                 \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_idle_callback,                                      \
            .data     = (ui_callback_data_t)(DRIVE_IDLE_NO_IDLE + (x << 8))     \
        },                                                                      \
        {   .string   = "Skip cycles",                                          \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_idle_callback,                                      \
            .data     = (ui_callback_data_t)(DRIVE_IDLE_SKIP_CYCLES + (x << 8)) \
        },                                                                      \
        {   .string   = "Trap idle",                                            \
            .type     = MENU_ENTRY_OTHER_TOGGLE,                                \
            .callback = set_idle_callback,                                      \
            .data     = (ui_callback_data_t)(DRIVE_IDLE_TRAP_IDLE + (x << 8))   \
        },                                                                      \
        SDL_MENU_LIST_END                                                       \
    };

DRIVE_IDLE_MENU(8)
DRIVE_IDLE_MENU(9)
DRIVE_IDLE_MENU(10)
DRIVE_IDLE_MENU(11)

UI_MENU_DEFINE_TOGGLE(Drive8RTCSave)
UI_MENU_DEFINE_TOGGLE(Drive9RTCSave)
UI_MENU_DEFINE_TOGGLE(Drive10RTCSave)
UI_MENU_DEFINE_TOGGLE(Drive11RTCSave)

UI_MENU_DEFINE_TOGGLE(AttachDevice8d0Readonly)
UI_MENU_DEFINE_TOGGLE(AttachDevice9d0Readonly)
UI_MENU_DEFINE_TOGGLE(AttachDevice10d0Readonly)
UI_MENU_DEFINE_TOGGLE(AttachDevice11d0Readonly)

UI_MENU_DEFINE_TOGGLE(AttachDevice8d1Readonly)
UI_MENU_DEFINE_TOGGLE(AttachDevice9d1Readonly)
UI_MENU_DEFINE_TOGGLE(AttachDevice10d1Readonly)
UI_MENU_DEFINE_TOGGLE(AttachDevice11d1Readonly)

UI_MENU_DEFINE_TOGGLE(Drive8TrueEmulation)
UI_MENU_DEFINE_TOGGLE(Drive9TrueEmulation)
UI_MENU_DEFINE_TOGGLE(Drive10TrueEmulation)
UI_MENU_DEFINE_TOGGLE(Drive11TrueEmulation)

UI_MENU_DEFINE_TOGGLE(TrapDevice8)
UI_MENU_DEFINE_TOGGLE(TrapDevice9)
UI_MENU_DEFINE_TOGGLE(TrapDevice10)
UI_MENU_DEFINE_TOGGLE(TrapDevice11)

UI_MENU_DEFINE_STRING(Drive8FixedSize)
UI_MENU_DEFINE_STRING(Drive9FixedSize)
UI_MENU_DEFINE_STRING(Drive10FixedSize)
UI_MENU_DEFINE_STRING(Drive11FixedSize)

/* CAUTION: the position of the menu items is hardcoded in uidrive_menu_create() */

#define DRIVE_SETTINGS_OFFSET_DRIVE_D1_R0  12
#define DRIVE_SETTINGS_OFFSET_DRIVE_TDE    14
#define DRIVE_SETTINGS_OFFSET_DRIVE_VDRIVE 15
#define DRIVE_SETTINGS_OFFSET_IEEE_END     15
#define DRIVE_SETTINGS_OFFSET_DRIVE_RTC    19

#define DRIVE_MENU(x)                                                           \
    static ui_menu_entry_t drive_##x##_menu[] = {                               \
/* 0  */{   .string   = "Drive " #x " type",                                    \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = drive_##x##_show_type_callback,                         \
            .data     = (ui_callback_data_t)drive_##x##_type_menu },            \
/* 1  */{   .string   = "Drive " #x " dir settings",                            \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = submenu_callback,                                       \
            .data     = (ui_callback_data_t)drive_##x##_fsdir_menu },           \
/* 2  */{   .string   = "Drive " #x " extra tracks handling",                   \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = drive_##x##_show_extend_callback,                       \
            .data     = (ui_callback_data_t)drive_##x##_extend_menu },          \
/* 3  */{   .string   = "Drive " #x " expansion memory",                        \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = drive_##x##_show_expand_callback,                       \
            .data     = (ui_callback_data_t)drive_##x##_expand_menu },          \
/* 4  */{   .string   = "Drive " #x " expansion board",                         \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = drive_##x##_show_exboard_callback,                      \
            .data     = (ui_callback_data_t)drive_##x##_exboard_menu },         \
/* 5  */{   .string   = "Drive " #x " idle method",                             \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = drive_##x##_show_idle_callback,                         \
            .data     = (ui_callback_data_t)drive_##x##_idle_menu },            \
/* 6  */{   .string   = "Drive " #x " parallel cable",                          \
            .type     = MENU_ENTRY_SUBMENU,                                     \
            .callback = drive_##x##_show_parallel_callback,                     \
            .data     = (ui_callback_data_t)drive_##x##_parallel_menu },        \
/* 7  */{   .string   = "Drive " #x " RPM*100",                                 \
            .type     = MENU_ENTRY_RESOURCE_INT,                                \
            .callback = slider_Drive##x##RPM_callback,                          \
            .data     = (ui_callback_data_t)"Set RPM (29500-30500)" },          \
/* 8  */{   .string   = "Drive " #x " wobble frequency",                        \
            .type     = MENU_ENTRY_RESOURCE_INT,                                \
            .callback = slider_Drive##x##WobbleFrequency_callback,              \
            .data     = (ui_callback_data_t)"Set Wobble frequency (0-10000)" }, \
/* 9  */{   .string   = "Drive " #x " wobble amplitude",                        \
            .type     = MENU_ENTRY_RESOURCE_INT,                                \
            .callback = slider_Drive##x##WobbleAmplitude_callback,              \
            .data     = (ui_callback_data_t)"Set Wobble (0-5000)" },            \
/* 10 */SDL_MENU_ITEM_SEPARATOR,                                                \
/* 11 */{   .string   = "Attach Drive " #x" drive 0 read only",                 \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,                             \
            .callback = toggle_AttachDevice##x##d0Readonly_callback },          \
/* 12 */{   .string   = "Attach Drive " #x" drive 1 read only",                 \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,                             \
            .callback = toggle_AttachDevice##x##d1Readonly_callback },          \
/* 13 */SDL_MENU_ITEM_SEPARATOR,                                                \
/* 14 */{   .string   = "Drive " #x" True Drive Emulation",                     \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,                             \
            .callback = toggle_Drive##x##TrueEmulation_callback },              \
/* 15 */{   .string   = "Drive " #x" Virtual (Trap) Device",                    \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,                             \
            .callback = toggle_TrapDevice##x##_callback },                      \
/* 16 */SDL_MENU_ITEM_SEPARATOR,                                                \
/* 17 */{   .string   = "CMD HD fixed size",                                    \
            .type     = MENU_ENTRY_RESOURCE_STRING,                             \
            .callback = string_Drive##x##FixedSize_callback,                    \
            .data     = (ui_callback_data_t)"Set CMD HD fixed size" },          \
/* 18 */SDL_MENU_ITEM_SEPARATOR,                                                \
/* 19 */{   .string   = "Save Drive " #x" CMD RTC data",                        \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,                             \
            .callback = toggle_Drive##x##RTCSave_callback },                    \
        SDL_MENU_LIST_END                                                       \
    };


DRIVE_MENU(8)
DRIVE_MENU(9)
DRIVE_MENU(10)
DRIVE_MENU(11)

static const ui_menu_entry_t fliplist_menu[] = {
    {   .action   = ACTION_FLIPLIST_ADD_8_0,
        .string   = "Add current image to fliplist",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_FLIPLIST_REMOVE_8_0,
        .string   = "Remove current image from fliplist",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_FLIPLIST_NEXT_8_0,
        .string   = "Attach next image in fliplist",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_FLIPLIST_PREVIOUS_8_0,
        .string   = "Attach previous image in fliplist",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_FLIPLIST_LOAD_8_0,
        .string   = "Load fliplist",
        .type     = MENU_ENTRY_DIALOG,
        .callback = fliplist_callback,
        .data     = (ui_callback_data_t)UI_FLIP_LOAD
    },
    {  .action    = ACTION_FLIPLIST_SAVE_8_0,
       .string    = "Save fliplist",
       .type      = MENU_ENTRY_DIALOG,
       .callback  = fliplist_callback,
       .data      = (ui_callback_data_t)UI_FLIP_SAVE
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(AutostartHandleTrueDriveEmulation)
UI_MENU_DEFINE_TOGGLE(AutostartWarp)
UI_MENU_DEFINE_TOGGLE(AutostartDelayRandom)
UI_MENU_DEFINE_TOGGLE(AutostartBasicLoad)
UI_MENU_DEFINE_TOGGLE(AutostartTapeBasicLoad)
UI_MENU_DEFINE_TOGGLE(AutostartRunWithColon)
UI_MENU_DEFINE_RADIO(AutostartDropMode)
UI_MENU_DEFINE_RADIO(AutostartPrgMode)
UI_MENU_DEFINE_STRING(AutostartPrgDiskImage)


static UI_MENU_CALLBACK(custom_AutostartDelay_callback)
{
    static char buf[20];
    char *value;
    int previous;
    int new_value;

    resources_get_int("AutostartDelay", &previous);

    if (activated) {
        sprintf(buf, "%d", previous);
        value = sdl_ui_text_input_dialog(
                "Autostart delay in seconds (0 = default, max = 1000)", buf);
        if (value) {
            new_value = (int)strtol(value, NULL, 10);
            if (new_value < 0) {
                new_value = 0;
            } else if (new_value > 1000) {
                new_value = 1000;
            }
            if (new_value != previous) {
                resources_set_int("AutostartDelay", new_value);
            }
            lib_free(value);
        }
    } else {
        sprintf(buf, "%d seconds", previous);
        return buf;
    }
    return NULL;
}

/* CAUTION: the position of the menu items is hardcoded in uidrive_menu_create() */
static ui_menu_entry_t autostart_settings_menu[] = {
/* 0  */
    {   .string   = "Handle TDE on autostart",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_AutostartHandleTrueDriveEmulation_callback
    },
/* 1  */
    {   .string   = "Autostart warp",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_AutostartWarp_callback
    },
/* 2  */
    {   .string   = "Autostart delay",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = custom_AutostartDelay_callback
    },
/* 3  */
    {   .string   = "Autostart random delay",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_AutostartDelayRandom_callback
    },
/* 4  */
    {   .string   = "Load to BASIC start (tape)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_AutostartTapeBasicLoad_callback
    },
/* 5  */
    {   .string   = "Load to BASIC start (disk)",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_AutostartBasicLoad_callback
    },
    {   .string   = "Use ':' with RUN",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_AutostartRunWithColon_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("PRG Autostart mode"),
    {   .string   = "VirtualFS",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_AutostartPrgMode_callback,
        .data     = (ui_callback_data_t)AUTOSTART_PRG_MODE_VFS
    },
    {   .string   = "Inject",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_AutostartPrgMode_callback,
        .data     = (ui_callback_data_t)AUTOSTART_PRG_MODE_INJECT
    },
    {   .string   = "Disk image",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_AutostartPrgMode_callback,
        .data     = (ui_callback_data_t)AUTOSTART_PRG_MODE_DISK
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autostart disk",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_AutostartPrgDiskImage_callback,
        .data     = (ui_callback_data_t)"Disk image for autostarting PRG files"
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Autostart drag'n drop mode"),
    {   .string   = "Attach image",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_AutostartDropMode_callback,
        .data     = (ui_callback_data_t)AUTOSTART_DROP_MODE_ATTACH
    },
    {   .string   = "Load",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_AutostartDropMode_callback,
        .data     = (ui_callback_data_t)AUTOSTART_DROP_MODE_LOAD
    },
    {   .string   = "Load and Run",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_AutostartDropMode_callback,
        .data     = (ui_callback_data_t)AUTOSTART_DROP_MODE_RUN
    },

    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(custom_drive_volume_callback)
{
    static char buf[20];
    int previous, new_value;

    resources_get_int("DriveSoundEmulationVolume", &previous);

    if (activated) {
        new_value = sdl_ui_slider_input_dialog("Select volume", previous, 0, 4000);
        if (new_value != previous) {
            resources_set_int("DriveSoundEmulationVolume", new_value);
        }
    } else {
        sprintf(buf, "%3i%%", (previous * 100) / 4000);
        return buf;
    }
    return NULL;
}

ui_menu_entry_t drive_menu_no_iec[] = {
    SDL_MENU_ITEM_TITLE("No IEC port on current model"),
    SDL_MENU_LIST_END
};

/* CAUTION: the position of the menu items is hardcoded in uidrive_menu_create() */
ui_menu_entry_t drive_menu_template[] = {
    /* start of hardcoded offsets in uidrive_menu_create() */
/* 0  */
    {   .action   = ACTION_DRIVE_ATTACH_8_0,
        .string   = "Attach disk image to drive 8",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(8, 0)
    },
/* 1  */
    {   .action   = ACTION_DRIVE_ATTACH_8_1,
        .string   = "Attach disk image to drive 8:1",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(8, 1),
        .status   = MENU_STATUS_NA
    },
/* 2  */
    {   .action   = ACTION_DRIVE_ATTACH_9_0,
        .string   = "Attach disk image to drive 9",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(9, 0)
    },
/* 3  */
    {   .action   = ACTION_DRIVE_ATTACH_9_1,
        .string   = "Attach disk image to drive 9:1",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(9, 1),
        .status   = MENU_STATUS_NA
    },
/* 4  */
    {   .action   = ACTION_DRIVE_ATTACH_10_0,
        .string   = "Attach disk image to drive 10",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(10, 0),
    },
/* 5 */
    {   .action   = ACTION_DRIVE_ATTACH_10_1,
        .string   = "Attach disk image to drive 10:1",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(10, 1),
        .status   = MENU_STATUS_NA
    },
/* 6  */
    {   .action   = ACTION_DRIVE_ATTACH_11_0,
        .string   = "Attach disk image to drive 11",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(11, 0),
    },
/* 7 */
    {   .action   = ACTION_DRIVE_ATTACH_11_1,
        .string   = "Attach disk image to drive 11:1",
        .type     = MENU_ENTRY_DIALOG,
        .callback = attach_disk_callback,
        .data     = UNIT_DRIVE_TO_PTR(11, 1),
        .status   = MENU_STATUS_NA
    },
/* 8 */
    {   .action   = ACTION_DRIVE_DETACH_8_0,
        .string   = "Detach disk image from drive 8",
        .type     = MENU_ENTRY_OTHER,
    },
/* 9 */
    {   .action   = ACTION_DRIVE_DETACH_8_1,
        .string   = "Detach disk image from drive 8:1",
        .type     = MENU_ENTRY_OTHER,
    },
/* 10 */
    {   .action   = ACTION_DRIVE_DETACH_9_0,
        .string   = "Detach disk image from drive 9",
        .type     = MENU_ENTRY_OTHER,
    },
/* 11 */
    {   .action   = ACTION_DRIVE_DETACH_9_1,
        .string   = "Detach disk image from drive 9:1",
        .type     = MENU_ENTRY_OTHER,
    },
/* 12 */
    {   .action   = ACTION_DRIVE_DETACH_10_0,
        .string   = "Detach disk image from drive 10",
        .type     = MENU_ENTRY_OTHER,
    },
/* 13 */
    {   .action   = ACTION_DRIVE_DETACH_10_1,
        .string   = "Detach disk image from drive 10:1",
        .type     = MENU_ENTRY_OTHER,
    },
/* 14 */
    {   .action   = ACTION_DRIVE_DETACH_11_0,
        .string   = "Detach disk image from drive 11",
        .type     = MENU_ENTRY_OTHER,
    },
/* 15 */
    {   .action   = ACTION_DRIVE_DETACH_11_1,
        .string   = "Detach disk image from drive 11:1",
        .type     = MENU_ENTRY_OTHER,
    },
    /* end of hardcoded offsets in uidrive_menu_create() */
    SDL_MENU_ITEM_SEPARATOR,

    /* the comments below indicating menu index(?) appear to be incorrect and
     * not count the menu item separators */
/* 16 */
    {   .action   = ACTION_DRIVE_DETACH_ALL,
        .string   = "Detach all disk images",
        .type     = MENU_ENTRY_OTHER,
    },
/* 17 */
    {   .string   = "Create new disk image",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)create_disk_image_menu
    },

    SDL_MENU_ITEM_SEPARATOR,

/* 18 */
    {   .string   = "Drive 8 settings",
        .type     =  MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)drive_8_menu
    },
/* 19 */
    {   .string   = "Drive 9 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)drive_9_menu
    },
/* 20 */
    {   .string   = "Drive 10 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)drive_10_menu
    },
/* 21 */
    {   .string   = "Drive 11 settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)drive_11_menu
    },

    SDL_MENU_ITEM_SEPARATOR,
/* 22 */
    {   .string   = "Drive sound emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_DriveSoundEmulation_callback
    },
/* 23 */
    {   .string   = "Drive sound Volume",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_drive_volume_callback
    },

    SDL_MENU_ITEM_SEPARATOR,
/* 24 */
    {   .string   = "FS-Device uses long names",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_FSDeviceLongNames_callback
    },
/* 25 */
    {   .string   = "FS-Device always overwrites",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_FSDeviceOverwrite_callback
    },

    SDL_MENU_ITEM_SEPARATOR,
/* 26 */
    {   .string   = "Autostart settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)autostart_settings_menu
    },
/* 27 */
    {   .string   = "Fliplist settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)fliplist_menu
    },
    SDL_MENU_LIST_END
};

ui_menu_entry_t drive_menu[sizeof(drive_menu_template) / sizeof(ui_menu_entry_t)];

/* patch some things that are slightly different in the emulators */
void uidrive_menu_create(int has_driveport)
{
    int newend = 4;
    int i, d0, d1;
    int ieee_bus = iec_available_busses() & IEC_BUS_IEEE;

    had_driveport = has_driveport;

    if ((machine_class == VICE_MACHINE_CBM5x0) ||
        (machine_class == VICE_MACHINE_CBM6x0) ||
        (machine_class == VICE_MACHINE_PET)) {
        autostart_settings_menu[4].type = MENU_ENTRY_TEXT;
        autostart_settings_menu[4].status = MENU_STATUS_INACTIVE;
        autostart_settings_menu[4].callback = seperator_callback;
    }

    if (!has_driveport && !ieee_bus) {

        /* copy over 'no iec' menu if there is no driveport */
        drive_menu[0].string = drive_menu_no_iec[0].string;
        drive_menu[0].type = drive_menu_no_iec[0].type;
        drive_menu[0].callback = drive_menu_no_iec[0].callback;
        drive_menu[0].data = drive_menu_no_iec[0].data;
        drive_menu[1].string = drive_menu_no_iec[1].string;
        drive_menu[1].type = drive_menu_no_iec[1].type;
        drive_menu[1].callback = drive_menu_no_iec[1].callback;
        drive_menu[1].data = drive_menu_no_iec[1].data;

        /* disable drive reset menu items */
        for (i = 0; i < 4; i++) {
            reset_menu[7 + (i * 2) + 0].status = MENU_STATUS_INACTIVE;
            reset_menu[7 + (i * 2) + 1].status = MENU_STATUS_INACTIVE;
        }
    } else {

        /* First copy the template to the actual menu */
        for (i = 0; drive_menu_template[i].string != NULL; i++) {
            drive_menu[i].string    = drive_menu_template[i].string;
            drive_menu[i].type      = drive_menu_template[i].type;
            drive_menu[i].callback  = drive_menu_template[i].callback;
            drive_menu[i].data      = drive_menu_template[i].data;
            drive_menu[i].action    = drive_menu_template[i].action;
            drive_menu[i].activated = drive_menu_template[i].activated;
            drive_menu[i].status    = drive_menu_template[i].status;
        }
        drive_menu[i].string = drive_menu_template[i].string;
        drive_menu[i].type = drive_menu_template[i].type;
        drive_menu[i].callback = drive_menu_template[i].callback;
        drive_menu[i].data = drive_menu_template[i].data;


        if (machine_class == VICE_MACHINE_VIC20 || machine_class == VICE_MACHINE_C64DTV) {
            newend = 1;
        } else if (machine_class == VICE_MACHINE_PLUS4) {
            newend = 2;
        }
        memset(&drive_8_parallel_menu[newend], 0, sizeof(ui_menu_entry_t));
        memset(&drive_9_parallel_menu[newend], 0, sizeof(ui_menu_entry_t));
        memset(&drive_10_parallel_menu[newend], 0, sizeof(ui_menu_entry_t));
        memset(&drive_11_parallel_menu[newend], 0, sizeof(ui_menu_entry_t));

        if (machine_class == VICE_MACHINE_PET || machine_class == VICE_MACHINE_CBM5x0 || machine_class == VICE_MACHINE_CBM6x0) {
            memset(&drive_8_menu[DRIVE_SETTINGS_OFFSET_IEEE_END], 0 , sizeof(ui_menu_entry_t));
            memset(&drive_9_menu[DRIVE_SETTINGS_OFFSET_IEEE_END], 0 , sizeof(ui_menu_entry_t));
            memset(&drive_10_menu[DRIVE_SETTINGS_OFFSET_IEEE_END], 0 , sizeof(ui_menu_entry_t));
            memset(&drive_11_menu[DRIVE_SETTINGS_OFFSET_IEEE_END], 0 , sizeof(ui_menu_entry_t));
        }

        drive_8_menu[DRIVE_SETTINGS_OFFSET_DRIVE_D1_R0].status = drive_is_dualdrive_by_devnr(8) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
        drive_9_menu[DRIVE_SETTINGS_OFFSET_DRIVE_D1_R0].status = drive_is_dualdrive_by_devnr(9) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
        drive_10_menu[DRIVE_SETTINGS_OFFSET_DRIVE_D1_R0].status = drive_is_dualdrive_by_devnr(10) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
        drive_11_menu[DRIVE_SETTINGS_OFFSET_DRIVE_D1_R0].status = drive_is_dualdrive_by_devnr(11) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;

        /* depending on the active drive type, enable the attach and detach
           menu items in the drive menu */
        for (i = 0; i < 4; i++) {
            d0 = d1 = MENU_STATUS_INACTIVE;
            if (drive_get_type_by_devnr(8 + i) != 0) {
                d0 = MENU_STATUS_ACTIVE;
                if (drive_is_dualdrive_by_devnr(8 + i)) {
                    d1 = MENU_STATUS_ACTIVE;
                }
            }
            drive_menu[0 + (i * 2)].status = d0;
            drive_menu[1 + (i * 2)].status = d1;
            drive_menu[9 + (i * 2)].status = d0;
            drive_menu[10 + (i * 2)].status = d1;

            d0 = MENU_STATUS_INACTIVE;
            if (drive_get_type_by_devnr(8 + i) == DRIVE_TYPE_CMDHD) {
                d0 = MENU_STATUS_ACTIVE;
            }
            reset_menu[7 + (i * 2) + 0].status = d0;
            reset_menu[7 + (i * 2) + 1].status = d0;
        }
    }
}
