/*
 * menu_wic64.c - WiC64 menu for SDL UI.
 *
 * Written by
 *
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
#ifdef HAVE_LIBCURL

#include <stdio.h>

#include "lib.h"
#include "menu_common.h"
#include "types.h"
#include "uiactions.h"
#include "uimenu.h"
#include "ui.h"
#include "userport.h"

#include "log.h"
#include "menu_wic64.h"
#include "userport_wic64.h"

static ui_menu_entry_t *dyn_menu_wic64;

UI_MENU_DEFINE_TOGGLE(WIC64Logenabled)
UI_MENU_DEFINE_INT(WIC64Hexdumplines);
UI_MENU_DEFINE_INT(WIC64RemoteTimeout);
UI_MENU_DEFINE_INT(WIC64LogLevel);
UI_MENU_DEFINE_STRING(WIC64DefaultServer)
UI_MENU_DEFINE_TOGGLE(WIC64Resetuser)
UI_MENU_DEFINE_RADIO(WIC64Timezone);
UI_MENU_DEFINE_STRING(WIC64SecToken)
UI_MENU_DEFINE_STRING(WIC64IPAddress)
UI_MENU_DEFINE_STRING(WIC64MACAddress)
UI_MENU_DEFINE_TOGGLE(WIC64DHCP)

/** \brief  Generate WiC64 runtime timezones menu
 *
 * Iterate WiC64 timezones table and create radio buttons for each zone.
 */
void wic64_timezones_menu_new(void)
{
    const tzones_t *zones;
    size_t          num_zones;
    size_t          z;

    zones          = userport_wic64_get_timezones(&num_zones);
    dyn_menu_wic64 = lib_malloc((num_zones + 1u) * sizeof *dyn_menu_wic64);

    for (z = 0; z < num_zones; z++) {
        dyn_menu_wic64[z].action   = ACTION_NONE;
        dyn_menu_wic64[z].string   = lib_msprintf("%d: %s", zones[z].idx, zones[z].tz_name);
        dyn_menu_wic64[z].type     = MENU_ENTRY_RESOURCE_RADIO;
        dyn_menu_wic64[z].callback = radio_WIC64Timezone_callback;
        dyn_menu_wic64[z].data     = vice_int_to_ptr(zones[z].idx);
    }
    dyn_menu_wic64[z].string = NULL;
}

/** \brief  Free WiC64 runtime timezones menu */
void wic64_timezones_menu_free(void)
{
    int z;

    for (z = 0; dyn_menu_wic64 && (dyn_menu_wic64[z].string != NULL); z++) {
        lib_free(dyn_menu_wic64[z].string);
    }
    lib_free(dyn_menu_wic64);
    dyn_menu_wic64 = NULL;
}

static UI_MENU_CALLBACK(custom_wic64_reset_callback)
{
    if (activated) {
        userport_wic64_factory_reset();
        ui_message("WiC64 has been reset to factory default.");
    }
    return NULL;
}

#define MENTRIES 16
ui_menu_entry_t wic64_menu[MENTRIES];

ui_callback_data_t uiwic64_menu_create(void)
{
    int j = 0;
    static char tl[64];

    if (dyn_menu_wic64 == NULL) {
        wic64_timezones_menu_new();
    }

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "";
    wic64_menu[j].type     = MENU_ENTRY_TEXT;
    wic64_menu[j].callback = seperator_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "WiC64 Settings";
    wic64_menu[j].type     = MENU_ENTRY_TEXT;
    wic64_menu[j].callback = seperator_callback;
    wic64_menu[j].data     = (ui_callback_data_t)1;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Remote Timeout";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_INT;
    wic64_menu[j].callback = int_WIC64RemoteTimeout_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Default server:";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_STRING;
    wic64_menu[j].callback = string_WIC64DefaultServer_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Timezone";
    wic64_menu[j].type     = MENU_ENTRY_DYNAMIC_SUBMENU;
    wic64_menu[j].callback = submenu_radio_callback;
    wic64_menu[j].data     = dyn_menu_wic64;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "DHCP (set IP automatically)";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_TOGGLE;
    wic64_menu[j].callback = toggle_WIC64DHCP_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "IP Address:";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_STRING;
    wic64_menu[j].callback = string_WIC64IPAddress_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "MAC Address:";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_STRING;
    wic64_menu[j].callback = string_WIC64MACAddress_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Security Token:";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_STRING;
    wic64_menu[j].callback = string_WIC64SecToken_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Reset User when resetting WiC64";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_TOGGLE;
    wic64_menu[j].callback = toggle_WIC64Resetuser_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Reset WiC64 to factory default";
    wic64_menu[j].type     = MENU_ENTRY_OTHER;
    wic64_menu[j].callback = custom_wic64_reset_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "WiC64 tracing";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_TOGGLE;
    wic64_menu[j].callback = toggle_WIC64Logenabled_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    wic64_menu[j].string   = "Hexdump lines (0=unlimited):";
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_INT;
    wic64_menu[j].callback = int_WIC64Hexdumplines_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].action   = ACTION_NONE;
    sprintf(tl, "Trace level (0..%d, 0: off)", WIC64_MAXTRACELEVEL);
    wic64_menu[j].string   = tl;
    wic64_menu[j].type     = MENU_ENTRY_RESOURCE_INT;
    wic64_menu[j].callback = int_WIC64LogLevel_callback;
    wic64_menu[j].data     = NULL;
    j++;

    wic64_menu[j].string = NULL;
    if (j >= MENTRIES) {
        log_error(LOG_DEFAULT, "internal error: %s, %d >= MENTRIES(%d)", __FUNCTION__, j, MENTRIES);
    }
    return (ui_callback_data_t) wic64_menu;
}

#endif  /* HAVE_LIBCURL */
