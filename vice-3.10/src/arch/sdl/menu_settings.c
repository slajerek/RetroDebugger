/*
 * menu_settings.c - Common SDL settings related functions.
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
#include "types.h"

#include "vice_sdl.h"
#include <stdlib.h>

#include "joystick.h"
#include "kbd.h"
#include "keyboard.h"
#include "keymap.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_settings.h"
#include "resources.h"
#include "ui.h"
#include "uifilereq.h"
#include "uihotkeys.h"
#include "uimenu.h"
#include "uipoll.h"
#include "uistatusbar.h"

#if 0
static UI_MENU_CALLBACK(save_settings_callback)
{
    if (activated) {
        if (resources_save(NULL) < 0) {
            ui_error("Cannot save current settings.");
        } else {
            ui_message("Settings saved.");
        }

        /* TODO:
           to be uncommented after the fliplist menus have been made
        uifliplist_save_settings(); */
    }
    return NULL;
}
#endif

static UI_MENU_CALLBACK(save_settings_to_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose settings file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (resources_save(name) < 0) {
                ui_error("Cannot save current settings.");
            } else {
                ui_message("Settings saved.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_SETTINGS_SAVE_TO);
    }
    return NULL;
}

#if 0
static UI_MENU_CALLBACK(load_settings_callback)
{
    if (activated) {
        if (resources_reset_and_load(NULL) < 0) {
            ui_error("Cannot load settings.");
        } else {
            ui_message("Settings loaded.");
        }
    }
    return NULL;
}
#endif

static UI_MENU_CALLBACK(load_settings_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose settings file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_reset_and_load(name) < 0) {
                ui_error("Cannot load settings.");
            } else {
                ui_message("Settings loaded.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_SETTINGS_LOAD_FROM);
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_extra_settings_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose settings file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_load(name) < 0) {
                ui_error("Cannot load settings.");
            } else {
                ui_message("Settings loaded.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_SETTINGS_LOAD_EXTRA);
    }
    return NULL;
}

#if 0
static UI_MENU_CALLBACK(default_settings_callback)
{
    if (activated) {
        resources_set_defaults();
        ui_message("Default settings restored.");
    }
    return NULL;
}
#endif

static UI_MENU_CALLBACK(save_keymap_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose file for keymap", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (keyboard_keymap_dump(name) < 0) {
                ui_error("Cannot save keymap.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

/* update mapping type ("KeymapIndex") */
static UI_MENU_CALLBACK(radio_KeymapIndex_callback)
{
    const char *res = sdl_ui_menu_radio_helper(activated, param, "KeymapIndex");
    if (activated) {
        /* FIXME: update keyboard type menu (PET/C128) */
        resources_touch("KeyboardMapping");
        uikeyboard_update_mapping_menu();   /* host layout */
        uikeyboard_update_index_menu();     /* mapping type (self) */
    }
    return res;
}

/* mapping type ("KeymapIndex") */
static ui_menu_entry_t *keymap_index_submenu;

static const ui_menu_entry_t keymap_index_submenu_entries[] = {
    {   .string   = "Symbolic",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeymapIndex_callback,
        .data     = (ui_callback_data_t)KBD_INDEX_SYM
    },
    {   .string   = "Positional",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeymapIndex_callback,
        .data     = (ui_callback_data_t)KBD_INDEX_POS
    },
    {   .string   = "Symbolic (user)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeymapIndex_callback,
        .data     = (ui_callback_data_t)KBD_INDEX_USERSYM
    },
    {   .string   = "Positional (user)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_KeymapIndex_callback,
        .data     = (ui_callback_data_t)KBD_INDEX_USERPOS
    },
    SDL_MENU_LIST_END
};

/* offset in settings_manager_menu[] */
#define SETTINGS_KEYBOARD_MAPPING_IDX   10
#define SETTINGS_ACTIVE_KEYMAP_IDX      11

/* update of mapping types ("KeymapIndex") */
void uikeyboard_update_index_menu(void)
{
    int idx, type, mapping;
    ui_menu_entry_t *entry;

    resources_get_int("KeyboardType", &type);
    resources_get_int("KeyboardMapping", &mapping);

    if(settings_manager_menu[SETTINGS_ACTIVE_KEYMAP_IDX].data) {
        lib_free(settings_manager_menu[SETTINGS_ACTIVE_KEYMAP_IDX].data);
    }

    entry = keymap_index_submenu = lib_malloc(sizeof(ui_menu_entry_t) * (5));
    for (idx = 0; idx < 4; idx++) {
        if (!((idx < 2) && (keyboard_is_keymap_valid(idx, mapping, type) < 0))) {
            memcpy(entry, &keymap_index_submenu_entries[idx], sizeof(ui_menu_entry_t));
            entry++;
        }
    }
    memset(entry, 0, sizeof(ui_menu_entry_t));
    settings_manager_menu[SETTINGS_ACTIVE_KEYMAP_IDX].data = keymap_index_submenu;
}

/* select type of host mapping ("KeyboardMapping") */
static UI_MENU_CALLBACK(radio_KeyboardMapping_callback)
{
    const char *res = sdl_ui_menu_radio_helper(activated, param, "KeyboardMapping");
    if (activated) {
        /* FIXME: update keyboard type menu (PET/C128) */
        resources_touch("KeymapIndex");
        uikeyboard_update_index_menu();     /* mapping type (self) */
        uikeyboard_update_mapping_menu();   /* host layout */
    }
    return res;
}

/* host language/layout */
static ui_menu_entry_t *keyboard_mapping_submenu;
ui_menu_entry_t ui_keyboard_mapping_entry = {
    .type     = MENU_ENTRY_RESOURCE_RADIO,
    .callback = (ui_callback_t)radio_KeyboardMapping_callback
};

/* update keymap selection (host language/layout, "KeyboardMapping") */
void uikeyboard_update_mapping_menu(void)
{
    int num;
    mapping_info_t *kbdlist = keyboard_get_info_list();
    ui_menu_entry_t *entry;

    num = keyboard_get_num_mappings();
    entry = keyboard_mapping_submenu = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));
    while(num) {
        if (!(keyboard_is_hosttype_valid(kbdlist->mapping) < 0)) {
            ui_keyboard_mapping_entry.string = kbdlist->name;
            ui_keyboard_mapping_entry.data = (ui_callback_data_t)(vice_int_to_ptr(kbdlist->mapping));
            memcpy(entry, &ui_keyboard_mapping_entry, sizeof(ui_menu_entry_t));
            entry++;
        }
        kbdlist++;
        num--;
    }
    memset(entry, 0, sizeof(ui_menu_entry_t));
    settings_manager_menu[SETTINGS_KEYBOARD_MAPPING_IDX].data = keyboard_mapping_submenu;
}

void uikeyboard_menu_create(void)
{
    uikeyboard_update_mapping_menu();   /* host layout */
    uikeyboard_update_index_menu();     /* mapping type */
}

void uikeyboard_menu_shutdown(void)
{
    lib_free(settings_manager_menu[SETTINGS_ACTIVE_KEYMAP_IDX].data);
    lib_free(settings_manager_menu[SETTINGS_KEYBOARD_MAPPING_IDX].data);
}

static UI_MENU_CALLBACK(load_sym_keymap_callback)
{
    /* temporary fix until I find out what 'keymap' is supposed to be */
#ifdef SDL_DEBUG
    int keymap = -1;
#endif
    if (activated) {
        char *name = NULL;

#ifdef SDL_DEBUG
        fprintf(stderr, "%s: map %i, \"%s\"\n", __func__, keymap, "KeymapUserSymFile");
#endif

        name = sdl_ui_file_selection_dialog("Choose keymap file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("KeymapUserSymFile", name)) {
                ui_error("Cannot load keymap.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_pos_keymap_callback)
{
    /* temporary fix until I find out what 'keymap' is supposed to be */
#ifdef SDL_DEBUG
    int keymap = -1;
#endif

    if (activated) {
        char *name = NULL;

#ifdef SDL_DEBUG
        fprintf(stderr, "%s: map %i, \"%s\"\n", __func__, keymap, "KeymapUserPosFile");
#endif

        name = sdl_ui_file_selection_dialog("Choose keymap file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("KeymapUserPosFile", name)) {
                ui_error("Cannot load keymap.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_hotkeys_to_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose hotkey file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (ui_hotkeys_save_as(name)) {
                ui_message("Hotkeys saved to '%s'.", name);
            } else {
                ui_error("Failed to save hotkeys to '%s.", name);
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_HOTKEYS_SAVE_TO);
    }
    return NULL;
}

/* Load hotkeys file by setting "HotkeyFile" resource, and mark UI action
 * finished.
 */
static UI_MENU_CALLBACK(load_hotkeys_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose hotkeys file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("HotkeyFile", name) < 0) {
                ui_error("Cannot load hotkeys.");
            } else {
                ui_message("Hotkeys loaded.");
            }
            lib_free(name);
        }
        ui_action_finish(ACTION_HOTKEYS_LOAD_FROM);
    }
    return NULL;
}

#ifdef HAVE_SDL_NUMJOYSTICKS
static UI_MENU_CALLBACK(save_joymap_callback)
{
    const char *file = NULL;
    if (activated) {
        if (resources_get_string("JoyMapFile", &file)) {
            ui_error("Cannot find resource.");
            return NULL;
        }

        if (joy_arch_mapping_dump(file)) {
            ui_error("Cannot save joymap.");
        } else {
            ui_message("Joymap saved.");
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(save_joymap_to_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose joystick map file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (joy_arch_mapping_dump(name) < 0) {
                ui_error("Cannot save joymap.");
            } else {
                ui_message("Joymap saved.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_joymap_callback)
{
    const char *file = NULL;
    if (activated) {
        if (resources_get_string("JoyMapFile", &file)) {
            ui_error("Cannot find resource.");
            return NULL;
        }

        if (joy_arch_mapping_load(file)) {
            ui_error("Cannot load joymap.");
        } else {
            ui_message("Joymap loaded.");
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(load_joymap_from_callback)
{
    if (activated) {
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose joystick map file", FILEREQ_MODE_CHOOSE_FILE);

        if (name != NULL) {
            if (resources_set_string("JoyMapFile", name) < 0) {
                ui_error("Cannot load joymap.");
            } else {
                ui_message("Joymap loaded.");
            }
            lib_free(name);
        }
    }
    return NULL;
}
#endif

UI_MENU_DEFINE_TOGGLE(SaveResourcesOnExit)
UI_MENU_DEFINE_TOGGLE(ConfirmOnExit)

static UI_MENU_CALLBACK(custom_ui_keyset_callback)
{
    SDL_Event e;
    int previous;

    if (resources_get_int((const char *)param, &previous)) {
        return sdl_menu_text_unknown;
    }

    if (activated) {
        e = sdl_ui_poll_event("key", (const char *)param, -1, 0, 1, 1, 5);

        if (e.type == SDL_KEYDOWN) {
            resources_set_int((const char *)param, (int)SDL2x_to_SDL1x_Keys(e.key.keysym.sym));
        }
    } else {
        return SDL_GetKeyName(SDL1x_to_SDL2x_Keys(previous));
    }
    return NULL;
}

static const ui_menu_entry_t define_ui_keyset_menu[] = {
    {   .string   = "Activate menu",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKey"
    },
    {   .string   = "Menu up",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyUp"
    },
    {   .string   = "Menu down",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyDown"
    },
    {   .string   = "Menu left",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyLeft"
    },
    {   .string   ="Menu right",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyRight"
    },
    {   .string   = "Menu page up",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyPageUp"
    },
    {   .string   = "Menu page down",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyPageDown"
    },
    {   .string   = "Menu home",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyHome"
    },
    {   .string   = "Menu end",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyEnd"
    },
    {   .string   = "Menu select",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeySelect"
    },
    {   .string   = "Menu cancel",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyCancel"
    },
    {   .string   = "Menu exit",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyExit"
    },
    {   .string   = "Menu map",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_ui_keyset_callback,
        .data     = (ui_callback_data_t)"MenuKeyMap"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(KbdStatusbar)

ui_menu_entry_t settings_manager_menu[] = {
    {   .action   = ACTION_SETTINGS_SAVE,
        .string   = "Save current settings",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_SETTINGS_LOAD,
        .string   = "Load settings",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_SETTINGS_SAVE_TO,
        .string   = "Save current settings to",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_settings_to_callback
    },
    {   .action   = ACTION_SETTINGS_LOAD_FROM,
        .string   = "Load settings from",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_settings_from_callback
    },
    {   .action   = ACTION_SETTINGS_LOAD_EXTRA,
        .string   = "Load extra settings from",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_extra_settings_from_callback
    },
    {   .action   = ACTION_SETTINGS_DEFAULT,
        .string   = "Restore default settings",
        .type     = MENU_ENTRY_OTHER
    },
    {   .string   = "Save settings on exit",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SaveResourcesOnExit_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Confirm on exit",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ConfirmOnExit_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    /* CAUTION: the position of this item is hardcoded above */
    {   .string   = "Keyboard mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = NULL    /* set in uikeyboard_update_index_menu() */
    },
    /* CAUTION: the position of this item is hardcoded above */
    {   .string   = "Active keymap",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = NULL    /* set in uikeyboard_update_index_menu() */
    },
    {   .string   = "Load symbolic user keymap",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_sym_keymap_callback
    },
    {   .string   = "Load positional user keymap",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_pos_keymap_callback
    },
    {   .string   = "Show keyboard status in statusbar",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_KbdStatusbar_callback,
    },
    {   .string   = "Save current keymap to",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_keymap_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_HOTKEYS_SAVE,
        .string   = "Save hotkeys",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_HOTKEYS_SAVE_TO,
        .string   = "Save hotkeys to",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_hotkeys_to_callback
    },
    {   .action   = ACTION_HOTKEYS_LOAD,
        .string   = "Load hotkeys",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_HOTKEYS_LOAD_FROM,
        .string   = "Load hotkeys from",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_hotkeys_from_callback
    },
    {   .action   = ACTION_HOTKEYS_DEFAULT,
        .string   = "Load default hotkeys",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_HOTKEYS_CLEAR,
        .string   = "Clear hotkeys",
        .type     = MENU_ENTRY_OTHER
    },

#ifdef HAVE_SDL_NUMJOYSTICKS
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Save joystick map",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_joymap_callback,
    },
    {   .string   = "Save joystick map to",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_joymap_to_callback
    },
    {   .string   = "Load joystick map from",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_joymap_from_callback
    },
    {   .string   = "Load joystick map",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_joymap_callback
    },
#endif
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Define UI keys",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_ui_keyset_menu
    },
    SDL_MENU_LIST_END
};


/* vsid setting menu */
ui_menu_entry_t settings_manager_menu_vsid[] = {
    {   .action   = ACTION_SETTINGS_SAVE,
        .string   = "Save current settings",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_SETTINGS_LOAD,
        .string   = "Load settings",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_SETTINGS_SAVE_TO,
        .string   = "Save current settings to",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_settings_to_callback
    },
    {   .action   = ACTION_SETTINGS_LOAD_FROM,
        .string   = "Load settings from",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_settings_from_callback
    },
    {   .action   = ACTION_SETTINGS_LOAD_EXTRA,
        .string   = "Load extra settings from",
        .type     = MENU_ENTRY_OTHER,
        .callback = load_extra_settings_from_callback
    },
    {   .action   = ACTION_SETTINGS_DEFAULT,
        .string   = "Restore default settings",
        .type     = MENU_ENTRY_OTHER
    },
    {   .string   = "Save settings on exit",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SaveResourcesOnExit_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Confirm on exit",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_ConfirmOnExit_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .action   = ACTION_HOTKEYS_SAVE,
        .string   = "Save hotkeys",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_HOTKEYS_SAVE_TO,
        .string   = "Save hotkeys to",
        .type     = MENU_ENTRY_OTHER,
        .callback = save_hotkeys_to_callback
    },
    {   .action   = ACTION_HOTKEYS_LOAD,
        .string   = "Load hotkeys from",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_HOTKEYS_LOAD_FROM,
        .string   = "Load hotkeys",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .action   = ACTION_HOTKEYS_DEFAULT,
        .string   = "Load default hotkeys",
        .type     = MENU_ENTRY_OTHER
    },
    {   .action   = ACTION_HOTKEYS_CLEAR,
        .string   = "Clear hotkeys",
        .type     = MENU_ENTRY_OTHER
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Define UI keys",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_ui_keyset_menu
    },
    SDL_MENU_LIST_END
};
