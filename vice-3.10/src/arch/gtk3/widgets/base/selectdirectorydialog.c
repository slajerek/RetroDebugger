/** \file   selectdirectorydialog.c
 * \brief   GtkFileChooser wrapper to select/create a directory
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

#include "filechooserhelpers.h"
#include "mainlock.h"
#include "ui.h"

#include "selectdirectorydialog.h"


/** \brief  Function to call in the response hander
 *
 * This function is called to pass the result back to the user.
 *
 * Three values are passed: the dialog, the resulting filename (or NULL when
 * the user selected 'Cancel') and an optional gpointer that was passed in
 * the constructor.
 */
static void (*filename_cb)(GtkDialog *, gchar *, gpointer);


/** \brief  Handler for 'response' event of the directory-select dialog
 *
 * \param[in]   dialog      directory-select dialog
 * \param[in]   response_id response ID
 * \param[in]   data        optional data of callback
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    if (response_id == GTK_RESPONSE_ACCEPT) {
        gchar *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        filename_cb(dialog, filename, data);
    } else {
        /* signal cancel by passing NULL as filename */
        filename_cb(dialog, NULL, data);
    }
}


/** \brief  Create a 'select directory' dialog
 *
 * The \a callback is expected to accept three arguments: the dialog, the
 * filename returned from the dialog (NULL indicates the user canceled the
 * dialog) and an optional extra argument passed here as \a param.
 *
 * \param[in]   title           dialog title
 * \param[in]   proposed        proposed directory name (optional)
 * \param[in]   allow_create    allow creating a new directory
 * \param[in]   path            set starting directory (optional)
 * \param[in]   callback        user function to call on response
 * \param[in]   param           optional extra param for the callback
 *
 * \return  dialog
 */
GtkWidget *vice_gtk3_select_directory_dialog(const char  *title,
                                             const char  *proposed,
                                             gboolean     allow_create,
                                             const char  *path,
                                             void       (*callback)(GtkDialog*, char*, gpointer),
                                             gpointer     param)
{
    GtkWidget     *dialog;
    GtkFileFilter *filter;
    GtkWindow     *window;

    mainlock_assert_is_not_vice_thread();

    filename_cb = callback;
    window = ui_get_active_window();
    dialog = gtk_file_chooser_dialog_new(title,
                                         window,
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         "Select", GTK_RESPONSE_ACCEPT,
                                         "Cancel", GTK_RESPONSE_REJECT,
                                         NULL);

    /* set proposed file name, if any */
    if (proposed != NULL && *proposed != '\0') {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), proposed);
    }

    /* change directory if specified */
    if (path != NULL && *path != '\0') {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
    }

    /* create a custom filter for directories. without it all files will still
       be displayed, which can be irritating */
    filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type (filter, "inode/directory");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), allow_create);

    /* set transient and modal */
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), window);

    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(on_response),
                     param);
    return dialog;
}
