/*
 * menu_joystick.c - Joystick menu for SDL UI.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#include "actions-joystick.h"
#include "joy.h"
#include "joystick.h"
#include "kbd.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "mouse.h"
#include "resources.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"
#include "uipoll.h"
#include "userport_joystick.h"
#include "util.h"
#include "vice_sdl.h"

#include "menu_joystick.h"


UI_MENU_DEFINE_RADIO(JoyDevice1)
UI_MENU_DEFINE_RADIO(JoyDevice2)
UI_MENU_DEFINE_RADIO(JoyDevice3)
UI_MENU_DEFINE_RADIO(JoyDevice4)
UI_MENU_DEFINE_RADIO(JoyDevice5)
UI_MENU_DEFINE_RADIO(JoyDevice6)
UI_MENU_DEFINE_RADIO(JoyDevice7)
UI_MENU_DEFINE_RADIO(JoyDevice8)
UI_MENU_DEFINE_RADIO(JoyDevice9)
UI_MENU_DEFINE_RADIO(JoyDevice10)
UI_MENU_DEFINE_RADIO(JoyDevice11)

/* FIXME: proper solution would be to dynamically allocate the array of menu
 *        items per port, for now we stick with max. 16 controllers */
static ui_menu_entry_t joystick_device_dyn_menu[JOYPORT_MAX_PORTS][5 + 16];
static int joystick_device_dyn_menu_init[JOYPORT_MAX_PORTS] = { 0 };

static void sdl_menu_joystick_device_free(int port)
{
    ui_menu_entry_t *entry = joystick_device_dyn_menu[port];
    int i;

    for (i = 0; entry[i].string != NULL; i++) {
        lib_free(entry[i].string);
        entry[i].string = NULL;
    }
}

static const ui_callback_t uijoystick_device_callbacks[JOYPORT_MAX_PORTS] = {
    radio_JoyDevice1_callback,
    radio_JoyDevice2_callback,
    radio_JoyDevice3_callback,
    radio_JoyDevice4_callback,
    radio_JoyDevice5_callback,
    radio_JoyDevice6_callback,
    radio_JoyDevice7_callback,
    radio_JoyDevice8_callback,
    radio_JoyDevice9_callback,
    radio_JoyDevice10_callback,
    radio_JoyDevice11_callback
};

static const char *joystick_device_dynmenu_helper(int port)
{
    int j = 0, id;
    ui_menu_entry_t *entry = joystick_device_dyn_menu[port];
    const char *device_name;
#ifdef HAVE_SDL_NUMJOYSTICKS
    int n;
#endif

    /* rebuild menu if it already exists. */
    if (joystick_device_dyn_menu_init[port] != 0) {
        sdl_menu_joystick_device_free(port);
    } else {
        joystick_device_dyn_menu_init[port] = 1;
    }

    if (joyport_has_mapping(port)) {
        entry[j].action   = ACTION_NONE;
        entry[j].string   = lib_strdup("None");
        entry[j].type     = MENU_ENTRY_RESOURCE_RADIO;
        entry[j].callback = uijoystick_device_callbacks[port];
        entry[j].data     = (ui_callback_data_t)vice_int_to_ptr(JOYDEV_NONE);
        j++;

        entry[j].action   = ACTION_NONE;
        entry[j].string   = lib_strdup("Numpad");
        entry[j].type     = MENU_ENTRY_RESOURCE_RADIO;
        entry[j].callback = uijoystick_device_callbacks[port];
        entry[j].data     = (ui_callback_data_t)vice_int_to_ptr(JOYDEV_NUMPAD);
        j++;

        entry[j].action   = ACTION_NONE;
        entry[j].string   = lib_strdup("Keyset 1");
        entry[j].type     = MENU_ENTRY_RESOURCE_RADIO;
        entry[j].callback = uijoystick_device_callbacks[port];
        entry[j].data     = (ui_callback_data_t)vice_int_to_ptr(JOYDEV_KEYSET1);
        j++;

        entry[j].action   = ACTION_NONE;
        entry[j].string   = lib_strdup("Keyset 2");
        entry[j].type     = MENU_ENTRY_RESOURCE_RADIO;
        entry[j].callback = uijoystick_device_callbacks[port];
        entry[j].data     = (ui_callback_data_t)vice_int_to_ptr(JOYDEV_KEYSET2);
        j++;

#ifdef HAVE_SDL_NUMJOYSTICKS
        n = 0;
        joystick_ui_reset_device_list();
        while (n < 16 && (device_name = joystick_ui_get_next_device_name(&id)) != NULL) {
            entry[j].action   = ACTION_NONE;
            entry[j].string   = lib_strdup(device_name);
            entry[j].type     = MENU_ENTRY_RESOURCE_RADIO;
            entry[j].callback = uijoystick_device_callbacks[port];
            entry[j].data     = (ui_callback_data_t)vice_int_to_ptr(JOYDEV_JOYSTICK + n);
            j++;
            n++;
        }
#endif
        entry[j].string = NULL;

        return MENU_SUBMENU_STRING;
    }
    return MENU_NOT_AVAILABLE_STRING;
}

static UI_MENU_CALLBACK(Joystick1Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_1);
}

static UI_MENU_CALLBACK(Joystick2Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_2);
}

static UI_MENU_CALLBACK(Joystick3Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_3);
}

static UI_MENU_CALLBACK(Joystick4Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_4);
}

static UI_MENU_CALLBACK(Joystick5Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_5);
}

static UI_MENU_CALLBACK(Joystick6Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_6);
}

static UI_MENU_CALLBACK(Joystick7Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_7);
}

static UI_MENU_CALLBACK(Joystick8Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_8);
}

static UI_MENU_CALLBACK(Joystick9Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_9);
}

static UI_MENU_CALLBACK(Joystick10Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_10);
}

static UI_MENU_CALLBACK(Joystick11Device_dynmenu_callback)
{
    return joystick_device_dynmenu_helper(JOYPORT_11);
}

UI_MENU_DEFINE_TOGGLE(JoyOpposite)
UI_MENU_DEFINE_TOGGLE(JoyMenuControl)

UI_MENU_DEFINE_TOGGLE(JoyStick1AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick2AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick3AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick4AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick5AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick6AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick7AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick8AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick9AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick10AutoFire)
UI_MENU_DEFINE_TOGGLE(JoyStick11AutoFire)

UI_MENU_DEFINE_SLIDER(JoyStick1AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick2AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick3AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick4AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick5AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick6AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick7AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick8AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick9AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick10AutoFireSpeed, 1, 255)
UI_MENU_DEFINE_SLIDER(JoyStick11AutoFireSpeed, 1, 255)

UI_MENU_DEFINE_RADIO(JoyStick1AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick2AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick3AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick4AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick5AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick6AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick7AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick8AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick9AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick10AutoFireMode)
UI_MENU_DEFINE_RADIO(JoyStick11AutoFireMode)

#define VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(port)                              \
    static const ui_menu_entry_t joystick_port##port##_autofire_mode_menu[] = { \
        {   .string   = "Autofire when fire is pressed",                        \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                              \
            .callback = radio_JoyStick##port##AutoFireMode_callback,            \
            .data     = (ui_callback_data_t)JOYSTICK_AUTOFIRE_MODE_PRESS        \
        },                                                                      \
        {   .string   = "Autofire when fire is not pressed",                    \
            .type     = MENU_ENTRY_RESOURCE_RADIO,                              \
            .callback = radio_JoyStick##port##AutoFireMode_callback,            \
            .data     = (ui_callback_data_t)JOYSTICK_AUTOFIRE_MODE_PERMANENT    \
        },                                                                      \
        SDL_MENU_LIST_END                                                       \
    };

VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(1)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(2)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(3)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(4)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(5)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(6)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(7)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(8)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(9)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(10)
VICE_SDL_JOYSTICK_AUTOFIRE_MODE_MENU(11)

#define VICE_SDL_JOYSTICK_AUTOFIRE_MENU(port)                                        \
    static const ui_menu_entry_t joystick_port##port##_autofire_menu[] = {           \
        {   .string   = "Enable autofire",                                           \
            .type     = MENU_ENTRY_RESOURCE_TOGGLE,                                  \
            .callback = toggle_JoyStick##port##AutoFire_callback,                    \
        },                                                                           \
        {   .string   = "Autofire mode",                                             \
            .type     = MENU_ENTRY_SUBMENU,                                          \
            .callback = submenu_radio_callback,                                      \
            .data     = (ui_callback_data_t)joystick_port##port##_autofire_mode_menu \
        },                                                                           \
        {   .string   = "Autofire speed",                                            \
            .type     = MENU_ENTRY_RESOURCE_INT,                                     \
            .callback = slider_JoyStick##port##AutoFireSpeed_callback,               \
            .data     = (ui_callback_data_t)"Set autofire speed (1 - 255)"           \
        },                                                                           \
        SDL_MENU_LIST_END                                                            \
    };

VICE_SDL_JOYSTICK_AUTOFIRE_MENU(1)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(2)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(3)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(4)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(5)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(6)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(7)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(8)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(9)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(10)
VICE_SDL_JOYSTICK_AUTOFIRE_MENU(11)

static ui_menu_entry_t joystick_autofire_dyn_menu[JOYPORT_MAX_PORTS + 1];
static int joystick_autofire_dyn_menu_init = 0;

static const ui_menu_entry_t *joystick_port_autofire_menus[JOYPORT_MAX_PORTS] = {
    joystick_port1_autofire_menu,
    joystick_port2_autofire_menu,
    joystick_port3_autofire_menu,
    joystick_port4_autofire_menu,
    joystick_port5_autofire_menu,
    joystick_port6_autofire_menu,
    joystick_port7_autofire_menu,
    joystick_port8_autofire_menu,
    joystick_port9_autofire_menu,
    joystick_port10_autofire_menu,
    joystick_port11_autofire_menu
};

static void sdl_menu_joystick_autofire_free(void)
{
    ui_menu_entry_t *entry = joystick_autofire_dyn_menu;
    int i;

    for (i = 0; entry[i].string != NULL; i++) {
        lib_free(entry[i].string);
        entry[i].string = NULL;
    }
}

static UI_MENU_CALLBACK(joystick_autofire_dynmenu_callback)
{
    int i;
    int j = 0;
    int mappings = 0;

    /* rebuild menu if it already exists. */
    if (joystick_autofire_dyn_menu_init != 0) {
        sdl_menu_joystick_autofire_free();
    } else {
        joystick_autofire_dyn_menu_init = 1;
    }

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joyport_has_mapping(i)) {
            mappings++;
        }
    }

    if (mappings) {
        for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
            if (joyport_has_mapping(i)) {
                joystick_autofire_dyn_menu[j].action   = ACTION_NONE;
                joystick_autofire_dyn_menu[j].string   = util_concat(joyport_get_port_name(i), " Autofire", NULL);
                joystick_autofire_dyn_menu[j].type     = MENU_ENTRY_SUBMENU;
                joystick_autofire_dyn_menu[j].callback = submenu_callback;
                joystick_autofire_dyn_menu[j].data     = (ui_callback_data_t)joystick_port_autofire_menus[i];
                j++;
            }
        }
        joystick_autofire_dyn_menu[j].string = NULL;

        return MENU_SUBMENU_STRING;
    }
    return MENU_NOT_AVAILABLE_STRING;
}

#ifdef USE_SDL2UI
static UI_MENU_CALLBACK(custom_rescan_joy_callback)
{
    if (activated) {
        sdljoy_rescan();
    }
    return NULL;
}
#endif

static UI_MENU_CALLBACK(custom_keyset_callback)
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

static const ui_menu_entry_t define_keyset_menu[] = {
    {   .string   = "Keyset 1 Up",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1North"
    },
    {   .string   = "Keyset 1 Down",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1South"
    },
    {   .string   = "Keyset 1 Left",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1West"
    },
    {   .string   = "Keyset 1 Right",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1East"
    },
    {   .string   = "Keyset 1 Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire"
    },
    {   .string   = "Keyset 1 2nd Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire2"
    },
    {   .string   = "Keyset 1 3rd Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire3"
    },
    {   .string   = "Keyset 1 4th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire4"
    },
    {   .string   = "Keyset 1 5th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire5"
    },
    {   .string   = "Keyset 1 6th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire6"
    },
    {   .string   = "Keyset 1 7th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire7"
    },
    {   .string   = "Keyset 1 8th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet1Fire8"
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Keyset 2 Up",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2North"
    },
    {   .string   = "Keyset 2 Down",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2South"
    },
    {   .string   = "Keyset 2 Left",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2West"
    },
    {   .string   = "Keyset 2 Right",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2East"
    },
    {   .string   = "Keyset 2 Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire"
    },
    {   .string   = "Keyset 2 2nd Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire2"
    },
    {   .string   = "Keyset 2 3rd Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire3"
    },
    {   .string   = "Keyset 2 4th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire4"
    },
    {   .string   = "Keyset 2 5th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire5"
    },
    {   .string   = "Keyset 2 6th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire6"
    },
    {   .string   = "Keyset 2 7th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire7"
    },
    {   .string   = "Keyset 2 8th Fire",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_keyset_callback,
        .data     = (ui_callback_data_t)"KeySet2Fire8"
    },
    SDL_MENU_LIST_END
};

#ifdef HAVE_SDL_NUMJOYSTICKS
static const char *joy_pin[JOYPORT_MAX_PORTS][JOYPORT_MAX_PINS];

static const char *joy_pot[] = {
    "Pot-X",
    "Pot-Y"
};

static UI_MENU_CALLBACK(custom_joymap_callback)
{
    char *target = NULL;
    SDL_Event e;
    int pin, port;
    VICE_SDL_JoystickID joystick_device = -1;

    pin = (vice_ptr_to_int(param)) & 15;
    port = (vice_ptr_to_int(param)) >> 5;
    if (joystick_port_map[port] >= JOYDEV_REALJOYSTICK_MIN) {
        joystick_device = joy_ordinal_to_id(joystick_port_map[port] - JOYDEV_REALJOYSTICK_MIN);
    }

    if (activated) {
        target = lib_msprintf("Port %i %s (press del to clear)", port + 1, joy_pin[port][pin]);
        e = sdl_ui_poll_event("joystick", target, joystick_device, 0, 1, 0, 5);
        lib_free(target);

        switch (e.type) {
            case SDL_JOYAXISMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYHATMOTION:
                sdljoy_set_joystick(e, 1 << pin);
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE) {
                    joy_delete_pin_mapping(joystick_device, 1 << pin);
                }
                break;
            default:
                break;
        }
    } else {
        return get_joy_pin_mapping_string(joystick_device, 1 << pin);
    }

    return NULL;
}

static UI_MENU_CALLBACK(clear_joymap_callback)
{
    int pin, port, joystick_device;

    port = (vice_ptr_to_int(param)) >> 5;

    if (activated && joystick_port_map[port] >= JOYDEV_REALJOYSTICK_MIN) {
        joystick_device = joy_ordinal_to_id(joystick_port_map[port] - JOYDEV_REALJOYSTICK_MIN);
        for (pin = 0; pin < JOYPORT_MAX_PINS; pin++) {
            joy_delete_pin_mapping(joystick_device, 1 << pin);
        }
    }

    return NULL;
}

static UI_MENU_CALLBACK(custom_joymap_axis_callback)
{
    char *target = NULL;
    SDL_Event e;
    int pot, port;
    VICE_SDL_JoystickID joystick_device = -1;

    pot = (vice_ptr_to_int(param)) & 15;
    port = (vice_ptr_to_int(param)) >> 5;
    if (joystick_port_map[port] >= JOYDEV_REALJOYSTICK_MIN) {
        joystick_device = joy_ordinal_to_id(joystick_port_map[port] - JOYDEV_REALJOYSTICK_MIN);
    }

    if (activated) {
        target = lib_msprintf("Port %i %s (del clears mappings)", port + 1, joy_pot[pot]);
        e = sdl_ui_poll_event("joystick", target, joystick_device, 0, 1, 0, 5);
        lib_free(target);

        switch (e.type) {
            case SDL_JOYAXISMOTION:
                sdljoy_set_joystick_axis(e, pot);
                resources_set_int_sprintf("PaddlesInput%d", PADDLES_INPUT_JOY_AXIS, port + 1);
                break;
            case SDL_MOUSEMOTION:
                joy_delete_pot_mapping(joystick_device, pot);
                resources_set_int_sprintf("PaddlesInput%d", PADDLES_INPUT_MOUSE, port + 1);
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE) {
                    joy_delete_pot_mapping(joystick_device, pot);
                }
                break;
            default:
                break;
        }
    } else {
        return get_joy_pot_mapping_string(joystick_device, pot);
    }

    return NULL;
}

static UI_MENU_CALLBACK(custom_joy_misc_callback)
{
    char *target = NULL;
    SDL_Event e;
    int type;

    type = vice_ptr_to_int(param);

    if (activated) {
        target = lib_msprintf("%s (del clears mappings)", type ? "Map" : "Menu activate");
        e = sdl_ui_poll_event("joystick", target, -1, 1, 1, 0, 5); /* TODO joystick */
        lib_free(target);

        switch (e.type) {
            case SDL_JOYAXISMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYHATMOTION:
                sdljoy_set_extra(e, type);
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE) {
                    joy_delete_extra_mapping(type);
                }
                break;
            default:
                break;
        }
    } else {
        return get_joy_extra_mapping_string(type);
    }

    return NULL;
}

UI_MENU_DEFINE_SLIDER(JoyThreshold, 0, 32767)
UI_MENU_DEFINE_SLIDER(JoyFuzz, 0, 32767)

static const ui_menu_entry_t define_joy_misc_menu[] = {
    {   .string   = "Menu activate",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_joy_misc_callback,
        .data     = (ui_callback_data_t)0
    },
    {   .string   = "Map",
        .type     = MENU_ENTRY_DIALOG,
        .callback = custom_joy_misc_callback,
        .data     = (ui_callback_data_t)1
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Threshold",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = slider_JoyThreshold_callback,
        .data     = (ui_callback_data_t)"Set joystick threshold (0 - 32767)"
    },
    {   .string   = "Fuzz",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = slider_JoyFuzz_callback,
        .data     = (ui_callback_data_t)"Set joystick fuzz (0 - 32767)"
    },
    SDL_MENU_LIST_END
};

static ui_menu_entry_t joystick_mapping_dyn_menu[JOYPORT_MAX_PORTS][JOYPORT_MAX_PINS + JOYPORT_MAX_POTS + 2];
static int joystick_mapping_dyn_menu_init[JOYPORT_MAX_PORTS] = { 0 };

static void sdl_menu_joystick_mapping_free(int port)
{
    ui_menu_entry_t *entry = joystick_mapping_dyn_menu[port];
    int i;

    for (i = 0; entry[i].string != NULL; i++) {
        lib_free(entry[i].string);
        entry[i].string = NULL;
    }
}

static const char *joystick_mapping_dynmenu_helper(int port)
{
    joyport_map_desc_t *mappings = NULL;
    ui_menu_entry_t *entry = joystick_mapping_dyn_menu[port];
    int i;
    int j = 0;
    char *mapname;

    /* rebuild menu if it already exists. */
    if (joystick_mapping_dyn_menu_init[port] != 0) {
        sdl_menu_joystick_mapping_free(port);
    } else {
        joystick_mapping_dyn_menu_init[port] = 1;
    }

    if (joyport_port_is_active(port)) {
        mappings = joyport_get_mapping(port);
        if (mappings != NULL) {
            if (mappings->pinmap != NULL) {
                for (i = 0; mappings->pinmap[i].name; i++) {
                    mapname = lib_strdup(mappings->pinmap[i].name);
                    entry[j].action   = ACTION_NONE;
                    entry[j].string   = mapname;
                    entry[j].type     = MENU_ENTRY_DIALOG;
                    entry[j].callback = custom_joymap_callback;
                    entry[j].data     = (ui_callback_data_t)vice_int_to_ptr((mappings->pinmap[i].pin | (port << 5)));
                    joy_pin[port][mappings->pinmap[i].pin] = mapname;
                    j++;
                }
            }
            if (mappings->potmap != NULL) {
                for (i = 0; mappings->potmap[i].name; i++) {
                    entry[j].action   = ACTION_NONE;
                    entry[j].string   = lib_strdup(mappings->potmap[i].name);
                    entry[j].type     = MENU_ENTRY_DIALOG;
                    entry[j].callback = custom_joymap_axis_callback;
                    entry[j].data     = (ui_callback_data_t)vice_int_to_ptr((mappings->potmap[i].pin | (port << 5)));
                    j++;
                }
            }
            entry[j].action   = ACTION_NONE;
            entry[j].string   = lib_strdup("Clear all mappings");
            entry[j].type     = MENU_ENTRY_DIALOG;
            entry[j].callback = clear_joymap_callback;
            entry[j].data     = (ui_callback_data_t)vice_int_to_ptr(port << 5);
            j++;
        }
        entry[j].string = NULL;

        return MENU_SUBMENU_STRING;
    }
    return MENU_NOT_AVAILABLE_STRING;
}

static UI_MENU_CALLBACK(Joystick1Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_1);
}

static UI_MENU_CALLBACK(Joystick2Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_2);
}

static UI_MENU_CALLBACK(Joystick3Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_3);
}

static UI_MENU_CALLBACK(Joystick4Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_4);
}

static UI_MENU_CALLBACK(Joystick5Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_5);
}

static UI_MENU_CALLBACK(Joystick6Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_6);
}

static UI_MENU_CALLBACK(Joystick7Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_7);
}

static UI_MENU_CALLBACK(Joystick8Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_8);
}

static UI_MENU_CALLBACK(Joystick9Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_9);
}

static UI_MENU_CALLBACK(Joystick10Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_10);
}

static UI_MENU_CALLBACK(Joystick11Mapping_dynmenu_callback)
{
    return joystick_mapping_dynmenu_helper(JOYPORT_11);
}

static ui_menu_entry_t joystick_host_mapping_dyn_menu[JOYPORT_MAX_PORTS + 1];
static int joystick_host_mapping_dyn_menu_init = 0;

static const ui_callback_t uijoystick_host_mapping_callbacks[JOYPORT_MAX_PORTS] = {
    Joystick1Mapping_dynmenu_callback,
    Joystick2Mapping_dynmenu_callback,
    Joystick3Mapping_dynmenu_callback,
    Joystick4Mapping_dynmenu_callback,
    Joystick5Mapping_dynmenu_callback,
    Joystick6Mapping_dynmenu_callback,
    Joystick7Mapping_dynmenu_callback,
    Joystick8Mapping_dynmenu_callback,
    Joystick9Mapping_dynmenu_callback,
    Joystick10Mapping_dynmenu_callback,
    Joystick11Mapping_dynmenu_callback
};

static void sdl_menu_joystick_host_mapping_free(void)
{
    ui_menu_entry_t *entry = joystick_host_mapping_dyn_menu;
    int i;

    for (i = 0; entry[i].string != NULL; i++) {
        lib_free(entry[i].string);
        entry[i].string = NULL;
    }
}

static UI_MENU_CALLBACK(joystick_host_mapping_dynmenu_callback)
{
    int i;
    int j = 0;
    int mappings = 0;

    /* rebuild menu if it already exists. */
    if (joystick_host_mapping_dyn_menu_init != 0) {
        sdl_menu_joystick_host_mapping_free();
    } else {
        joystick_host_mapping_dyn_menu_init = 1;
    }

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joyport_has_mapping(i)) {
            mappings++;
        }
    }

    if (mappings) {
        for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
            if (joyport_has_mapping(i)) {
                joystick_host_mapping_dyn_menu[j].action   = ACTION_NONE;
                joystick_host_mapping_dyn_menu[j].string   = util_concat("Host -> ", joyport_get_port_name(i), NULL);
                joystick_host_mapping_dyn_menu[j].type     = MENU_ENTRY_SUBMENU;
                joystick_host_mapping_dyn_menu[j].callback = uijoystick_host_mapping_callbacks[i];
                joystick_host_mapping_dyn_menu[j].data     = (ui_callback_data_t)joystick_mapping_dyn_menu[i];
                j++;
            }
        }
        joystick_host_mapping_dyn_menu[j].string = NULL;

        return MENU_SUBMENU_STRING;
    }
    return MENU_NOT_AVAILABLE_STRING;
}
#endif

const ui_menu_entry_t joystick_menu[] = {
    {   .string   = "Native joystick port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[0]
    },
    {   .string   = "Native joystick port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick2Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[1]
    },
    {   .string   = "Joystick adapter port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick3Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[2]
    },
    {   .string   = "Joystick adapter port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick4Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[3]
    },
    {   .string   = "Joystick adapter port 3",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick5Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[4]
    },
    {   .string   = "Joystick adapter port 4",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick6Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[5]
    },
    {   .string   = "Joystick adapter port 5",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick7Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[6]
    },
    {   .string   = "Joystick adapter port 6",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick8Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[7]
    },
    {   .string   = "Joystick adapter port 7",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick9Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[8]
    },
    {   .string   = "Joystick adapter port 8",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick10Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[9]
    },
    {   .action    = ACTION_SWAP_CONTROLPORT_TOGGLE,
        .string    = "Swap native joystick ports",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = swap_controlport_toggle_display
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Allow opposite directions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyOpposite_callback
    },
    {   .action   = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .string   = "Allow keyset joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "KeySetEnable"
    },
    {   .string   = "Enable joystick menu navigation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyMenuControl_callback
    },
    {   .string   = "Define keysets",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_keyset_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autofire options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_autofire_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_autofire_dyn_menu
    },
#ifdef HAVE_SDL_NUMJOYSTICKS
    {   .string   = "Host joystick mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_host_mapping_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_host_mapping_dyn_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Rescan host joysticks",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_rescan_joy_callback
    },
#endif
    {   .string   = "Extra joystick options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_joy_misc_menu
    },
#endif
    SDL_MENU_LIST_END
};

const ui_menu_entry_t joystick_c64_menu[] = {
    {   .string   = "Native joystick port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[0]
    },
    {   .string   = "Native joystick port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick2Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[1]
    },
    {   .string   = "Joystick adapter port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick3Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[2]
    },
    {   .string   = "Joystick adapter port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick4Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[3]
    },
    {   .string   = "Joystick adapter port 3",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick5Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[4]
    },
    {   .string   = "Joystick adapter port 4",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick6Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[5]
    },
    {   .string   = "Joystick adapter port 5",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick7Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[6]
    },
    {   .string   = "Joystick adapter port 6",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick8Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[7]
    },
    {   .string   = "Joystick adapter port 7",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick9Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[8]
    },
    {   .string   = "Joystick adapter port 8",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick10Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[9]
    },
    {   .action    = ACTION_SWAP_CONTROLPORT_TOGGLE,
        .string    = "Swap native joystick ports",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = swap_controlport_toggle_display
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Allow opposite directions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyOpposite_callback,
    },
    {   .action   = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .string   = "Allow keyset joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "KeySetEnable"
    },
    {   .string   = "Enable joystick menu navigation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyMenuControl_callback
    },
    {   .string   = "Define keysets",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_keyset_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autofire options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_autofire_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_autofire_dyn_menu
    },
#ifdef HAVE_SDL_NUMJOYSTICKS
    {   .string   = "Host joystick mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_host_mapping_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_host_mapping_dyn_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Rescan host joysticks",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_rescan_joy_callback
    },
#endif
    {   .string   = "Extra joystick options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_joy_misc_menu
    },
#endif
    SDL_MENU_LIST_END
};

const ui_menu_entry_t joystick_c64dtv_menu[] = {
    {   .string   = "Native joystick port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[0]
    },
    {   .string   = "Native joystick port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick2Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[1]
    },
    {   .string   = "Joystick adapter port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick3Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[2]
    },
    {   .string   = "Joystick adapter port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick4Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[3]
    },
    {   .string   = "Joystick adapter port 3",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick5Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[4]
    },
    {   .string   = "Joystick adapter port 4",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick6Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[5]
    },
    {   .string   = "Joystick adapter port 5",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick7Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[6]
    },
    {   .string   = "Joystick adapter port 6",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick8Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[7]
    },
    {   .string   = "Joystick adapter port 7",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick9Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[8]
    },
    {   .string   = "Joystick adapter port 8",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick10Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[9]
    },
    {   .action    = ACTION_SWAP_CONTROLPORT_TOGGLE,
        .string    = "Swap native joystick ports",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = swap_controlport_toggle_display
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Allow opposite directions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyOpposite_callback
    },
    {   .action   = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .string   = "Allow keyset joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "KeySetEnable"
    },
    {   .string   = "Enable joystick menu navigation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyMenuControl_callback
    },
    {   .string   = "Define keysets",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_keyset_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autofire options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_autofire_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_autofire_dyn_menu
    },
#ifdef HAVE_SDL_NUMJOYSTICKS
    {   .string   = "Host joystick mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_host_mapping_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_host_mapping_dyn_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Rescan host joysticks",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_rescan_joy_callback
    },
#endif
    {   .string   = "Extra joystick options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_joy_misc_menu
    },
#endif
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(SIDCartJoy)

const ui_menu_entry_t joystick_plus4_menu[] = {
    {   .string   = "Native joystick port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[0]
    },
    {   .string   = "Native joystick port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick2Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[1]
    },
    {   .string   = "Joystick adapter port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick3Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[2]
    },
    {   .string   = "Joystick adapter port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick4Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[3]
    },
    {   .string   = "Joystick adapter port 3",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick5Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[4]
    },
    {   .string   = "Joystick adapter port 4",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick6Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[5]
    },
    {   .string   = "Joystick adapter port 5",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick7Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[6]
    },
    {   .string   = "Joystick adapter port 6",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick8Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[7]
    },
    {   .string   = "Joystick adapter port 7",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick9Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[8]
    },
    {   .string   = "Joystick adapter port 8",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick10Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[9]
    },
    {   .string   = "SID cartridge joystick port",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick11Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[10]
    },
    {   .action    = ACTION_SWAP_CONTROLPORT_TOGGLE,
        .string    = "Swap native joystick ports",
        .type      = MENU_ENTRY_OTHER_TOGGLE,
        .displayed = swap_controlport_toggle_display
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Allow opposite directions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyOpposite_callback
    },
    {   .action   = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .string   = "Allow keyset joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "KeySetEnable"
    },
    {   .string   = "Enable joystick menu navigation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyMenuControl_callback
    },
    {   .string   = "Define keysets",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_keyset_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "SID Cart Joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_SIDCartJoy_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autofire options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_autofire_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_autofire_dyn_menu
    },
#ifdef HAVE_SDL_NUMJOYSTICKS
    {   .string   = "Host joystick mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_host_mapping_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_host_mapping_dyn_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Rescan host joysticks",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_rescan_joy_callback
    },
#endif
    {   .string   = "Extra joystick options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_joy_misc_menu
    },
#endif
    SDL_MENU_LIST_END
};

const ui_menu_entry_t joystick_vic20_menu[] = {
    {   .string   = "Native joystick port",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick1Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[0]
    },
    {   .string   = "Joystick adapter port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick3Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[2]
    },
    {   .string   = "Joystick adapter port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick4Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[3]
    },
    {   .string   = "Joystick adapter port 3",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick5Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[4]
    },
    {   .string   = "Joystick adapter port 4",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick6Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[5]
    },
    {   .string   = "Joystick adapter port 5",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick7Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[6]
    },
    {   .string   = "Joystick adapter port 6",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick8Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[7]
    },
    {   .string   = "Joystick adapter port 7",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick9Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[8]
    },
    {   .string   = "Joystick adapter port 8",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick10Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[9]
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Allow opposite directions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyOpposite_callback,
    },
    {   .action   = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .string   = "Allow keyset joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "KeySetEnable"
    },
    {   .string   = "Enable joystick menu navigation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyMenuControl_callback
    },
    {   .string   = "Define keysets",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_keyset_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autofire options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_autofire_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_autofire_dyn_menu
    },
#ifdef HAVE_SDL_NUMJOYSTICKS
    {   .string   = "Host joystick mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_host_mapping_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_host_mapping_dyn_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Rescan host joysticks",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_rescan_joy_callback
    },
#endif
    {   .string   = "Extra joystick options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_joy_misc_menu
    },
#endif
    SDL_MENU_LIST_END
};

const ui_menu_entry_t joystick_userport_only_menu[] = {
    {   .string   = "Userport joystick adapter port 1",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick3Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[2]
    },
    {   .string   = "Userport joystick adapter port 2",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick4Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[3]
    },
    {   .string   = "Userport joystick adapter port 3",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick5Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[4]
    },
    {   .string   = "Userport joystick adapter port 4",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick6Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[5]
    },
    {   .string   = "Userport joystick adapter port 5",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick7Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[6]
    },
    {   .string   = "Userport joystick adapter port 6",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick8Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[7]
    },
    {   .string   = "Userport joystick adapter port 7",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick9Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[8]
    },
    {   .string   = "Userport joystick adapter port 8",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = Joystick10Device_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_device_dyn_menu[9]
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Allow opposite directions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyOpposite_callback
    },
    {   .action   = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .string   = "Allow keyset joystick",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .resource = "KeySetEnable"
    },
    {   .string   = "Enable joystick menu navigation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_JoyMenuControl_callback
    },
    {   .string   = "Define keysets",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_keyset_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Autofire options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_autofire_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_autofire_dyn_menu
    },
#ifdef HAVE_SDL_NUMJOYSTICKS
    {   .string   = "Host joystick mapping",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = joystick_host_mapping_dynmenu_callback,
        .data     = (ui_callback_data_t)joystick_host_mapping_dyn_menu
    },
#ifdef USE_SDL2UI
    {   .string   = "Rescan host joysticks",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_rescan_joy_callback
    },
#endif
    {   .string   = "Extra joystick options",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)define_joy_misc_menu
    },
#endif
    SDL_MENU_LIST_END
};


void uijoystick_menu_shutdown(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joystick_mapping_dyn_menu_init[i]) {
            sdl_menu_joystick_mapping_free(i);
        }
        if (joystick_device_dyn_menu_init[i]) {
            sdl_menu_joystick_device_free(i);
        }
    }
    if (joystick_autofire_dyn_menu_init) {
        sdl_menu_joystick_autofire_free();
    }
    if (joystick_host_mapping_dyn_menu_init) {
        sdl_menu_joystick_host_mapping_free();
    }
}
