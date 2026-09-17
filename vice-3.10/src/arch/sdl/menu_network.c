/*
 * menu_network.c - Network menu for SDL UI.
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

#ifdef HAVE_NETWORK

#include <stdio.h>

#include "menu_common.h"
#include "network.h"
#include "resources.h"
#include "types.h"
#include "ui.h"
#include "uimenu.h"

#include "menu_network.h"


UI_MENU_DEFINE_STRING(NetworkServerName)
UI_MENU_DEFINE_STRING(NetworkServerBindAddress)
UI_MENU_DEFINE_INT(NetworkServerPort)

static UI_MENU_CALLBACK(custom_network_control_callback)
{
    int value;
    int flag = vice_ptr_to_int(param);

    resources_get_int("NetworkControl", &value);

    if (activated) {
        value ^= flag;
        resources_set_int("NetworkControl", value);
    } else {
        if (value & flag) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}

static const ui_menu_entry_t network_control_menu[] = {
    {   .string   = "Server keyboard",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)NETWORK_CONTROL_KEYB
    },
    {   .string   = "Server joystick 1",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)NETWORK_CONTROL_JOY1
    },
    {   .string   = "Server joystick 2",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)NETWORK_CONTROL_JOY2
    },
    {   .string   = "Server devices",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)NETWORK_CONTROL_DEVC
    },
    {   .string   = "Server settings",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)NETWORK_CONTROL_RSRC
    },
    {   .string   = "Client keyboard",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)(NETWORK_CONTROL_KEYB << NETWORK_CONTROL_CLIENTOFFSET)
    },
    {   .string   = "Client joystick 1",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)(NETWORK_CONTROL_JOY1 << NETWORK_CONTROL_CLIENTOFFSET)
    },
    {   .string   = "Client joystick 2",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)(NETWORK_CONTROL_JOY2 << NETWORK_CONTROL_CLIENTOFFSET)
    },
    {   .string   = "Client devices",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)(NETWORK_CONTROL_DEVC << NETWORK_CONTROL_CLIENTOFFSET)
    },
    {   .string   = "Client settings",
        .type     = MENU_ENTRY_OTHER_TOGGLE,
        .callback = custom_network_control_callback,
        .data     = (ui_callback_data_t)(NETWORK_CONTROL_RSRC << NETWORK_CONTROL_CLIENTOFFSET)
    },
    SDL_MENU_LIST_END
};

static void update_network_menu(void);

static UI_MENU_CALLBACK(custom_connect_client_callback)
{
    if (activated) {
        network_disconnect();
        if (network_connect_client() < 0) {
            ui_error("Couldn't connect client.");
        } else {
            update_network_menu();
            return sdl_menu_text_exit_ui;
        }
    }
    update_network_menu();
    return NULL;
}

static UI_MENU_CALLBACK(custom_start_server_callback)
{
    if (activated) {
        network_disconnect();
        if (network_start_server() < 0) {
            ui_error("Couldn't start netplay server.");
        } else {
            update_network_menu();
            return sdl_menu_text_exit_ui;
        }
    }
    update_network_menu();
    return NULL;
}

static UI_MENU_CALLBACK(custom_disconnect_callback)
{
    if (activated) {
        network_disconnect();
    }
    update_network_menu();
    return NULL;
}

#define OFFS_SERVER_PORT    1
#define OFFS_SERVER_ADDR    3
#define OFFS_START_SERVER   4
#define OFFS_REMOTE_ADDR    6
#define OFFS_START_CLIENT   7
#define OFFS_DISCONNECT     9
#define OFFS_CONTROL        11

ui_menu_entry_t network_menu[] = {
    SDL_MENU_ITEM_TITLE("Netplay"),
/* 1*/
    {   .string   = "Netplay port",
        .type     = MENU_ENTRY_RESOURCE_INT,
        .callback = int_NetworkServerPort_callback,
        .data     = (ui_callback_data_t)"Set network server port"
    },
    SDL_MENU_ITEM_SEPARATOR,

/* 3*/
    {   .string   = "Server address",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_NetworkServerBindAddress_callback,
        .data     = (ui_callback_data_t)"Set network server bind address"
    },
/* 4*/
    {   .string   = "Start server",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_start_server_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

/* 6*/
    {   .string   = "Remote Server",
        .type     = MENU_ENTRY_RESOURCE_STRING,
        .callback = string_NetworkServerName_callback,
        .data     = (ui_callback_data_t)"Set remote server address"
    },
/* 7*/
    {   .string   = "Connect client",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_connect_client_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

/* 9*/
    {   .string   = "Disconnect",
        .type     = MENU_ENTRY_OTHER,
        .callback = custom_disconnect_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

/*11*/
    {   .string   = "Control Settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)network_control_menu
    },
    SDL_MENU_LIST_END
};

static void update_network_menu(void)
{
    int mode = network_get_mode();
    network_menu[OFFS_START_SERVER].status = (mode == NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    network_menu[OFFS_START_CLIENT].status = (mode == NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    network_menu[OFFS_DISCONNECT].status = (mode != NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;

    network_menu[OFFS_SERVER_PORT].status = (mode == NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    network_menu[OFFS_SERVER_ADDR].status = (mode == NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    network_menu[OFFS_REMOTE_ADDR].status = (mode == NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
    network_menu[OFFS_CONTROL].status = (mode == NETWORK_IDLE) ? MENU_STATUS_ACTIVE : MENU_STATUS_INACTIVE;
}

#endif
