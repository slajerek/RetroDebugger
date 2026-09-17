/** \file   vsidplaylistadddialog.c
 * \brief   GTK3 Add SID files dialog for the playlist widget
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

#include "archdep_get_hvsc_dir.h"
#include "vice_gtk3.h"
#include "debug_gtk3.h"
#include "filechooserhelpers.h"
#include "lastdir.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"

#include "vsidplaylistadddialog.h"


/** \brief  callback function of the dialog
 */
static void (*dialog_cb)(GSList *) = NULL;

/** \brief  Last used directory of the dialog
 *
 * Used by the functions in lastdir.c
 */
static gchar *last_used_dir = NULL;

/** \brief  Last used file of the dialog
 *
 * Used by the functions in lastdir.c
 */
static gchar *last_used_file = NULL;


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   widget  dialog (unused)
 * \param[in]   data    extra event data (unused)
 *
 * XXX: Currently does nothing. Not sure what I meant to do with this --compyx
 */
static void on_destroy(GtkWidget *widget, gpointer data)
{
    /* NOP */
}


/** \brief  Handler for the 'response' event of the dialog
 *
 * Calls the registered callback with a list of files.
 *
 * \param[in]   dialog      dialog triggering the event
 * \param[in]   response_id ID of response of the \a dialog
 * \param[in]   data        extra event data (unused)
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    if (response_id == GTK_RESPONSE_ACCEPT) {
        GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        char *first = files->data;
        gchar *dir = g_path_get_dirname(first);

        lastdir_update_raw(dir, &last_used_dir, first, &last_used_file);
        g_free(dir);    /* lastdir_update makes a copy */
        dialog_cb(files);
        g_slist_free(files);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_PSID_PLAYLIST_ADD);
}

/** \brief  Create GtkFileChooser instance to open SID files
 *
 * \return  GtkFileChooser
 */
static GtkWidget *vsid_playlist_add_dialog_create(void)
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new(
            "Open SID file",
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Open", GTK_RESPONSE_ACCEPT,
            NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    /*
     * If this the first time adding a SID and HVSCRoot is set (or the env var
     * HVSC_BASE) use that for the default directory.
     *
     * Unfortunately attaching SIDs via the main menu and this playlist each use
     * their own `last_dir`, so perhaps I should merge them, or perhaps even
     * remove the main menu item once the playlist works properly.
     */
    if (last_used_dir == NULL) {
        const char *hvsc_root = archdep_get_hvsc_dir();

        if (hvsc_root != NULL && *hvsc_root != '\0') {
            /*
             * The last_dir.c code uses GLib memory management, so use
             * g_strdup() here and not lib_strdup(). I did, and it produced
             * a nice segfault, and I actually wrote the lastdir code ;)
             */
            last_used_dir = g_strdup(hvsc_root);
        }
    }
    lastdir_set(dialog, &last_used_dir, &last_used_file);

    g_signal_connect(dialog, "response", G_CALLBACK(on_response), NULL);
    g_signal_connect_unlocked(dialog, "destroy", G_CALLBACK(on_destroy), NULL);
    return dialog;
}


/** \brief  Run dialog to add SID files to the playlist
 *
 * \param[in]   callback    function accepting a list of selected files
 */
void vsid_playlist_add_dialog_exec(void (*callback)(GSList *files))
{
    GtkWidget *dialog = vsid_playlist_add_dialog_create();
    dialog_cb = callback;

    gtk_widget_show(dialog);
}


/** \brief  Free memory used by the lastdir.c functions
 *
 * Frees the `last_used_dir` and `last_used_file` strings.
 *
 * Called on emulator shutdown.
 */
void vsid_playlist_add_dialog_free(void)
{
    lastdir_shutdown(&last_used_dir, &last_used_file);
}
