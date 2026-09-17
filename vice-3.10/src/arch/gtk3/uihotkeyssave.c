/** \file   uihotkeyssave.c
 * \brief   File dialog to save a hotkeys file
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

#include "archdep.h"
#include "debug_gtk3.h"
#include "filechooserhelpers.h"
#include "hotkeys.h"
#include "lib.h"
#include "savefiledialog.h"
#include "resources.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uihotkeys.h"
#include "util.h"

#include "uihotkeyssave.h"


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   dialog  dialog
 * \param[in]   data    extra event data (unused)
 */
static void on_destroy(GtkDialog *dialog, gpointer data)
{
    ui_action_finish(ACTION_HOTKEYS_SAVE_TO);
}

/** \brief  Callback for the save dialog
 *
 * \param[in]   dialog      dialog
 * \param[in]   filename    filename entered by user
 * \param[in]   data        extra event data (unused)
 */
static void save_callback(GtkDialog *dialog, gchar *filename, gpointer data)
{
    if (filename != NULL) {
        char buffer[1024];

        if (ui_hotkeys_save_as(filename)) {
            g_snprintf(buffer, sizeof buffer,
                       "Saved hotkeys as %s", filename);
        } else {
            g_snprintf(buffer, sizeof buffer,
                       "Failed to save hotkeys as %s", filename);
        }
        ui_display_statustext(buffer, true);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Show dialog to save a hotkeys file
 */
void ui_hotkeys_save_dialog_show(void)
{
    GtkWidget *dialog;
    const char *hotkeyfile = NULL;
    char *proposed;

    resources_get_string("HotkeyFile", &hotkeyfile);
    if (hotkeyfile != NULL && *hotkeyfile) {
        /* propose current filename and location */
        proposed = lib_strdup(hotkeyfile);
    } else {
        /* construct proposed filename (using cwd) */
        char *full;
        gchar *base;
        char cwd[ARCHDEP_PATH_MAX];

        archdep_getcwd(cwd, sizeof(cwd));
        full = ui_hotkeys_vhk_filename_user();
        base = g_path_get_basename(full);
        lib_free(full);
        proposed = util_join_paths(cwd, base, NULL);
        g_free(base);
    }

    dialog = vice_gtk3_save_file_dialog("Save hotkeys file",
                                        proposed,
                                        TRUE,
                                        NULL,
                                        save_callback,
                                        NULL);
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog), proposed);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_widget_show(dialog);
}
