/*
 * kbd.c - SDL keyboard driver.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "types.h"

#include "vice_sdl.h"
#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "kbd.h"
#include "fullscreenarch.h"
#include "hotkeys.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "monitor.h"
#include "resources.h"
#include "sysfile.h"
#include "ui.h"
#include "uihotkey.h"
#include "uihotkeys.h"
#include "uimenu.h"
#include "util.h"
#include "vkbd.h"

/* #define SDL_DEBUG */

static log_t sdlkbd_log = LOG_DEFAULT;

/* Menu keys */
int sdl_ui_menukeys[MENU_ACTION_NUM];


/* ------------------------------------------------------------------------ */

/* Convert 'known' keycodes to SDL1x keycodes.
   Unicode keycodes and 'unknown' keycodes are
   translated to 'SDLK_UNKNOWN'.

   This makes SDL2 key handling compatible with
   SDL1 key handling, but a proper solution for
   handling unicode will still need to be made.
 */
#ifdef USE_SDL2UI
typedef struct SDL2Key_s {
    SDLKey SDL1x;
    SDLKey SDL2x;
} SDL2Key_t;

static SDL2Key_t SDL2xKeys[] = {
    { 12, SDLK_CLEAR },
    { 19, SDLK_PAUSE },
    { 256, SDLK_KP_0 },
    { 257, SDLK_KP_1 },
    { 258, SDLK_KP_2 },
    { 259, SDLK_KP_3 },
    { 260, SDLK_KP_4 },
    { 261, SDLK_KP_5 },
    { 262, SDLK_KP_6 },
    { 263, SDLK_KP_7 },
    { 264, SDLK_KP_8 },
    { 265, SDLK_KP_9 },
    { 266, SDLK_KP_PERIOD },
    { 267, SDLK_KP_DIVIDE },
    { 268, SDLK_KP_MULTIPLY },
    { 269, SDLK_KP_MINUS },
    { 270, SDLK_KP_PLUS },
    { 271, SDLK_KP_ENTER },
    { 272, SDLK_KP_EQUALS },
    { 273, SDLK_UP },
    { 274, SDLK_DOWN },
    { 275, SDLK_RIGHT },
    { 276, SDLK_LEFT },
    { 277, SDLK_INSERT },
    { 278, SDLK_HOME },
    { 279, SDLK_END },
    { 280, SDLK_PAGEUP },
    { 281, SDLK_PAGEDOWN },
    { 282, SDLK_F1 },
    { 283, SDLK_F2 },
    { 284, SDLK_F3 },
    { 285, SDLK_F4 },
    { 286, SDLK_F5 },
    { 287, SDLK_F6 },
    { 288, SDLK_F7 },
    { 289, SDLK_F8 },
    { 290, SDLK_F9 },
    { 291, SDLK_F10 },
    { 292, SDLK_F11 },
    { 293, SDLK_F12 },
    { 294, SDLK_F13 },
    { 295, SDLK_F14 },
    { 296, SDLK_F15 },
    { 300, SDLK_NUMLOCKCLEAR },
    { 301, SDLK_CAPSLOCK },
    { 302, SDLK_SCROLLLOCK },
    { 303, SDLK_RSHIFT },
    { 304, SDLK_LSHIFT },
    { 305, SDLK_RCTRL },
    { 306, SDLK_LCTRL },
    { 307, SDLK_RALT },
    { 308, SDLK_LALT },
    { 309, SDLK_RGUI },
    { 310, SDLK_LGUI },
    { 313, SDLK_MODE },
    { 314, SDLK_APPLICATION },
    { 315, SDLK_HELP },
    { 316, SDLK_PRINTSCREEN },
    { 317, SDLK_SYSREQ },
    { 319, SDLK_MENU },
    { 320, SDLK_POWER },
    { 322, SDLK_UNDO },
    { 0, SDLK_UNKNOWN }
};

SDLKey SDL2x_to_SDL1x_Keys(SDLKey key)
{
    int i;

    /* keys 0-255 are the same on SDL1x */
    if (key < 256) {
        return key;
    }

    /* keys 0x40000xxx need translation */
    if (key & 0x40000000) {
        for (i = 0; SDL2xKeys[i].SDL1x; ++i) {
            if (SDL2xKeys[i].SDL2x == key) {
                return SDL2xKeys[i].SDL1x;
            }
        } /* fallthrough, unknown SDL2x key */
    } else { /* SDL1 format key may come from ini file */
        for (i = 0; SDL2xKeys[i].SDL1x; ++i) {
            if (SDL2xKeys[i].SDL1x == key) {
                return SDL2xKeys[i].SDL1x;
            }
        }
    }

    /* unicode key, so return 'unknown' */
    return SDLK_UNKNOWN;
}

SDLKey SDL1x_to_SDL2x_Keys(SDLKey key)
{
    int i;

    for (i = 0; SDL2xKeys[i].SDL1x; ++i) {
        if (SDL2xKeys[i].SDL1x == key) {
            return SDL2xKeys[i].SDL2x;
        }
    }

    return key;
}
#else
SDLKey SDL2x_to_SDL1x_Keys(SDLKey key)
{
    return key;
}

SDLKey SDL1x_to_SDL2x_Keys(SDLKey key)
{
    return key;
}
#endif

/* ------------------------------------------------------------------------ */

static int sdlkbd_get_modifier(SDLMod mod)
{
    int ret = 0;
    if (mod & KMOD_LSHIFT) {
        ret |= KBD_MOD_LSHIFT;
    }
    if (mod & KMOD_RSHIFT) {
        ret |= KBD_MOD_RSHIFT;
    }
    if (mod & KMOD_LALT) {
        ret |= KBD_MOD_LALT;
    }
    if (mod & KMOD_RALT) {
        ret |= KBD_MOD_RALT;
    }
    if (mod & KMOD_LCTRL) {
        ret |= KBD_MOD_LCTRL;
    }
    return ret;
}

void sdlkbd_press(SDLKey key, SDLMod mod)
{
#if 0
    ui_menu_entry_t *hotkey_action = NULL;
#endif
    ui_action_map_t *map;

#ifdef SDL_DEBUG
    log_debug(LOG_DEFAULT, "%s: %i (%s),%04x", __func__, key, SDL_GetKeyName(SDL1x_to_SDL2x_Keys(key)), mod);
#endif
#ifdef WINDOWS_COMPILE
/* HACK: The Alt-Gr Key seems to work differently on windows and linux.
         On Linux one Keypress "SDLK_RALT" will be produced.
         On Windows two Keypresses will be produced, first "SDLK_LCTRL"
         then "SDLK_RALT".
         The following is a hack to compensate for that and make it
         always work like on linux.
*/
    if (SDL1x_to_SDL2x_Keys(key) == SDLK_RALT) {
        mod &= ~KMOD_LCTRL;
        keyboard_key_released(SDL2x_to_SDL1x_Keys(SDLK_LCTRL), KBD_MOD_LCTRL);
    } else {
        if ((mod & KMOD_LCTRL) && (mod & KMOD_RALT)) {
            mod &= ~KMOD_LCTRL;
        }
    }
#endif
    if ((int)(key) == sdl_ui_menukeys[0] && hotkeys_allowed_modifiers(mod) == KMOD_NONE) {
        sdl_ui_activate();
        return;
    }

    /* This iterates an array of ~270 elements at the time of writing, exiting
     * early when a map is found for the hotkey, otherwise iterating the full
     * array.
     */
    map = ui_action_map_get_by_arch_hotkey(SDL1x_to_SDL2x_Keys(key), mod);
    if (map != NULL) {
#ifdef SDL_DEBUG
        printf("Hotkey pressed for %d (%s)\n", map->action, ui_action_get_name(map->action));
#endif
        ui_action_trigger(map->action);
        return; /* do not pass keypress to emulated keyboard */
    }

    keyboard_key_pressed((unsigned long)key, sdlkbd_get_modifier(mod));
}

void sdlkbd_release(SDLKey key, SDLMod mod)
{
#ifdef SDL_DEBUG
    log_debug(LOG_DEFAULT, "%s: %i (%s),%04x", __func__, key, SDL_GetKeyName(key), mod);
#endif

#ifdef WINDOWS_COMPILE
/* HACK: The Alt-Gr Key seems to work differently on windows and linux.
         see above */
    if (SDL1x_to_SDL2x_Keys(key) == SDLK_RALT) {
        mod &= ~KMOD_LCTRL;
    } else {
        if ((mod & KMOD_LCTRL) && (mod & KMOD_RALT)) {
            mod &= ~KMOD_LCTRL;
        }
    }
#endif

    keyboard_key_released((unsigned long)key, sdlkbd_get_modifier(mod));
}

ui_menu_action_t sdlkbd_press_for_menu_action(SDLKey key, SDLMod mod)
{
    ui_menu_action_t i, retval = MENU_ACTION_NONE;

#ifdef SDL_DEBUG
    log_debug(LOG_DEFAULT, "%s: %i (%s),%04x", __func__, key, SDL_GetKeyName(key), mod);
#endif

    if (key != SDLK_UNKNOWN) {
        for (i = MENU_ACTION_UP; i < MENU_ACTION_NUM; ++i) {
            if (sdl_ui_menukeys[i] == (int)key) {
                retval = i;
                break;
            }
        }
        if ((int)(key) == sdl_ui_menukeys[0]) {
            retval = MENU_ACTION_EXIT;
        }
    }
    return retval;
}

ui_menu_action_t sdlkbd_release_for_menu_action(SDLKey key, SDLMod mod)
{
    ui_menu_action_t i, retval = MENU_ACTION_NONE_RELEASE;

#ifdef SDL_DEBUG
    log_debug(LOG_DEFAULT, "%s: %i (%s),%04x", __func__, key, SDL_GetKeyName(key), mod);
#endif

    if (key != SDLK_UNKNOWN) {
        for (i = MENU_ACTION_UP; i < MENU_ACTION_NUM; ++i) {
            if (sdl_ui_menukeys[i] == (int)key) {
                retval = i;
                break;
            }
        }
    }
    return retval + MENU_ACTION_NONE_RELEASE;
}

/* ------------------------------------------------------------------------ */

void kbd_arch_init(void)
{
    sdlkbd_log = log_open("SDLKeyboard");

    /* initialize generic hotkeys system
     * (not sure it should be here, but seems early enough) */
    ui_hotkeys_init("sdl");

#if 0
    sdlkbd_keyword_clear();
#endif
}

signed long kbd_arch_keyname_to_keynum(char *keyname)
{
    signed long keynum = (signed long)atoi(keyname);
    if (keynum == 0) {
        log_warning(sdlkbd_log, "Keycode 0 is reserved for unknown keys.");
    }
    return keynum;
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
    joykeys[0] = SDL2x_to_SDL1x_Keys(SDLK_KP0);
    joykeys[1] = SDL2x_to_SDL1x_Keys(SDLK_KP1);
    joykeys[2] = SDL2x_to_SDL1x_Keys(SDLK_KP2);
    joykeys[3] = SDL2x_to_SDL1x_Keys(SDLK_KP3);
    joykeys[4] = SDL2x_to_SDL1x_Keys(SDLK_KP4);
    joykeys[5] = SDL2x_to_SDL1x_Keys(SDLK_KP6);
    joykeys[6] = SDL2x_to_SDL1x_Keys(SDLK_KP7);
    joykeys[7] = SDL2x_to_SDL1x_Keys(SDLK_KP8);
    joykeys[8] = SDL2x_to_SDL1x_Keys(SDLK_KP9);
    joykeys[9] = SDL2x_to_SDL1x_Keys(SDLK_KP_PERIOD);
    joykeys[10] = SDL2x_to_SDL1x_Keys(SDLK_KP_ENTER);
}


char *kbd_get_menu_keyname(void)
{
    /* any modifier used to set a menu key via the UI is accepted as the key, and
     * the resources don't support modifiers either */
    return lib_strdup(SDL_GetKeyName(SDL1x_to_SDL2x_Keys(sdl_ui_menukeys[0])));
}
