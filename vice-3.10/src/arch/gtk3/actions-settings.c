/** \file   actions-settings.c
 * \brief   UI action implementations for settings management
 *
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
#include <stddef.h>
#include <stdbool.h>

#include "archdep.h"
#include "basedialogs.h"
#include "lastdir.h"
#include "lib.h"
#include "mainlock.h"
#include "resources.h"
#include "types.h"
#include "ui.h"
#include "uiactions.h"
#include "uihotkeys.h"
#include "uisettings.h"
#include "util.h"

#include "actions-settings.h"


static char *last_dir = NULL;
static char *last_file = NULL;


/** \brief  Generate full path and name of the current vice config file
 *
 * \return  heap-allocated path to the current config file, free with lib_free()
 */
static char *get_config_file_path(void)
{
    char *path = NULL;

    if (vice_config_file != NULL) {
        /* -config used */

        /* check for relative path */
        if (archdep_path_is_relative(vice_config_file)) {
            char *cwd = g_get_current_dir();

            path = util_join_paths(cwd, vice_config_file, NULL);
            g_free(cwd);
        } else {
            path = lib_strdup(vice_config_file);
        }
    }
    return path;
}

/** \brief  Callback for the confirmation dialog to restore settings
 *
 * Restore settings to their factory values if \a result is TRUE.
 *
 * \param[in]   dialog  confirm-dialog reference
 * \param[in]   result  result
 */
static void restore_default_callback(GtkDialog *dialog, gboolean result)
{
    if (result) {
        mainlock_obtain();
        ui_hotkeys_load_vice_default();
        resources_set_defaults();
        mainlock_release();
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_SETTINGS_DEFAULT);
}

/** \brief  Show dialog to restore settings to default
 *
 * \param[in]   self    action map
 */
static void settings_default_action(ui_action_map_t *self)
{
    vice_gtk3_message_confirm(NULL, /* current emulator window as parent */
                              restore_default_callback,
                              "Reset all settings to default",
                              "Are you sure you wish to reset all settings to"
                              " their default  values?\n\n"
                              "The new settings will not be saved until using"
                              " the 'Save settings' menu item, or having"
                              " 'Save on exit' enabled and exiting VICE.");
}

/** \brief  Show settings dialog
 *
 * \param[in]   self    action map
 */
static void settings_dialog_action(ui_action_map_t *self)
{
    ui_settings_dialog_show(NULL);
}

/** \brief  Reload settings from current settings file
 *
 * \param[in]   self    action map
 */
static void settings_load_action(ui_action_map_t *self)
{
    int result;

    mainlock_obtain();
    result = resources_reset_and_load(NULL);
    mainlock_release();
    if (result != 0) {
        vice_gtk3_message_error(NULL, /* current emu window as parent */
                                "VICE core error",
                                "Failed to load default settings file");
    } else {
        /* set window geometries from resources, if present */
        ui_set_window_geometries();
    }
    ui_action_finish(ACTION_SETTINGS_LOAD);
}


/** \brief  Callback for the load-settings dialog
 *
 * \param[in]   dialog      dialog
 * \param[in]   filename    filename
 * \param[in]   data        mode (0: reset and load, 1: add extra settings)
 */
static void settings_load_filename_callback(GtkDialog *dialog,
                                            gchar     *filename,
                                            gpointer   data)
{
    if (filename!= NULL) {
        int result;

        mainlock_obtain();
        result = (data == NULL) ? resources_reset_and_load(filename)
                                : resources_load(filename);
        mainlock_release();
        if (result != 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "VICE core error",
                                    "Failed to load settings from '%s'",
                                    filename);
        } else {
            /* set window geometries from resources, if present */
            ui_set_window_geometries();
        }

        lastdir_update(GTK_WIDGET(dialog), &last_dir, &last_file);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_SETTINGS_LOAD_FROM);
}


/** \brief  Action to pop up the load from (extra) settings dialog
 *
 * \param[in]   self    action map
 */
static void settings_load_from_action(ui_action_map_t *self)
{
    GtkWidget *dialog;
    char      *path;

    dialog = vice_gtk3_open_file_dialog("Load settings file",
                                        NULL, NULL, NULL,
                                        settings_load_filename_callback,
                                        self->data);
    path = get_config_file_path();
    if (path != NULL) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), path);
        lib_free(path);
    } else if (last_dir == NULL) {
        /* use config file dir */
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            archdep_user_config_path());
    } else {
        lastdir_set(dialog, &last_dir, &last_file);
    }
    gtk_widget_show_all(dialog);
}

/** \brief  Save current settings
 *
 * \param[in]   self    action map
 */
static void settings_save_action(ui_action_map_t *self)
{
    int result;

    mainlock_obtain();
    result = resources_save(NULL);
    mainlock_release();
    if (result != 0) {
        vice_gtk3_message_error(NULL, /* current emu window as parent */
                                "VICE core error",
                                "Failed to save default settings file");
    }
    ui_action_finish(self->action);
}

/** \brief  Callback for the save-to settings dialog
 *
 * \param[in,out]   dialog      save-file dialog
 * \param[in,out]   filename    filename
 * \param[in]       unused      extra data (unused)
 */
static void on_settings_save_to_filename(GtkDialog *dialog,
                                         gchar     *filename,
                                         gpointer   unused)
{
    if (filename!= NULL) {
        mainlock_obtain();
        if (resources_save(filename) != 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "VICE core error",
                                    "Failed to save settings as '%s'",
                                    filename);
        }
        mainlock_release();
        lastdir_update(GTK_WIDGET(dialog), &last_dir, &last_file);
        g_free(filename);
    }
    mainlock_release();
    gtk_widget_destroy(GTK_WIDGET(dialog));
    mainlock_obtain();
    ui_action_finish(ACTION_SETTINGS_SAVE_TO);
}

/** \brief  Pop up a dialog to save current settings to a file
 *
 * \param[in]   self    action map
 */
static void settings_save_to_action(ui_action_map_t *self)
{
    GtkWidget *dialog;
    char      *path;

    dialog = vice_gtk3_save_file_dialog("Save settings as ...",
                                        NULL, TRUE, NULL,
                                        on_settings_save_to_filename,
                                        NULL);
    path = get_config_file_path();
    if (path != NULL) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), path);
        lib_free(path);
    } else if (last_dir == NULL) {
        /* use config file dir */
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            archdep_user_config_path());
    } else {
        lastdir_set(dialog, &last_dir, &last_file);
    }
    gtk_widget_show_all(dialog);
}


/** \brief  List of actions for settings management */
static const ui_action_map_t settings_actions[] = {
    {
        .action  = ACTION_SETTINGS_DEFAULT,
        .handler = settings_default_action
    },
    {
        .action  = ACTION_SETTINGS_DIALOG,
        .handler = settings_dialog_action,
        .blocks  = true,
        .dialog  = true
    },
    {
        .action  = ACTION_SETTINGS_LOAD,
        .handler = settings_load_action,
        .blocks  = true
    },
    {
        .action  = ACTION_SETTINGS_LOAD_FROM,
        .handler = settings_load_from_action,
        .data    = vice_int_to_ptr(0),  /* reset settings before loading */
        .blocks  = true,
        .dialog  = true
    },
    {
        .action  = ACTION_SETTINGS_LOAD_EXTRA,
        .handler = settings_load_from_action,
        .data    = vice_int_to_ptr(1),  /* don't reset setting before loading */
        .blocks  = true,
        .dialog  = true
    },
    {
        .action  = ACTION_SETTINGS_SAVE,
        .handler = settings_save_action,
        .blocks  = true
    },
    {
        .action  = ACTION_SETTINGS_SAVE_TO,
        .handler = settings_save_to_action,
        .blocks  = true,
        .dialog  = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register settings-related actions */
void actions_settings_register(void)
{
    ui_actions_register(settings_actions);
}

/** \brief  Free the last used directory/filename
 *
 * Should be called on emu shutdown.
 */
void actions_settings_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
