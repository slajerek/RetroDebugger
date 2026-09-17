/*
 * menu_reset.c - Implementation of the reset settings menu for the SDL UI.
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

#include "drive.h"
#include "machine.h"
#include "menu_common.h"
#include "uiactions.h"
#include "uimenu.h"
#include "vsync.h"


ui_menu_entry_t reset_menu[] = {
    /* reset machine */
    {
        .action    = ACTION_MACHINE_RESET_CPU,
        .string    = "Reset machine CPU",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_MACHINE_POWER_CYCLE,
        .string    = "Power cycle machine",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },

    SDL_MENU_ITEM_SEPARATOR,

    /* reset drive */
    {
        .action    = ACTION_RESET_DRIVE_8,
        .string    = "Drive 8",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_9,
        .string    = "Drive 9",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_10,
        .string    = "Drive 10",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_11,
        .string    = "Drive 11",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },

/* options below this are enabled/disabled by uidrive_menu_create() drive_menu.c */
    {
        .action    = ACTION_RESET_DRIVE_8_CONFIG,
        .string    = "Drive 8  (in configuration mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_9_CONFIG,
        .string    = "Drive 9  (in configuration mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_10_CONFIG,
        .string    = "Drive 10 (in configuration mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_11_CONFIG,
        .string    = "Drive 11 (in configuration mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },

    {
        .action    = ACTION_RESET_DRIVE_8_INSTALL,
        .string    = "Drive 8  (in installation mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_9_INSTALL,
        .string    = "Drive 9  (in installation mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_10_INSTALL,
        .string    = "Drive 10 (in installation mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },
    {
        .action    = ACTION_RESET_DRIVE_11_INSTALL,
        .string    = "Drive 11 (in installation mode)",
        .type      = MENU_ENTRY_OTHER,
        .activated = MENU_EXIT_UI_STRING
    },

    SDL_MENU_LIST_END
};
