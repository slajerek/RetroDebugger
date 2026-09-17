/** \file   uisnapshot.c
 * \brief   Snapshot dialogs and menu item handlers
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
#include <stdlib.h>
#include <stdbool.h>

#include "lib.h"
#include "util.h"
#include "archdep.h"
#include "basewidgets.h"
#include "widgethelpers.h"
#include "debug_gtk3.h"
#include "machine.h"
#include "mainlock.h"
#include "resources.h"
#include "filechooserhelpers.h"
#include "openfiledialog.h"
#include "savefiledialog.h"
#include "selectdirectorydialog.h"
#include "basedialogs.h"
#include "interrupt.h"
#include "vsync.h"
#include "vsyncapi.h"
#include "snapshot.h"
#include "vice-event.h"
#include "uistatusbar.h"
#include "ui.h"
#include "uiactions.h"
#include "uiapi.h"

#include "uisnapshot.h"


/*****************************************************************************
 *                              Helper functions                             *
 ****************************************************************************/


/** \brief  Create a string in the format 'yyyymmddHHMMss' of the current time
 *
 * \return  string owned by GLib, free with g_free()
 */
static gchar *create_datetime_string(void)
{
    GDateTime *d;
    gchar *s;

    d = g_date_time_new_now_local();
    s = g_date_time_format(d, "%Y%m%d%H%M%S");
    g_date_time_unref(d);
    return s;
}


/** \brief  Construct filename for quickload/quicksave snapshots
 *
 * \return  filename for the quickload/save file
 *
 * \note    free after use with lib_free()
 */
static char *quicksnap_filename(void)
{
    char filename[4096];

    /* construct the filename */
    g_snprintf(filename, sizeof(filename), "%s.vsf", machine_get_name());

    return util_join_paths(archdep_user_config_path(), filename, NULL);
}


/** \brief  Create a filename based on the current datetime
 *
 * \return  heap-allocated string, owned by VICE, free with lib_free()
 */
static char *create_proposed_snapshot_name(void)
{
    char *date;
    char *filename;

    date = create_datetime_string();
    filename = lib_msprintf("vice-snapshot-%s.vsf", date);
    g_free(date);
    return filename;
}



/** \brief  Show dialog to save a snapshot
 *
 * FIXME:   Why isn't this dialog non-blocking?
 */
static void save_snapshot_dialog(void)
{
    GtkWidget *dialog;
    GtkWidget *extra;
    GtkWidget *roms_widget;
    GtkWidget *disks_widget;
    gint response_id;
    int save_roms;
    int save_disks;

    dialog = gtk_file_chooser_dialog_new("Save snapshot file",
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "Save", GTK_RESPONSE_ACCEPT,
            "Cancel", GTK_RESPONSE_CANCEL,
            NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),
            create_file_chooser_filter(file_chooser_filter_snapshot, FALSE));

    /* set proposed filename */
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                                      create_proposed_snapshot_name());

    /* create extras widget */
    extra = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(extra), 16);

    disks_widget = gtk_check_button_new_with_label("Save attached disks");
    roms_widget = gtk_check_button_new_with_label("Save attached ROMs");
    gtk_grid_attach(GTK_GRID(extra), disks_widget, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(extra), roms_widget, 1, 0, 1, 1);
    gtk_widget_show_all(extra);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), extra);
    response_id = gtk_dialog_run(GTK_DIALOG(dialog));
    save_roms = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(roms_widget));
    save_disks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(disks_widget));

    if (response_id == GTK_RESPONSE_ACCEPT) {
        gchar *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename != NULL) {
            char *fname_copy;
            char buffer[1024];  /* I guess this was meant to be used to display
                                   a message on the statusbar? */

            fname_copy = util_add_extension_const(filename, "vsf");

            if (machine_write_snapshot(fname_copy, save_roms, save_disks, 0) < 0) {
                snapshot_display_error();
                g_snprintf(buffer, 1024, "Failed to save snapshot '%s'",
                        fname_copy);
            } else {
                g_snprintf(buffer, 1024, "Saved snapshot '%s'", fname_copy);
            }
            lib_free(fname_copy);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
    ui_action_finish(ACTION_SNAPSHOT_SAVE);
}


/*****************************************************************************
 *                              CPU trap handlers                            *
 ****************************************************************************/

/** \brief  Flag indicating the UI is 'done', whatever that means
 */
static bool ui_done;


/** \brief  file-open dialog callback for "Load snapshot file"
 *
 * \param[in,out]   dialog      file-open dialog
 * \param[in,out]   filename    filename
 * \param[in]       data        extra callback data (unused)
 */
static void load_snapshot_filename_callback(GtkDialog *dialog,
                                            gchar *filename,
                                            gpointer data)
{
    if (filename != NULL) {
        /* load snapshot */
        if (machine_read_snapshot(filename, 0) < 0) {
            snapshot_display_error();
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_done = true;
    ui_action_finish(ACTION_SNAPSHOT_LOAD);
}


/** \brief  load-snapshot handler
 *
 * \param[in]   user_data   extra callback data (unused)
 *
 * \return  FALSE
 */
static gboolean load_snapshot_trap_impl(gpointer user_data)
{
    const char *filters[] = { "*.vsf", NULL };

    vice_gtk3_open_file_dialog(
            "Open snapshot file",
            "Snapshot files", filters, NULL,
            load_snapshot_filename_callback,
            NULL);
    /* FIXME: shouldn't this return TRUE? */
    return FALSE;
}

/** \brief  CPU trap handler for the load snapshot dialog
 *
 * \param[in]   addr    memory address (unused)
 * \param[in]   data    unused
 */
static void load_snapshot_trap(uint16_t addr, void *data)
{
    vsync_suspend_speed_eval();
    sound_suspend();

    /*
     * We need to use the main thread to do UI stuff. And we
     * also need to block the VICE thread until we get the
     * user decision.
     */
    ui_done = false;
    gdk_threads_add_timeout(0, load_snapshot_trap_impl, NULL);

    /* block until the operation is done */
    while (!ui_done) {
        mainlock_yield_and_sleep(tick_per_second() / 60);
    }
}

/****/


/** \brief  save-snapshot handler
 *
 * \param[in]   user_data   extra event data (unused)
 *
 * \return  FALSE
 */
static gboolean save_snapshot_trap_impl(gpointer user_data)
{
    save_snapshot_dialog();

    ui_done = true;

    return FALSE;
}


/** \brief  CPU trap handler to trigger the Save dialog
 *
 * \param[in]   addr    memory address (unused)
 * \param[in]   data    unused
 */
static void save_snapshot_trap(uint16_t addr, void *data)
{
    vsync_suspend_speed_eval();
    sound_suspend();

    /*
     * We need to use the main thread to do UI stuff. And we
     * also need to block the VICE thread until we get the
     * user decision.
     */
    ui_done = false;
    gdk_threads_add_timeout(0, save_snapshot_trap_impl, NULL);

    /* block until the operation is done */
    while (!ui_done) {
        mainlock_yield_and_sleep(tick_per_second() / 60);
    }
}


/** \brief  CPU trap handler for the QuickLoad snapshot menu item
 *
 * \param[in]   addr    memory address (unused)
 * \param[in]   data    quickload snapshot filename
 */
static void quickload_snapshot_trap(uint16_t addr, void *data)
{
    char *filename = (char *)data;

    vsync_suspend_speed_eval();
    sound_suspend();

    if (machine_read_snapshot(filename, 0) < 0) {
        snapshot_display_error();
    }
    lib_free(filename);
}


/** \brief  CPU trap handler for the QuickSave snapshot menu item
 *
 * \param[in]   addr    memory address (unused)
 * \param[in]   data    quicksave snapshot filename
 */
static void quicksave_snapshot_trap(uint16_t addr, void *data)
{
    char *filename = (char *)data;

    vsync_suspend_speed_eval();
    sound_suspend();

    if (machine_write_snapshot(filename, TRUE, TRUE, 0) < 0) {
        snapshot_display_error();
    }
    lib_free(filename);
}


/*****************************************************************************
 *                              Public functions                             *
 ****************************************************************************/


/** \brief  Display UI to load a snapshot file
 */
void ui_snapshot_load_snapshot(void)
{
    if (!ui_pause_active()) {
        interrupt_maincpu_trigger_trap(load_snapshot_trap, NULL);
    } else {
        load_snapshot_trap_impl(NULL);
    }
}


/** \brief  Display UI to save a snapshot file
 */
void ui_snapshot_save_snapshot(void)
{
    if (!ui_pause_active()) {
        interrupt_maincpu_trigger_trap(save_snapshot_trap, NULL);
    } else {
        save_snapshot_trap_impl(NULL);
    }
}


/** \brief  Quicksave snapshot */
void ui_snapshot_quickload_snapshot(void)
{
    char *fname = quicksnap_filename();

    interrupt_maincpu_trigger_trap(quickload_snapshot_trap, (void *)fname);
}


/** \brief  Quicksave snapshot */
void ui_snapshot_quicksave_snapshot(void)
{
    char *fname = quicksnap_filename();

    interrupt_maincpu_trigger_trap(quicksave_snapshot_trap, (void *)fname);
}
