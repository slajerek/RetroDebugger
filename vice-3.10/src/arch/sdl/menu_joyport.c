/*
 * menu_joyport.c - Joyport menu for SDL UI.
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

#include "joyport.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"

#include "menu_joyport.h"

UI_MENU_DEFINE_RADIO(JoyPort1Device)
UI_MENU_DEFINE_RADIO(JoyPort2Device)
UI_MENU_DEFINE_RADIO(JoyPort3Device)
UI_MENU_DEFINE_RADIO(JoyPort4Device)
UI_MENU_DEFINE_RADIO(JoyPort5Device)
UI_MENU_DEFINE_RADIO(JoyPort6Device)
UI_MENU_DEFINE_RADIO(JoyPort7Device)
UI_MENU_DEFINE_RADIO(JoyPort8Device)
UI_MENU_DEFINE_RADIO(JoyPort9Device)
UI_MENU_DEFINE_RADIO(JoyPort10Device)
UI_MENU_DEFINE_RADIO(JoyPort11Device)

static ui_menu_entry_t joyport_dyn_menu[JOYPORT_MAX_PORTS][JOYPORT_MAX_DEVICES + 1];

static int joyport_dyn_menu_init[JOYPORT_MAX_PORTS] = { 0 };

static void sdl_menu_joyport_free(int port)
{
    ui_menu_entry_t *entry = joyport_dyn_menu[port];
    int i;

    for (i = 0; entry[i].string != NULL; i++) {
        lib_free(entry[i].string);
        entry[i].string = NULL;
    }
}

static const ui_callback_t uijoyport_device_callbacks[JOYPORT_MAX_PORTS] = {
    radio_JoyPort1Device_callback,
    radio_JoyPort2Device_callback,
    radio_JoyPort3Device_callback,
    radio_JoyPort4Device_callback,
    radio_JoyPort5Device_callback,
    radio_JoyPort6Device_callback,
    radio_JoyPort7Device_callback,
    radio_JoyPort8Device_callback,
    radio_JoyPort9Device_callback,
    radio_JoyPort10Device_callback,
    radio_JoyPort11Device_callback
};

static const char *joyport_dynmenu_helper(int port)
{
    joyport_desc_t *devices = NULL;
    ui_menu_entry_t *entry = joyport_dyn_menu[port];
    int i;

    /* rebuild menu if it already exists. */
    if (joyport_dyn_menu_init[port] != 0) {
        sdl_menu_joyport_free(port);
    } else {
        joyport_dyn_menu_init[port] = 1;
    }

    if (joyport_port_is_active(port)) {
        devices = joyport_get_valid_devices(port, 1);
        for (i = 0; devices[i].name; ++i) {
            entry[i].action   = ACTION_NONE;
            entry[i].string   = lib_strdup(devices[i].name);
            entry[i].type     = MENU_ENTRY_RESOURCE_RADIO;
            entry[i].callback = uijoyport_device_callbacks[port];
            entry[i].data     = (ui_callback_data_t)vice_int_to_ptr(devices[i].id);
        }

        entry[i].string = NULL;
        lib_free(devices);

        return MENU_SUBMENU_STRING;
    }
    return MENU_NOT_AVAILABLE_STRING;
}

static UI_MENU_CALLBACK(JoyPort1Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_1);
}

static UI_MENU_CALLBACK(JoyPort2Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_2);
}

static UI_MENU_CALLBACK(JoyPort3Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_3);
}

static UI_MENU_CALLBACK(JoyPort4Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_4);
}

static UI_MENU_CALLBACK(JoyPort5Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_5);
}

static UI_MENU_CALLBACK(JoyPort6Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_6);
}

static UI_MENU_CALLBACK(JoyPort7Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_7);
}

static UI_MENU_CALLBACK(JoyPort8Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_8);
}

static UI_MENU_CALLBACK(JoyPort9Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_9);
}

static UI_MENU_CALLBACK(JoyPort10Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_10);
}

static UI_MENU_CALLBACK(JoyPort11Device_dynmenu_callback)
{
    return joyport_dynmenu_helper(JOYPORT_11);
}

ui_menu_entry_t joyport_menu[JOYPORT_MAX_PORTS + 2];

UI_MENU_DEFINE_TOGGLE(BBRTCSave)

static const ui_callback_t uijoyport_callbacks[JOYPORT_MAX_PORTS] = {
    JoyPort1Device_dynmenu_callback,
    JoyPort2Device_dynmenu_callback,
    JoyPort3Device_dynmenu_callback,
    JoyPort4Device_dynmenu_callback,
    JoyPort5Device_dynmenu_callback,
    JoyPort6Device_dynmenu_callback,
    JoyPort7Device_dynmenu_callback,
    JoyPort8Device_dynmenu_callback,
    JoyPort9Device_dynmenu_callback,
    JoyPort10Device_dynmenu_callback,
    JoyPort11Device_dynmenu_callback
};

void uijoyport_menu_create(int p1, int p2, int p3_p5, int p6, int p7_p10, int p11)
{
    int i, j = 0;
    int port_ids[] = { p1, p2, p3_p5, p3_p5, p3_p5, p6, p7_p10, p7_p10, p7_p10, p7_p10, p11 };

    if (machine_class != VICE_MACHINE_C64DTV) {
        if (p1 || p2) {
            joyport_menu[j].action   = ACTION_NONE;
            joyport_menu[j].string   = "Save BBRTC data when changed";
            joyport_menu[j].type     = MENU_ENTRY_RESOURCE_TOGGLE;
            joyport_menu[j].callback = toggle_BBRTCSave_callback;
            joyport_menu[j].data     = NULL;
            ++j;
        }
    }

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (port_ids[i] != 0) {
            joyport_menu[j].action   = ACTION_NONE;
            joyport_menu[j].string   = (char *)joyport_get_port_name(i);
            joyport_menu[j].type     = MENU_ENTRY_DYNAMIC_SUBMENU;
            joyport_menu[j].callback = uijoyport_callbacks[i];
            joyport_menu[j].data     = (ui_callback_data_t)joyport_dyn_menu[i];
            ++j;
        }
    }

    joyport_menu[j].string = NULL;
}


/** \brief  Clean up memory used by the dynamically created joyport menus
 */
void uijoyport_menu_shutdown(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joyport_dyn_menu_init[i]) {
            sdl_menu_joyport_free(i);
        }
    }
}
