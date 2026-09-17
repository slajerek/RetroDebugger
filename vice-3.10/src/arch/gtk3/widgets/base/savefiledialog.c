/** \file   savefiledialog.c
 * \brief   GtkFileChooser wrapper to save/create a file
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
 */

#include "vice.h"

#include <gtk/gtk.h>

#include "debug_gtk3.h"
#include "filechooserhelpers.h"
#include "ui.h"
#include "mainlock.h"

#include "savefiledialog.h"


/** \brief  Callback for the 'response' event
 */
static void (*filename_cb)(GtkDialog *, char *, gpointer);


/** \brief  Handler for the dialog's 'response' event
 *
 * \param[in,out]   dialog      save-file dialog
 * \param[in]       response_id response ID
 * \param[in]       data        extra data to pass to the user callback
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    gchar *filename;

    switch (response_id) {
        case GTK_RESPONSE_ACCEPT:
            filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename != NULL) {
                filename_cb(dialog, filename, data);
            }
            break;
        default:
            filename_cb(dialog, NULL, data);
            break;
    }
}


/** \brief  Create a 'save file' dialog
 *
 * \param[in]   title       dialog title
 * \param[in]   proposed    proposed file name (optional)
 * \param[in]   confirm     confirm overwriting an existing file
 * \param[in]   path        set starting directory (optional)
 * \param[in]   callback    function to call on dialog response
 * \param[in]   param       extra data to pass to callback (optional)
 *
 * \return  new dialog
 *
 * \note    the filename returned is allocated by GLib and needs to be freed
 *          after use with g_free()
 */
GtkWidget *vice_gtk3_save_file_dialog(
        const char *title,
        const char *proposed,
        gboolean confirm,
        const char *path,
        void (*callback)(GtkDialog *, gchar *, gpointer),
        gpointer param)
{
    GtkWidget *dialog;

    filename_cb = callback;

    mainlock_assert_is_not_vice_thread();

    dialog = gtk_file_chooser_dialog_new(
            title,
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "Save", GTK_RESPONSE_ACCEPT,
            "Cancel", GTK_RESPONSE_REJECT,
            NULL);

    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), ui_get_active_window());

    /* set overwrite confirmation */
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
            confirm);

    /* set proposed file name, if any */
    if (proposed != NULL && *proposed != '\0') {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), proposed);
    }

    /* change directory if specified */
    if (path != NULL && *path != '\0') {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
    }

    g_signal_connect(dialog, "response", G_CALLBACK(on_response), param);
    gtk_widget_show(dialog);
    return dialog;
}
