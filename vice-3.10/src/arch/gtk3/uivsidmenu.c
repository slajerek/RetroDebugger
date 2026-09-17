/** \file   uivsidmenu.c
 * \brief   Native GTK3 menus for the SID player, vsid
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

#include "debug.h"
#include "debug_gtk3.h"
#include "machine.h"
#include "psid.h"
#include "uiactions.h"
#include "uimachinemenu.h"
#include "uimenu.h"

#include "uivsidmenu.h"


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


/** \brief  File submenu
 */
static GtkWidget *file_submenu = NULL;

/** \brief  Playlist submenu
 */
static GtkWidget *playlist_submenu = NULL;


/** \brief  Tune submenu
 */
static GtkWidget *tune_submenu = NULL;


/** \brief  Settings submenu
 */
static GtkWidget *settings_submenu = NULL;

#ifdef DEBUG
/** \brief  Debug submenu, only available when --enable-debug was specified
 */
static GtkWidget *debug_submenu = NULL;
#endif

/** \brief  Help submenu
 */
static GtkWidget *help_submenu = NULL;

/** \brief  List of items in the tune submenu
 */
static GSList *tune_submenu_group = NULL;


/** \brief  File->Reset submenu
 */
static const ui_menu_item_t reset_submenu[] = {
    {   .label    = "Reset machine CPU",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_MACHINE_RESET_CPU,
        .unlocked = true
    },
    {   .label    = "Power cycle machine",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_MACHINE_POWER_CYCLE,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};

/** \brief  'File' menu
 */
static const ui_menu_item_t file_menu[] = {
    {   .label    = "Load PSID file...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_LOAD,
        .unlocked = true
    },
    UI_MENU_SEPARATOR,
#if 0
    /* XXX: this item might need its own dialog that only
     *      contains sound recording options
     */
    { "Record sound file ...", UI_MENU_TYPE_ITEM_ACTION,
      ACTION_MEDIA_RECORD,
      NULL, false },

    { "Stop sound recording", UI_MENU_TYPE_ITEM_ACTION,
      ACTION_MEDIA_STOP,
      NULL, false },

    UI_MENU_SEPARATOR,
#endif
    /* monitor */
    {   .label    = "Activate monitor",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_MONITOR_OPEN
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Reset",
        .type     = UI_MENU_TYPE_SUBMENU,
        .submenu  = reset_submenu
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Exit player",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_QUIT,
        .unlocked = true
    },
    UI_MENU_TERMINATOR
};


/** \brief  'Tune' menu
 */
#if 0
static ui_menu_item_t tune_menu[] = {

    UI_MENU_TERMINATOR
};
#endif

/** \brief  Playlist menu items */
static const ui_menu_item_t playlist_menu[] = {
    {   .label    = "Load playlist...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_LOAD,
        .unlocked = true
    },
    {   .label    = "Save playlist...",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_SAVE,
        .unlocked = true
    },
    {   .label    = "Clear playlist",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_CLEAR,
        .unlocked = true
    },
    UI_MENU_SEPARATOR,

    {   .label    = "Play first tune",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_FIRST
    },
    {   .label    = "Play previous tune",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_PREVIOUS
    },
    {   .label    = "Play next tune",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_NEXT
    },
    {   .label    = "Play last tune",
        .type     = UI_MENU_TYPE_ITEM_ACTION,
        .action   = ACTION_PSID_PLAYLIST_LAST
    },
    UI_MENU_TERMINATOR
};

/** \brief  'Settings' menu
 */
static const ui_menu_item_t settings_menu[] = {
    /* XXX: this item should perhaps be removed and its functionality
     *      added to the settings dialog
     */
    {   .label  = "Override PSID settings",
        .type   = UI_MENU_TYPE_ITEM_CHECK,
        .action = ACTION_PSID_OVERRIDE_TOGGLE
    },
    UI_MENU_TERMINATOR
};

/** \brief  'Debug' menu items
 */
#ifdef DEBUG
static const ui_menu_item_t debug_menu[] = {
    {   .label  = "Trace mode...",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DEBUG_TRACE_MODE
    },
    UI_MENU_SEPARATOR,

    {   .label  = "Main CPU trace",
        .type   = UI_MENU_TYPE_ITEM_CHECK,
        .action = ACTION_DEBUG_TRACE_CPU_TOGGLE
    },
    UI_MENU_SEPARATOR,

    {   .label  = "Autoplay playback frames...",
        .type   = UI_MENU_TYPE_ITEM_ACTION,
        .action = ACTION_DEBUG_AUTOPLAYBACK_FRAMES
    },
    {   .label  = "Save core dump",
        .type   = UI_MENU_TYPE_ITEM_CHECK,
        .action = ACTION_DEBUG_CORE_DUMP_TOGGLE
    },
    UI_MENU_TERMINATOR
};
#endif

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


/** \brief  Play a tune when selected with the Tune menu
 *
 * \param[in,out]   menuitem    menu item
 * \param[in]       user_data   tune index
 */
static void select_tune_from_menu(GtkMenuItem *menuitem,
                                  gpointer     user_data)
{
    int tune;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {
        return;
    }
    tune = GPOINTER_TO_INT(user_data);
    psid_init_driver();
    machine_play_psid(tune);
    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
}


/** \brief  Remove each of the old menu items before adding the new ones
 *
 * \param[in,out]   widget  menu item widget
 * \param[in]       data    extra data (unused)
 */
static void remove_item_from_menu (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(widget);
}


/** \brief  Create the tune menu when a new PSID is loaded
 *
 * \param[in]   count   number of items to remove from the old menu
 */
void ui_vsid_tune_menu_set_tune_count(int count)
{
    GtkWidget *item = NULL;
    int i;

    if (tune_submenu == NULL || !GTK_IS_CONTAINER(tune_submenu)) {
        debug_gtk3("tune_submenu invalid.");
        return;
    }

    gtk_container_foreach(GTK_CONTAINER(tune_submenu), remove_item_from_menu, NULL);
    for (i = count; i > 0; i--) {
        gchar buf[256];

        g_snprintf(buf, sizeof(buf), "Tune %s%d", i < 10 ? "_" : "", i);
        item = gtk_radio_menu_item_new_with_mnemonic_from_widget (GTK_RADIO_MENU_ITEM(item), buf);
        gtk_widget_show(item);
        g_signal_connect(item,
                         "activate",
                         G_CALLBACK(select_tune_from_menu),
                         GINT_TO_POINTER(i));
        gtk_menu_shell_prepend(GTK_MENU_SHELL(tune_submenu), item);
    }
    tune_submenu_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
}


/** \brief  Ensure the correct menu element is selected when the current tune is changed by anything other than the menu
 *
 * \param[in]   count   number of menu items
 */
void ui_vsid_tune_set_tune_current(int count)
{
    if (tune_submenu_group != NULL) {
        gpointer nth_item = g_slist_nth_data(tune_submenu_group, (guint)count - 1);
        if (nth_item != NULL) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(nth_item), TRUE);
        }
    }
}


/** \brief  Create the top menu bar with standard submenus
 *
 * \return  GtkMenuBar
 */
GtkWidget *ui_vsid_menu_bar_create(void)
{
    GtkWidget *menu_bar;
    gint window_id = 0;

    /* create the top menu bar */
    menu_bar = gtk_menu_bar_new();

    /* create the top-level 'File' menu */
    file_submenu = ui_menu_submenu_create(menu_bar, "File");

    /* create the top-level 'Playlist' menu */
    playlist_submenu = ui_menu_submenu_create(menu_bar, "Playlist");
#if 0
    /* create the top-level 'Tune' menu */
    tune_submenu = ui_menu_submenu_create(menu_bar, "Tune");
#endif
    /* create the top-level 'Settings' menu */
    settings_submenu = ui_menu_submenu_create(menu_bar, "Settings");

#ifdef DEBUG
    /* create the top-level 'Debug' menu (when --enable-debug is used) */
    debug_submenu = ui_menu_submenu_create(menu_bar, "Debug");
#endif

    /* create the top-level 'Help' menu */
    help_submenu = ui_menu_submenu_create(menu_bar, "Help");


    /* add items to the File menu */
    ui_menu_add(file_submenu, file_menu, window_id);
    /* add items to the Playlist menu */
    ui_menu_add(playlist_submenu, playlist_menu, window_id);

#if 0
    /* TODO: add items to the Tune menu */
    ui_menu_add(tune_submenu, tune_menu);
#endif

    /* add items to the Settings menu */
    ui_menu_add(settings_submenu, settings_menu, window_id);
    /* bit of a hack: add load/save */
    ui_machine_menu_bar_vsid_patch(settings_submenu, window_id);

#ifdef DEBUG
    /* add items to the Debug menu */
    ui_menu_add(debug_submenu, debug_menu, window_id);
#endif

    /* add items to the Help menu */
    ui_menu_add(help_submenu, help_menu, window_id);

    main_menu_bar = menu_bar;    /* XXX: do I need g_object_ref()/g_object_unref()
                                         for this */
    return menu_bar;
}
