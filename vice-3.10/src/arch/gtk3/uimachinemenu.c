/** \file   uimachinemenu.c
 * \brief   Native GTK3 menus for machine emulators (not vsid)
 *
 * \author  Marcus Sutton <loggedoubt@gmail.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stddef.h>

#include "archdep.h"
#include "debug.h"
#include "machine.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"

#include "uimachinemenu.h"

/*
 * The following are translation unit local so we can create functions that
 * modify menu contents or even functions that alter the top bar itself.
 */

/** \brief  Main menu bar widget
 *
 * Contains the submenus on the menu main bar
 *
 * This one lives until ui_exit() or thereabouts
 */
static GtkWidget *main_menu_bar = NULL;


/* {{{ disk_detach_submenu[] */
/** \brief  File->Detach disk submenu
 */
static const ui_menu_item_t disk_detach_submenu[] = {
    {   .label  = "Drive 8:0",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DRIVE_DETACH_8_0,
    },
    {   .label  = "Drive 8:1",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DRIVE_DETACH_8_1
    },
    {   .label  = "Drive 9:0",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DRIVE_DETACH_9_0
    },
    {   .label  = "Drive 9:1",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DRIVE_DETACH_9_1
    },
    {   .label  = "Drive 10:0",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DRIVE_DETACH_10_0
    },
    {   .label  = "Drive 10:1",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DRIVE_DETACH_10_1
    },
    {   .label   = "Drive 11:0",
        .type    = UI_MENU_TYPE_ITEM_ACTION,
        .action  = ACTION_DRIVE_DETACH_11_0
    },
    {   .label   = "Drive 11:1",
        .type    = UI_MENU_TYPE_ITEM_ACTION,
        .action  = ACTION_DRIVE_DETACH_11_1
    },
    {   .label   = "Detach all",
        .type    = UI_MENU_TYPE_ITEM_ACTION,
        .action  = ACTION_DRIVE_DETACH_ALL
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ disk_attach_submenu[] */
/** \brief  File->Attach disk submenu
 */
static const ui_menu_item_t disk_attach_submenu[] = {
    {   .label    = "Drive #8",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DRIVE_ATTACH_8_0,
        .unlocked = true
    },
    {   .label    = "Drive #9",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DRIVE_ATTACH_9_0,
        .unlocked = true
    },
    {   .label    = "Drive #10",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DRIVE_ATTACH_10_0,
        .unlocked = true
    },
    {   .label    = "Drive #11",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DRIVE_ATTACH_11_0,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ disk_fliplist_submenu[] */
/** \brief  File->Flip list submenu
 */
static const ui_menu_item_t disk_fliplist_submenu[] = {
    {   .label    = "Add current image (unit #8, drive 0)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_ADD_8_0
    },
    {   .label    = "Remove current image (unit #8, drive 0)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_REMOVE_8_0
    },
    {   .label    = "Attach next image (unit #8, drive 0)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_NEXT_8_0
    },
    {   .label    = "Attach previous image (unit #8, drive 0)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_PREVIOUS_8_0
    },
    {   .label    = "Clear fliplist (unit #8, drive 0)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_CLEAR_8_0
    },
    {   .label    = "Load flip list file for unit #8, drive 0...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_LOAD_8_0,
        .unlocked = true
    },
    {   .label    = "Save flip list file of unit #8, drive 0...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_FLIPLIST_SAVE_8_0,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ datasette_1_control_submenu[] */
/** \brief  File->Datasette control submenu for port #1
 */
static const ui_menu_item_t datasette_1_control_submenu[] = {
    {   .label  = "Stop",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_STOP_1
    },
    {   .label  = "Play",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_PLAY_1
    },
    {   .label  = "Forward",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_FFWD_1
    },
    {   .label  = "Rewind",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_REWIND_1
    },
    {   .label  = "Record",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_RECORD_1
    },
    {   .label  = "Reset",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_RESET_1
    },
    {   .label  = "Reset Counter",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_RESET_COUNTER_1
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ datasette_2_control_submenu[] */
/** \brief  File->Datasette control submenu for port #2
 */
static const ui_menu_item_t datasette_2_control_submenu[] = {
    {   .label  = "Stop",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_STOP_1
    },
    {   .label  = "Play",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_PLAY_1
    },
    {   .label  = "Forward",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_FFWD_1
    },
    {   .label  = "Rewind",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_REWIND_1
    },
    {   .label  = "Record",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_RECORD_1
    },
    {   .label  = "Reset",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_RESET_1
    },
    {   .label  = "Reset Counter",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_TAPE_RESET_COUNTER_1
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ reset_submenu[] */
/** \brief  File->Reset submenu
 */
static const ui_menu_item_t reset_submenu[] = {
    {   .label  = "Reset machine CPU",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_MACHINE_RESET_CPU
    },
    {   .label  = "Power cycle machine",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_MACHINE_POWER_CYCLE
    },
    UI_MENU_SEPARATOR,

    {   .label  = "Reset drive #8",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_RESET_DRIVE_8
    },
    {   .label  = "Reset drive #9",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_RESET_DRIVE_9
    },
    {   .label  = "Reset drive #10",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_RESET_DRIVE_10
    },
    {   .label  = "Reset drive #11",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_RESET_DRIVE_11
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_head[] */
/** \brief  'File' menu - head section
 */
static const ui_menu_item_t file_menu_head[] = {
    {   .label    = "Smart attach...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SMART_ATTACH,
        .unlocked = true
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Attach disk image",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = disk_attach_submenu
    },
    {   .label    = "Create and attach an empty disk image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DRIVE_CREATE,
        .unlocked = true
    },
    {   .label    = "Detach disk image",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = disk_detach_submenu
    },
    {   .label    = "Flip list",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = disk_fliplist_submenu
    },
    UI_MENU_SEPARATOR,

    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_tape[] */
/** \brief  'File' menu - tape section
 */
static const ui_menu_item_t file_menu_tape[] = {
    {   .label    = "Attach datasette image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_ATTACH_1,
        .unlocked = true
    },
    {   .label    = "Create and attach datasette image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_CREATE_1,
        .unlocked = true
    },
    {   .label    = "Detach datasette image",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_DETACH_1
    },
    {   .label    = "Datasette controls",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = datasette_1_control_submenu
    },
    UI_MENU_SEPARATOR,  /* Required since this menu gets inserted between
                           disk menu items and cartridge items on emulators
                           that have a datasette port. */
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_tape_xpet[] */
/** \brief  'File' menu - tape section for xpet
 */
static const ui_menu_item_t file_menu_tape_xpet[] = {
    {   .label    = "Attach datasette #1 image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_ATTACH_1,
        .unlocked = true
    },
    {   .label    = "Create and attach datasette #1 image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_CREATE_1,
        .unlocked = true
    },
    {   .label    = "Detach datasette #1 image",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_DETACH_1
    },
    {   .label    = "Datasette #1 controls",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = datasette_1_control_submenu
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Attach datasette #2 image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_ATTACH_2,
        .unlocked = true
    },
    {   .label    = "Create and attach datasette #2 image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_CREATE_2,
        .unlocked = true
    },
    {   .label    = "Detach datasette #2 image",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_TAPE_DETACH_2
    },
    {   .label    = "Datasette #2 controls",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = datasette_2_control_submenu
    },
    UI_MENU_SEPARATOR,  /* Required since this menu gets inserted between
                           disk menu items and cartridge items on emulators
                           that have a datasette port. */
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_cart_freeze */
/** \brief  'File' menu - cartridge section for C64/C128
 *
 * C64, SCPU64, C128, VIC20 and Plus4 containing "Cartridge freeze".
 */
static const ui_menu_item_t file_menu_cart_freeze[] = {
    {   .label    = "Attach cartridge image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_CART_ATTACH,
        .unlocked = true
    },
    {   .label    = "Detach cartridge image(s)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_CART_DETACH
    },
    {   .label    = "Cartridge freeze",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_CART_FREEZE
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_cart_no_freeze */
/** \brief  'File' menu - cartridge section for Plus/4, VIC-20 and CBM-II
 *
 * CBM-II, not containing "Cartridge freeze".
 */
static const ui_menu_item_t file_menu_cart_no_freeze[] = {
    {   .label    = "Attach cartridge image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_CART_ATTACH,
        .unlocked = true
    },
    {   .label    = "Detach cartridge image(s)",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_CART_DETACH
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ printer_submenu */
/** \brief  'File' menu - printer submenu (with userport printer)
 *
 * C64, C64SC, SCPU64, C128, VIC20, PET, CBM6x0.
 */
static const ui_menu_item_t printer_submenu[] = {
    {   .label  = "Send formfeed to printer #4",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_4
    },
    {   .label  = "Send formfeed to printer #5",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_5
    },
    {   .label  = "Send formfeed to plotter #6",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_6
    },
    {   .label  = "Send formfeed to userport printer",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_USERPORT
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_printer_no_userport */
/** \brief  'File' menu - printer submenu (without userport printer)
 *
 * C64DTV, PLUS4, CBM5x0.
 */
static const ui_menu_item_t printer_submenu_no_userport[] = {
    {   .label  = "Send formfeed to printer #4",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_4
    },
    {   .label  = "Send formfeed to printer #5",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_5
    },
    {   .label  = "Send formfeed to plotter #6",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_PRINTER_FORMFEED_6
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ file_menu_print */
/** \brief  'File' menu - printer section (with userport printer)
 *
 * C64, C64SC, SCPU64, C128, VIC20, PET, CBM6x0.
 */
static const ui_menu_item_t file_menu_printer[] = {
    {   .label   = "Printer/plotter",
        .type    = UI_MENU_TYPE_SUBMENU,
        .submenu = printer_submenu
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR,
};
/* }}} */

/* {{{ file_menu_printer_no_userport */
/** \brief  'File' menu - printer section (without userport printer)
 *
 * C64DTV, PLUS4, CBM5x0.
 */
static const ui_menu_item_t file_menu_printer_no_userport[] = {
    {   .label   = "Printer/plotter",
        .type    = UI_MENU_TYPE_SUBMENU,
        .submenu = printer_submenu_no_userport
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR,
};
/* }}} */

/* {{{ file_menu_tail */
/** \brief  'File' menu - tail section
 */
static const ui_menu_item_t file_menu_tail[] = {
    /* monitor */
    {   .label   = "Activate monitor",
        .type    = UI_MENU_TYPE_ITEM_ACTION,
        .action  = ACTION_MONITOR_OPEN,
    },
    UI_MENU_SEPARATOR,

    {   .label   = "Reset",
        .type    = UI_MENU_TYPE_SUBMENU,
        .submenu = reset_submenu
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Exit emulator",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_QUIT,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ edit_menu[] */
/** \brief  'Edit' menu
 */
static const ui_menu_item_t edit_menu[] = {
    {   .label  = "Copy",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_EDIT_COPY
    },
    {   .label  = "Paste",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_EDIT_PASTE
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ snapshot_menu[] */
/** \brief  'Snapshot' menu
 */
static ui_menu_item_t snapshot_menu[] = {
    {   .label    = "Load snapshot image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SNAPSHOT_LOAD
    },
    {   .label    = "Save snapshot image...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SNAPSHOT_SAVE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Quickload snapshot",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SNAPSHOT_QUICKLOAD
    },
    {   .label    = "Quicksave snapshot",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SNAPSHOT_QUICKSAVE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Start recording events",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HISTORY_RECORD_START
    },
    {   .label    = "Stop recording events",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HISTORY_RECORD_STOP
    },
    {   .label    = "Start playing back events",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HISTORY_PLAYBACK_START
    },
    {   .label    = "Stop playing back events",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HISTORY_PLAYBACK_STOP
    },
    {   .label    = "Set recording milestone",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HISTORY_MILESTONE_SET
    },
    {   .label    = "Return to milestone",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HISTORY_MILESTONE_RESET
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Save/Record media...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_MEDIA_RECORD,
        .unlocked = true
    },
    {   .label    = "Stop media recording",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_MEDIA_STOP
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Quicksave screenshot",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SCREENSHOT_QUICKSAVE
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ speed_submenu[] */
/** \brief  Index in the speed submenu for the "$MACHINE_NAME FPS" item
 *
 * Bit of a hack since the menu item labels are reused for all emus and we can't
 * dynamically set them via some printf()-ish construct
 */
#define MACHINE_FPS_INDEX   7

/** \brief  Settings -> Speed submenu */
static const ui_menu_item_t speed_submenu[] = {
    {   .label    = "200% CPU",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_CPU_200
    },
    {   .label    = "100% CPU",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_CPU_100
    },
    {   .label    = "50% CPU",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_CPU_50
    },
    {   .label    = "25% CPU",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_CPU_25
    },
    {   .label    = "10% CPU",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_CPU_10
    },
    {   .label    = "Custom CPU speed...",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_CPU_CUSTOM,
        .activate = true
    },
    UI_MENU_SEPARATOR,

    {   .label    = "MACHINE_NAME FPS",   /* gets replaced during runtime */
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_FPS_REAL
    },
    {   .label    = "50 FPS",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_FPS_50
    },
    {   .label    = "60 FPS",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_FPS_60
    },
    {   .label    = "Custom FPS",
        .type     = UI_MENU_TYPE_ITEM_RADIO_INT,
        .action   = ACTION_SPEED_FPS_CUSTOM,
        .activate = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_head[] */
/** \brief  Settings menu - head section */
static const ui_menu_item_t settings_menu_head[] = {
    {   .label    = "Fullscreen",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   =  ACTION_FULLSCREEN_TOGGLE,
        .unlocked = true
    },
    {   .label    = "Restore display state",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_RESTORE_DISPLAY,
        .unlocked = true
    },
    {   .label    = "Show menu/status in fullscreen",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_FULLSCREEN_DECORATIONS_TOGGLE,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_speed[] */
/** \brief  Settings menu - speed section */
static const ui_menu_item_t settings_menu_speed[] = {
    {   .label    = "Warp mode",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_WARP_MODE_TOGGLE
    },
    {   .label    = "Pause emulation",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_PAUSE_TOGGLE
    },
    {   .label    = "Advance frame",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_ADVANCE_FRAME,
        .unlocked = false
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Emulation speed",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = speed_submenu
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_statusbar_primary[] */
/** \brief  Settings menu - show statusbar (primary) item */
static const ui_menu_item_t settings_menu_statusbar_primary[] = {
    {   .action    = ACTION_SHOW_STATUSBAR_TOGGLE,
        .label     = "Show status bar",
        .type      = UI_MENU_TYPE_ITEM_CHECK,
        .unlocked  = false
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_statusbar_secondary[] */
/** \brief  Settings menu - show statusbar (secondary) item */
static const ui_menu_item_t settings_menu_statusbar_secondary[] = {
    {   .action    = ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE,
        .label     = "Show status bar",
        .type      = UI_MENU_TYPE_ITEM_CHECK
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_mouse[] */
/** \brief  Settings menu - mouse items */
static const ui_menu_item_t settings_menu_mouse[] = {
    {   .action   = ACTION_MOUSE_GRAB_TOGGLE,
        .label    = "Mouse grab",
        .type     = UI_MENU_TYPE_ITEM_CHECK
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_joy_swap[] */

/** \brief  Settings menu - joystick controlport swap
 *
 * Valid for x64/x64sc/x64dtv/xscpu64/x128/xplus4/xcbm5x0
 */
static const ui_menu_item_t settings_menu_joy_swap[] = {
    {   .label  = "Swap joysticks",
        .type   = UI_MENU_TYPE_ITEM_CHECK,
        .action = ACTION_SWAP_CONTROLPORT_TOGGLE
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_non_vsid[] */
/** \brief  'Settings' menu section before the tail section
 *
 * Only valid for non-VSID
 */
static const ui_menu_item_t settings_menu_non_vsid[] = {
    /* Needs to go here to avoid duplicate action names */
    {   .label  = "Allow keyset joysticks",
        .type   = UI_MENU_TYPE_ITEM_CHECK,
        .action = ACTION_KEYSET_JOYSTICK_TOGGLE
    },
    UI_MENU_SEPARATOR,
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ settings_menu_tail[] */
/** \brief  'Settings' menu tail section
 */
static const ui_menu_item_t settings_menu_tail[] = {
   /* the settings dialog */
    {   .label    = "Settings...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_DIALOG,
        .unlocked = true
    },
    {   .label    = "Load settings",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_LOAD
    },
    {   .label    = "Load settings from...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_LOAD_FROM,
        .unlocked = true
    },
    {   .label    = "Load extra settings from...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_LOAD_EXTRA,
        .unlocked = true
    },
    {   .label    = "Save settings",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_SAVE
    },
    {   .label    = "Save settings to...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_SAVE_TO,
        .unlocked = true
    },
    {   .label    = "Restore default settings",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_SETTINGS_DEFAULT,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */

#ifdef DEBUG
/* {{{ debug_menu[] */
/** \brief  'Debug' menu items for emu's except x64dtv
 */
static const ui_menu_item_t debug_menu[] = {
    {   .label    = "Trace mode...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DEBUG_TRACE_MODE,
        .unlocked = true
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Main CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_CPU_TOGGLE,
        .unlocked = false
    },
    UI_MENU_SEPARATOR,

    {   .label    = "IEC bus trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_IEC_TOGGLE
    },
    {   .label    = "IEEE-488 bus trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_IEEE488_TOGGLE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Drive #8 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE
    },
    {   .label    = "Drive #9 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE
    },
    {   .label    = "Drive #10 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE
    },
    {   .label    = "Drive #11 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Autoplay playback frames...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
        .unlocked = true
    },
    {   .label    = "Save core dump",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_CORE_DUMP_TOGGLE
    },
    UI_MENU_TERMINATOR
};
/* }}} */

/* {{{ debug_menu_c64dtv[] */
/** \brief  'Debug' menu items for x64dtv
 */
static const ui_menu_item_t debug_menu_c64dtv[] = {
    {   .label    = "Trace mode...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DEBUG_TRACE_MODE,
        .unlocked = true
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Main CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_CPU_TOGGLE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "IEC bus trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_IEC_TOGGLE
    },
    {   .label    = "Drive #8 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_8_TOGGLE
    },
    {   .label    = "Drive #9 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_9_TOGGLE
    },
    {   .label    = "Drive #10 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_10_TOGGLE
    },
    {   .label    = "Drive #11 CPU trace",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_TRACE_DRIVE_11_TOGGLE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Blitter log",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_BLITTER_LOG_TOGGLE
    },
    {   .label    = "DMA log",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_DMA_LOG_TOGGLE
    },
    {   .label    = "Flash log",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_FLASH_LOG_TOGGLE
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Autoplay playback frames...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_DEBUG_AUTOPLAYBACK_FRAMES,
        .unlocked = true
    },
    {   .label    = "Save core dump",
        .type     = UI_MENU_TYPE_ITEM_CHECK,
        .action   = ACTION_DEBUG_CORE_DUMP_TOGGLE
    },
    UI_MENU_TERMINATOR
};
/* }}} */
#endif

/* {{{ help_menu[] */
/** \brief  'Help' menu items
 */
static const ui_menu_item_t help_menu[] = {
    {   .label    = "Browse manual...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HELP_MANUAL,
        .unlocked = true
    },
    {   .label    = "Command line options...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HELP_COMMAND_LINE,
        .unlocked = true
    },
    {   .label    = "Compile time features...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HELP_COMPILE_TIME,
        .unlocked = true
    },
    {   .label    = "Hotkeys...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HELP_HOTKEYS,
        .unlocked = true
    },
    {   .label    = "About VICE...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_HELP_ABOUT,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};
/* }}} */


/** \brief  'File' menu - tape section pointer
 *
 * Set by ui_machine_menu_bar_create().
 */
static const ui_menu_item_t *file_menu_tape_section = NULL;

/** \brief  'File' menu - cart section pointer
 *
 * Set by ui_machine_menu_bar_create().
 */
static const ui_menu_item_t *file_menu_cart_section = NULL;

/** \brief  'File' menu - printer section pointer
 *
 * Set by ui_machine_menu_bar_create().
 */
static const ui_menu_item_t *file_menu_printer_section = NULL;

/** \brief  'Settings' menu - joystick section pointer
 *
 * Set by ui_machine_menu_bar_create().
 */
static const ui_menu_item_t *settings_menu_joy_section = NULL;


/** \brief  Create the top menu bar with standard submenus
 *
 * \param[in]   window_id   window ID (PRIMARY_WINDOW or SECONDARY_WINDOW)
 *
 * \return  GtkMenuBar
 */
GtkWidget *ui_machine_menu_bar_create(gint window_id)
{
    GtkWidget *menu_bar;
    GtkWidget *file_submenu;
    GtkWidget *snapshot_submenu;
    GtkWidget *edit_submenu;
    GtkWidget *settings_submenu;
#ifdef DEBUG
    GtkWidget *debug_submenu;
#endif
    GtkWidget *help_submenu;


    /* create the top menu bar */
    menu_bar = gtk_menu_bar_new();

    /* create the top-level 'File' menu */
    file_submenu = ui_menu_submenu_create(menu_bar, "File");

    /* create the top-level 'Edit' menu */
    edit_submenu = ui_menu_submenu_create(menu_bar, "Edit");

    /* create the top-level 'Snapshot' menu */
    snapshot_submenu = ui_menu_submenu_create(menu_bar, "Snapshot");

    /* create the top-level 'Settings' menu */
    settings_submenu = ui_menu_submenu_create(menu_bar, "Preferences");

#ifdef DEBUG
    /* create the top-level 'Debug' menu (when --enable-debug is used) */
    debug_submenu = ui_menu_submenu_create(menu_bar, "Debug");
#endif

    /* create the top-level 'Help' menu */
    help_submenu = ui_menu_submenu_create(menu_bar, "Help");

    /* determine which joystick swap, tape and cart menu items should be added */
    switch (machine_class) {

        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:
            file_menu_tape_section    = file_menu_tape;
            file_menu_cart_section    = file_menu_cart_freeze;
            file_menu_printer_section = file_menu_printer;
            settings_menu_joy_section = settings_menu_joy_swap;
            break;

        case VICE_MACHINE_C64DTV:
            /* no cart section */
            /* no tape section */
            file_menu_printer_section = file_menu_printer_no_userport;
            settings_menu_joy_section = settings_menu_joy_swap;
            break;

        case VICE_MACHINE_SCPU64:
            /* no tape section */
            file_menu_cart_section    = file_menu_cart_freeze;
            file_menu_printer_section = file_menu_printer;
            settings_menu_joy_section = settings_menu_joy_swap;
            break;

        case VICE_MACHINE_C128:
            file_menu_tape_section    = file_menu_tape;
            file_menu_cart_section    = file_menu_cart_freeze;
            file_menu_printer_section = file_menu_printer;
            settings_menu_joy_section = settings_menu_joy_swap;
            break;

        case VICE_MACHINE_VIC20:
            file_menu_tape_section    = file_menu_tape;
            file_menu_cart_section    = file_menu_cart_freeze;
            file_menu_printer_section = file_menu_printer;
            break;

        case VICE_MACHINE_PLUS4:
            file_menu_tape_section    = file_menu_tape;
            file_menu_cart_section    = file_menu_cart_freeze;
            file_menu_printer_section = file_menu_printer_no_userport;
            settings_menu_joy_section = settings_menu_joy_swap;
            break;

        case VICE_MACHINE_CBM5x0:
            file_menu_tape_section    = file_menu_tape;
            file_menu_cart_section    = file_menu_cart_no_freeze;
            file_menu_printer_section = file_menu_printer_no_userport;
            settings_menu_joy_section = settings_menu_joy_swap;
            break;

        case VICE_MACHINE_CBM6x0:
            file_menu_tape_section    = file_menu_tape;
            file_menu_cart_section    = file_menu_cart_no_freeze;
            file_menu_printer_section = file_menu_printer;
            break;

        case VICE_MACHINE_PET:
            file_menu_tape_section    = file_menu_tape_xpet;
            /* no cart section */
            file_menu_printer_section = file_menu_printer;
            break;

        case VICE_MACHINE_VSID:
            archdep_vice_exit(1);
            break;
        default:
            break;
    }

    /* add items to the File menu */
    ui_menu_add(file_submenu, file_menu_head, window_id);
    if (file_menu_tape_section != NULL) {
        ui_menu_add(file_submenu, file_menu_tape_section, window_id);
    }
    if (file_menu_cart_section != NULL) {
        ui_menu_add(file_submenu, file_menu_cart_section, window_id);
    }
    if (file_menu_printer_section != NULL) {
        ui_menu_add(file_submenu, file_menu_printer_section, window_id);
    }
    ui_menu_add(file_submenu, file_menu_tail, window_id);

    /* add items to the Edit menu */
    ui_menu_add(edit_submenu, edit_menu, window_id);
    /* add items to the Snapshot menu */
    ui_menu_add(snapshot_submenu, snapshot_menu, window_id);

    /* add items to the Settings menu */
    ui_menu_add(settings_submenu, settings_menu_head, window_id);
    /* different UI actions for "show-statusbar-toggle" */
    if (window_id == PRIMARY_WINDOW) {
        ui_menu_add(settings_submenu, settings_menu_statusbar_primary, window_id);
    } else {
        ui_menu_add(settings_submenu, settings_menu_statusbar_secondary, window_id);
    }
    ui_menu_add(settings_submenu, settings_menu_speed, window_id);
    ui_menu_add(settings_submenu, settings_menu_mouse, window_id);
    if (settings_menu_joy_section != NULL) {
        ui_menu_add(settings_submenu, settings_menu_joy_section, window_id);
    }
    if (machine_class != VICE_MACHINE_VSID) {
        ui_menu_add(settings_submenu, settings_menu_non_vsid, window_id);
    }
    ui_menu_add(settings_submenu, settings_menu_tail, window_id);

#ifdef DEBUG
    /* add items to the Debug menu */
    if (machine_class == VICE_MACHINE_C64DTV) {
        ui_menu_add(debug_submenu, debug_menu_c64dtv, window_id);
    } else {
        ui_menu_add(debug_submenu, debug_menu, window_id);
    }
#endif

    /* add items to the Help menu */
    ui_menu_add(help_submenu, help_menu, window_id);

    main_menu_bar = menu_bar;    /* XXX: do I need g_object_ref()/g_object_unref()
                                         for this */

    return menu_bar;
}


/** \brief  Add missing settings load/save items
 *
 * \param[in,out]   menu        GtkMenu
 * \param[in]       window_id   window ID (currently only 0/PRIMARY_WINDOW)
 */
void ui_machine_menu_bar_vsid_patch(GtkWidget *menu, gint window_id)
{
    ui_menu_add(menu, settings_menu_tail, window_id);
}
