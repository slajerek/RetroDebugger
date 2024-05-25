/*
 * kbd.c - SDL keyboard driver.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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
#include "vicetypes.h"

#include "vice_sdl.h"
#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "kbd.h"
#include "fullscreenarch.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "monitor.h"
#include "resources.h"
#include "sysfile.h"
#include "ui.h"
#include "uihotkey.h"
#include "uimenu.h"
#include "util.h"
#include "vkbd.h"

static log_t sdlkbd_log = LOG_ERR;

/* Hotkey filename */
static char *hotkey_file = NULL;

/* Menu keys */
int sdl_ui_menukeys[MENU_ACTION_NUM];

///* UI hotkeys: index is the key(combo), value is a pointer to the menu item.
//   4 is the number of the supported modifiers: shift, alt, control, meta. */
//#define SDLKBD_UI_HOTKEYS_MAX (SDLK_LAST * (1 << 4))
//ui_menu_entry_t *sdlkbd_ui_hotkeys[SDLKBD_UI_HOTKEYS_MAX];

/* ------------------------------------------------------------------------ */

/* Resources.  */

static int hotkey_file_set(const char *val, void *param)
{
#ifdef SDL_DEBUG
    fprintf(stderr,"%s: %s\n",__func__,val);
#endif

//    if (util_string_set(&hotkey_file, val)) {
//        return 0;
//    }
//
//    return sdlkbd_hotkeys_load(hotkey_file);
	
	return 0;
}

static resource_string_t resources_string[] = {
    { "HotkeyFile", NULL, RES_EVENT_NO, NULL,
      &hotkey_file, hotkey_file_set, (void *)0 },
    RESOURCE_STRING_LIST_END
};

int sdlkbd_init_resources(void)
{
//    resources_string[0].factory_value = archdep_default_hotkey_file_name();
//
//    if (resources_register_string(resources_string) < 0) {
//        return -1;
//    }
    return 0;
}

void sdlkbd_resources_shutdown(void)
{
    lib_free(resources_string[0].factory_value);
    resources_string[0].factory_value = NULL;
    lib_free(hotkey_file);
    hotkey_file = NULL;
}

/* ------------------------------------------------------------------------ */

//static inline int sdlkbd_key_mod_to_index(SDLKey key, SDLMod mod)
//{
//    int i = 0;
//
//    mod &= (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_META);
//
//    if (mod) {
//        if (mod & KMOD_SHIFT) {
//            i |= (1 << 0);
//        }
//
//        if (mod & KMOD_ALT) {
//            i |= (1 << 1);
//        }
//
//        if (mod & KMOD_CTRL) {
//            i |= (1 << 2);
//        }
//
//        if (mod & KMOD_META) {
//            i |= (1 << 3);
//        }
//    }
//    return i * SDLK_LAST + key;
//}
//
//ui_menu_entry_t *sdlkbd_get_hotkey(SDLKey key, SDLMod mod)
//{
//    return sdlkbd_ui_hotkeys[sdlkbd_key_mod_to_index(key, mod)];
//}
//
//void sdlkbd_set_hotkey(SDLKey key, SDLMod mod, ui_menu_entry_t *value)
//{
//    sdlkbd_ui_hotkeys[sdlkbd_key_mod_to_index(key, mod)] = value;
//}
//

static void sdlkbd_keyword_clear(void)
{
//    int i;
//
//    for (i = 0; i < SDLKBD_UI_HOTKEYS_MAX; ++i) {
//        sdlkbd_ui_hotkeys[i] = NULL;
//    }
}

static void sdlkbd_parse_keyword(char *buffer)
{
    char *key;

    key = strtok(buffer + 1, " \t:");

    if (!strcmp(key, "CLEAR")) {
        sdlkbd_keyword_clear();
    }
}

static void sdlkbd_parse_entry(char *buffer)
{
//    char *p;
//    char *full_path;
//    int keynum;
//    ui_menu_entry_t *action;
//
//    p = strtok(buffer, " \t:");
//
//    keynum = atoi(p);
//
//    if (keynum >= SDLKBD_UI_HOTKEYS_MAX) {
//        log_error(sdlkbd_log, "Too large hotkey %i!", keynum);
//        return;
//    }
//
//    p = strtok(NULL, "\r\n");
//    if (p != NULL) {
//        full_path = lib_stralloc(p);
//        action = sdl_ui_hotkey_action(p);
//        if (action == NULL) {
//            log_warning(sdlkbd_log, "Cannot find menu item \"%s\"!", full_path);
//        } else {
//            sdlkbd_ui_hotkeys[keynum] = action;
//        }
//        lib_free(full_path);
//    }
}

int sdlkbd_hotkeys_load(const char *filename)
{
//    FILE *fp;
//    char *complete_path = NULL;
//    char buffer[1000];
//
//    /* Silently ignore keymap load on resource & cmdline init */
//    if (sdlkbd_log == LOG_ERR) {
//        return 0;
//    }
//
//    if (filename == NULL) {
//        log_warning(sdlkbd_log, "Failed to open NULL.");
//        return -1;
//    }
//
//    fp = sysfile_open(filename, &complete_path, MODE_READ_TEXT);
//
//    if (fp == NULL) {
//        log_warning(sdlkbd_log, "Failed to open `%s'.", filename);
//        return -1;
//    }
//
//    log_message(sdlkbd_log, "Loading hotkey map `%s'.", complete_path);
//
//    lib_free(complete_path);
//
//    do {
//        buffer[0] = 0;
//        if (fgets(buffer, 999, fp)) {
//            char *p;
//
//            if (strlen(buffer) == 0) {
//                break;
//            }
//            buffer[strlen(buffer) - 1] = 0; /* remove newline */
//
//            /* remove comments */
//            if ((p = strchr(buffer, '#'))) {
//                *p=0;
//            }
//
//            switch(*buffer) {
//                case 0:
//                    break;
//                case '!':
//                    /* keyword handling */
//                    sdlkbd_parse_keyword(buffer);
//                    break;
//                default:
//                    /* table entry handling */
//                    sdlkbd_parse_entry(buffer);
//                    break;
//            }
//        }
//    } while (!feof(fp));
//    fclose(fp);

    return 0;
}

int sdlkbd_hotkeys_dump(const char *filename)
{
//    FILE *fp;
//    int i;
//    char *hotkey_path;
//
//    if (filename == NULL)
//        return -1;
//
//    fp = fopen(filename, MODE_WRITE_TEXT);
//
//    if (fp == NULL) {
//        return -1;
//    }
//
//    fprintf(fp, "# VICE hotkey mapping file\n"
//            "#\n"
//            "# A hotkey map is read in as patch to the current map.\n"
//            "#\n"
//            "# File format:\n"
//            "# - comment lines start with '#'\n"
//            "# - keyword lines start with '!keyword'\n"
//            "# - normal line has 'keynum path&to&menuitem'\n"
//            "#\n"
//            "# Keywords and their lines are:\n"
//            "# '!CLEAR'    clear all mappings\n"
//            "#\n\n"
//        );
//
//    fprintf(fp, "!CLEAR\n\n");
//
//    for (i = 0; i < SDLKBD_UI_HOTKEYS_MAX; ++i) {
//        if (sdlkbd_ui_hotkeys[i]) {
//            hotkey_path = sdl_ui_hotkey_path(sdlkbd_ui_hotkeys[i]);
//            fprintf(fp,"%i %s\n",i,hotkey_path);
//            lib_free(hotkey_path);
//        }
//    }
//
//    fclose(fp);

    return 0;
}

/* ------------------------------------------------------------------------ */

//ui_menu_action_t sdlkbd_press(SDLKey key, SDLMod mod)
//{
//    ui_menu_action_t i, retval = MENU_ACTION_NONE;
//    ui_menu_entry_t *hotkey_action = NULL;
//
//#ifdef SDL_DEBUG
//    fprintf(stderr, "%s: %i (%s),%i\n",__func__,key,SDL_GetKeyName(key),mod);
//#endif
//    if (sdl_menu_state || (sdl_vkbd_state & SDL_VKBD_ACTIVE)) {
//        if (key != SDLK_UNKNOWN) {
//            for (i = MENU_ACTION_UP; i < MENU_ACTION_NUM; ++i) {
//                if (sdl_ui_menukeys[i] == (int)key) {
//                    retval = i;
//                    break;
//                }
//            }
//            if ((int)(key) == sdl_ui_menukeys[0]) {
//                retval = MENU_ACTION_EXIT;
//            }
//        }
//        return retval;
//    }
//
//    if ((int)(key) == sdl_ui_menukeys[0]) {
//        sdl_ui_activate();
//        return retval;
//    }
//
//    if ((hotkey_action = sdlkbd_get_hotkey(key, mod)) != NULL) {
//        sdl_ui_hotkey(hotkey_action);
//        return retval;
//    }
//
//    keyboard_key_pressed((unsigned long)key);
//    return retval;
//}
//
//ui_menu_action_t sdlkbd_release(SDLKey key, SDLMod mod)
//{
//    ui_menu_action_t i, retval = MENU_ACTION_NONE_RELEASE;
//
//#ifdef SDL_DEBUG
//    fprintf(stderr, "%s: %i (%s),%i\n", __func__, key, SDL_GetKeyName(key), mod);
//#endif
//    if (sdl_vkbd_state & SDL_VKBD_ACTIVE) {
//        if (key != SDLK_UNKNOWN) {
//            for (i = MENU_ACTION_UP; i < MENU_ACTION_NUM; ++i) {
//                if (sdl_ui_menukeys[i] == (int)key) {
//                    retval = i;
//                    break;
//                }
//            }
//        }
//        return retval + MENU_ACTION_NONE_RELEASE;
//    }
//
//    keyboard_key_released((unsigned long)key);
//    return retval;
//}

/* ------------------------------------------------------------------------ */

void kbd_arch_init(void)
{
	LOGD("kbd_arch_init");
	
//#ifdef SDL_DEBUG
//    fprintf(stderr, "%s: hotkey table size %u (%lu bytes)\n", __func__, SDLKBD_UI_HOTKEYS_MAX, SDLKBD_UI_HOTKEYS_MAX * sizeof(ui_menu_entry_t *));
//#endif

//    sdlkbd_log = log_open("SDLKeyboard");
//
//    sdlkbd_keyword_clear();
//    sdlkbd_hotkeys_load(hotkey_file);
}

signed long kbd_arch_keyname_to_keynum(char *keyname)
{
    return (signed long)atoi(keyname);
}

const char *kbd_arch_keynum_to_keyname(signed long keynum)
{
    static char keyname[20];

    memset(keyname, 0, 20);

    sprintf(keyname, "%li", keynum);

    return keyname;
}

void kbd_initialize_numpad_joykeys(int* joykeys)
{
//    joykeys[0] = SDLK_KP0;
//    joykeys[1] = SDLK_KP1;
//    joykeys[2] = SDLK_KP2;
//    joykeys[3] = SDLK_KP3;
//    joykeys[4] = SDLK_KP4;
//    joykeys[5] = SDLK_KP6;
//    joykeys[6] = SDLK_KP7;
//    joykeys[7] = SDLK_KP8;
//    joykeys[8] = SDLK_KP9;
}

int kbd_arch_get_host_mapping(void)
{
//	LOGTODO("!!!!!!!!!!kbd_arch_get_host_mapping");
	
//	int n;
//	char *l;
//	int maps[KBD_MAPPING_NUM] = {
//		KBD_MAPPING_US, KBD_MAPPING_UK, KBD_MAPPING_DE, KBD_MAPPING_DA,
//		KBD_MAPPING_NO, KBD_MAPPING_FI, KBD_MAPPING_IT };
//	char str[KBD_MAPPING_NUM][3] = {
//		"us", "uk", "de", "da", "no", "fi", "it"};
//	/* setup the locale */
//	setlocale(LC_ALL, "");
//	l = setlocale(LC_ALL, NULL);
//	if (l && (strlen(l) > 1)) {
//		for (n = 1; n < KBD_MAPPING_NUM; n++) {
//			if (memcmp(l, str[n], 2) == 0) {
//				return maps[n];
//			}
//		}
//	}
	return KBD_MAPPING_US;
}
