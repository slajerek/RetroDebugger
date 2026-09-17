/*
 * menu_ethernet.c - Ethernet settings menu for SDL UI.
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

#ifdef HAVE_RAWNET

#include <stdio.h>

#include "types.h"

#include "lib.h"
#include "menu_common.h"
#include "menu_ethernet.h"
#include "rawnet.h"
#include "resources.h"
#include "uiactions.h"
#include "uimenu.h"

#define MAXINTERFACES    20
#define MAXDRIVERS       20

UI_MENU_DEFINE_RADIO(ETHERNET_INTERFACE)
UI_MENU_DEFINE_RADIO(ETHERNET_DRIVER)

static ui_menu_entry_t ethernet_interface_dyn_menu[MAXINTERFACES + 1];
static int ethernet_interface_dyn_menu_init = 0;
static ui_menu_entry_t ethernet_driver_dyn_menu[MAXDRIVERS + 1];
static int ethernet_driver_dyn_menu_init = 0;

static void free_ethernet_interface_dyn_menu(void)
{
    int i;

    for (i = 0; ethernet_interface_dyn_menu[i].string != NULL; i++) {
        lib_free(ethernet_interface_dyn_menu[i].string);
        lib_free(ethernet_interface_dyn_menu[i].data);
    }
}

static void free_ethernet_driver_dyn_menu(void)
{
    int i;

    for (i = 0; ethernet_driver_dyn_menu[i].string != NULL; i++) {
        lib_free(ethernet_driver_dyn_menu[i].string);
        lib_free(ethernet_driver_dyn_menu[i].data);
    }
}

/* this one is called by the machine ui at shutdown */
void sdl_menu_ethernet_interface_free(void)
{
    free_ethernet_interface_dyn_menu();
    free_ethernet_driver_dyn_menu();
}

static UI_MENU_CALLBACK(ETHERNET_INTERFACE_dynmenu_callback)
{
    char *pname;
    char *pdescription;
    int i;

    if (ethernet_interface_dyn_menu_init != 0) {
        free_ethernet_interface_dyn_menu();
    } else {
        ethernet_interface_dyn_menu_init = 1;
    }

    if (!rawnet_enumadapter_open()) {
        ethernet_interface_dyn_menu[0].action   = ACTION_NONE;
        ethernet_interface_dyn_menu[0].string   = lib_strdup("No Devices Present");
        ethernet_interface_dyn_menu[0].type     = MENU_ENTRY_TEXT;
        ethernet_interface_dyn_menu[0].callback = seperator_callback;
        ethernet_interface_dyn_menu[0].data     = NULL;
        ethernet_interface_dyn_menu[1].action   = ACTION_NONE;
        ethernet_interface_dyn_menu[1].string   = NULL;
        ethernet_interface_dyn_menu[1].type     = 0;
        ethernet_interface_dyn_menu[1].callback = NULL;
        ethernet_interface_dyn_menu[1].data     = NULL;
    } else {
        for (i = 0; (rawnet_enumadapter(&pname, &pdescription)) && (i < MAXINTERFACES); i++) {
            if (pdescription) {
                ethernet_interface_dyn_menu[i].string = pdescription;
            } else {
                ethernet_interface_dyn_menu[i].string = lib_strdup(pname);
            }
            ethernet_interface_dyn_menu[i].action   = ACTION_NONE;
            ethernet_interface_dyn_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
            ethernet_interface_dyn_menu[i].callback = radio_ETHERNET_INTERFACE_callback;
            ethernet_interface_dyn_menu[i].data     = (ui_callback_data_t)pname;
        }
        ethernet_interface_dyn_menu[i].action   = ACTION_NONE;
        ethernet_interface_dyn_menu[i].string   = NULL;
        ethernet_interface_dyn_menu[i].type     = 0;
        ethernet_interface_dyn_menu[i].callback = NULL;
        ethernet_interface_dyn_menu[i].data     = NULL;
        rawnet_enumadapter_close();
    }

    return submenu_radio_callback(activated, param);
}

static UI_MENU_CALLBACK(ETHERNET_DRIVER_dynmenu_callback)
{
    char *pname;
    char *pdescription;
    int i;

    if (ethernet_driver_dyn_menu_init != 0) {
        free_ethernet_driver_dyn_menu();
    } else {
        ethernet_driver_dyn_menu_init = 1;
    }

    if (!rawnet_enumdriver_open()) {
        ethernet_driver_dyn_menu[0].action   = ACTION_NONE;
        ethernet_driver_dyn_menu[0].string   = lib_strdup("No Drivers Present");
        ethernet_driver_dyn_menu[0].type     = MENU_ENTRY_TEXT;
        ethernet_driver_dyn_menu[0].callback = seperator_callback;
        ethernet_driver_dyn_menu[0].data     = NULL;
        ethernet_driver_dyn_menu[1].action   = ACTION_NONE;
        ethernet_driver_dyn_menu[1].string   = NULL;
        ethernet_driver_dyn_menu[1].type     = 0;
        ethernet_driver_dyn_menu[1].callback = NULL;
        ethernet_driver_dyn_menu[1].data     = NULL;
    } else {
        for (i = 0; (rawnet_enumdriver(&pname, &pdescription)) && (i < MAXDRIVERS); i++) {
            if (pdescription) {
                ethernet_driver_dyn_menu[i].string = pdescription;
            } else {
                ethernet_driver_dyn_menu[i].string = lib_strdup(pname);
            }
            ethernet_driver_dyn_menu[i].action   = ACTION_NONE;
            ethernet_driver_dyn_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
            ethernet_driver_dyn_menu[i].callback = radio_ETHERNET_DRIVER_callback;
            ethernet_driver_dyn_menu[i].data     = (ui_callback_data_t)pname;
        }
        ethernet_driver_dyn_menu[i].action   = ACTION_NONE;
        ethernet_driver_dyn_menu[i].string   = NULL;
        ethernet_driver_dyn_menu[i].type     = 0;
        ethernet_driver_dyn_menu[i].callback = NULL;
        ethernet_driver_dyn_menu[i].data     = NULL;
    }

    return submenu_radio_callback(activated, param);
}

/* Common menus */

static UI_MENU_CALLBACK(show_ETHERNET_DISABLED_callback)
{
    int value;

    resources_get_int("ETHERNET_DISABLED", &value);

    return value ? "(disabled)" : NULL;
}

const ui_menu_entry_t ethernet_menu[] = {
    {   .string   = "Ethernet support",
        .type     = MENU_ENTRY_TEXT,
        .callback = show_ETHERNET_DISABLED_callback,
        .data     = (ui_callback_data_t)1
    },
    {   .string   = "Driver",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = ETHERNET_DRIVER_dynmenu_callback,
        .data     = (ui_callback_data_t)ethernet_driver_dyn_menu
    },
    {   .string   = "Interface",
        .type     = MENU_ENTRY_DYNAMIC_SUBMENU,
        .callback = ETHERNET_INTERFACE_dynmenu_callback,
        .data     = (ui_callback_data_t)ethernet_interface_dyn_menu
    },
    SDL_MENU_LIST_END
};

#endif /* HAVE_RAWNET */
