/*
 * menu_userport.c - Userport menu for SDL UI.
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


#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "types.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"
#include "userport.h"

#include "menu_userport.h"
#ifdef HAVE_LIBCURL
#include "userport_wic64.h"
#include "menu_wic64.h"
#endif

UI_MENU_DEFINE_RADIO(UserportDevice)

static ui_menu_entry_t userport_dyn_menu[USERPORT_MAX_DEVICES + 1];

static int userport_dyn_menu_init = 0;

static void sdl_menu_userport_free(void)
{
    int i;

    for (i = 0; userport_dyn_menu[i].string != NULL; i++) {
        lib_free(userport_dyn_menu[i].string);
    }
}

static UI_MENU_CALLBACK(UserportDevice_dynmenu_callback)
{
    userport_desc_t *devices = userport_get_valid_devices(1);
    int i;

    /* rebuild menu if it already exists. */
    if (userport_dyn_menu_init != 0) {
        sdl_menu_userport_free();
    } else {
        userport_dyn_menu_init = 1;
    }

    for (i = 0; devices[i].name; ++i) {
        userport_dyn_menu[i].action   = ACTION_NONE;
        userport_dyn_menu[i].string   = lib_strdup(devices[i].name);
        userport_dyn_menu[i].type     = MENU_ENTRY_RESOURCE_RADIO;
        userport_dyn_menu[i].callback = radio_UserportDevice_callback;
        userport_dyn_menu[i].data     = (ui_callback_data_t)vice_int_to_ptr(devices[i].id);
    }
    userport_dyn_menu[i].string = NULL;

    lib_free(devices);

    return MENU_SUBMENU_STRING;
}

#if defined(USE_MPG123) && defined(HAVE_GLOB_H)
#include "uifilereq.h"
#include "resources.h"
static UI_MENU_CALLBACK(set_directory_callback)
{
    char *name;
    const char *res;

    res = (const char *)param;
    if (activated) {
        name = sdl_ui_file_selection_dialog("Select FunMP3 directory", FILEREQ_MODE_CHOOSE_DIR);
        if (name != NULL) {
            resources_set_string(res, name);
            lib_free(name);
        }
    }
    return NULL;
}
#endif

ui_menu_entry_t userport_menu[7];

UI_MENU_DEFINE_TOGGLE(UserportRTCDS1307Save)
UI_MENU_DEFINE_TOGGLE(UserportRTC58321aSave)

void uiuserport_menu_create(int rtc)
{
    int j = 0;

    if (rtc) {
        userport_menu[j].action   = ACTION_NONE;
        userport_menu[j].string   = "Save DS1307 RTC data when changed";
        userport_menu[j].type     = MENU_ENTRY_RESOURCE_TOGGLE;
        userport_menu[j].callback = toggle_UserportRTCDS1307Save_callback;
        userport_menu[j].data     = NULL;
        j++;

        userport_menu[j].action   = ACTION_NONE;
        userport_menu[j].string   = "Save 58321a RTC data when changed";
        userport_menu[j].type     = MENU_ENTRY_RESOURCE_TOGGLE;
        userport_menu[j].callback = toggle_UserportRTC58321aSave_callback;
        userport_menu[j].data     = NULL;
        j++;
    }

#if defined(USE_MPG123) && defined(HAVE_GLOB_H)
    if (machine_class == VICE_MACHINE_C64 ||
        machine_class == VICE_MACHINE_C64SC ||
        machine_class == VICE_MACHINE_C128 ||
        machine_class == VICE_MACHINE_VIC20 ||
        machine_class == VICE_MACHINE_PET ||
        machine_class == VICE_MACHINE_SCPU64) {

        userport_menu[j].action   = ACTION_NONE;
        userport_menu[j].string   = "FunMP3 Directory";
        userport_menu[j].type     = MENU_ENTRY_DIALOG;
        userport_menu[j].callback = set_directory_callback;
        userport_menu[j].data     = (ui_callback_data_t) "FunMP3Dir";
        j++;
    }
#endif

#ifdef HAVE_LIBCURL
    if (machine_class == VICE_MACHINE_C64 ||
        machine_class == VICE_MACHINE_C64SC ||
        machine_class == VICE_MACHINE_C128 ||
        machine_class == VICE_MACHINE_VIC20 ||
        machine_class == VICE_MACHINE_SCPU64) {

        userport_menu[j].action   = ACTION_NONE;
        userport_menu[j].string   = "WiC64 Settings";
        userport_menu[j].type     = MENU_ENTRY_SUBMENU;
        userport_menu[j].callback = submenu_callback;
        userport_menu[j].data     = uiwic64_menu_create();
        j++;
    }
#endif

    userport_menu[j].action   = ACTION_NONE;
    userport_menu[j].string   = "";
    userport_menu[j].type     = MENU_ENTRY_TEXT;
    userport_menu[j].callback = seperator_callback;
    userport_menu[j].data     = NULL;
    j++;

    userport_menu[j].action   = ACTION_NONE;
    userport_menu[j].string   = "Userport devices";
    userport_menu[j].type     = MENU_ENTRY_DYNAMIC_SUBMENU;
    userport_menu[j].callback = UserportDevice_dynmenu_callback;
    userport_menu[j].data     = (ui_callback_data_t)userport_dyn_menu;
    j++;

    userport_menu[j].string = NULL;
}


/** \brief  Clean up memory used by the dynamically created userport menus
 */
void uiuserport_menu_shutdown(void)
{
    if (userport_dyn_menu_init) {
        sdl_menu_userport_free();
    }
#ifdef HAVE_LIBCURL
    wic64_timezones_menu_free();
#endif
}
