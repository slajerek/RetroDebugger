/** \file   uihotkeysload.c
 * \brief   File dialog to load a hotkeys file
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

/* Resources set by this file:
 *
 * $VICERES HotkeyFile  all
 */

#include "vice.h"

#include <gtk/gtk.h>

#include "debug_gtk3.h"
#include "filechooserhelpers.h"
#include "openfiledialog.h"
#include "resources.h"
#include "uiactions.h"

#include "uihotkeysload.h"


static void (*extra_callback)(gboolean) = NULL;


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   dialog  dialog (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_destroy(GtkDialog *self, gpointer unused)
{
    ui_action_finish(ACTION_HOTKEYS_LOAD_FROM);
}

/** \brief  Callback for the dialog's response handler
 *
 * Sets resource "HotkeyFile", triggering parsing of file and destroys dialog,
 * marking the UI action `ACTION_HOTKEYS_LOAD_FROM` finished.
 *
 * \param[in]   self        dialog
 * \param[in]   filename    file to load (`NULL` means canceled)
 * \param[in]   data        extra event data (unused)
 */
static void load_callback(GtkDialog *self, gchar *filename, gpointer data)
{
    gboolean result = FALSE;

    if (filename != NULL) {
        debug_gtk3("Got filename '%s'", filename);
        resources_set_string("HotkeyFile", filename);
        g_free(filename);
        result = TRUE;
    }
    gtk_widget_destroy(GTK_WIDGET(self));
    if (extra_callback != NULL) {
        extra_callback(result);
    }
}


/** \brief  Show dialog to load hotkeys file
 *
 * Show file dialog to load hotkeysfile. If \a callback is set it will be called
 * with a boolean parameter that's `TRUE` when the user clicked 'Open'.
 *
 * \param[in]   callback    function to call on response (optional)
 */
void ui_hotkeys_load_dialog_show(void (*callback)(gboolean))
{
    GtkWidget *dialog;
    const char *hotkeyfile = NULL;

    extra_callback = callback;

    resources_get_string("HotkeyFile", &hotkeyfile);

    dialog = vice_gtk3_open_file_dialog("Select a hotkeys file to load",
                                        "Hotkeys files",
                                        file_chooser_pattern_hotkeys,
                                        NULL,
                                        load_callback,
                                        NULL);

    /* set directory and filename to previously selected file, if there's
     * no previous file the dialog uses the current working directory. */
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog), hotkeyfile);

    g_signal_connect(dialog, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_widget_show(dialog);
}
